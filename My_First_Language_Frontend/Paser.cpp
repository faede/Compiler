//#include "KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>


using namespace llvm;
//using namespace llvm::orc;

#include "lexer.cpp"
//#include "Paser.hpp"
#include "IR.cpp"


// save cunrrent token
static int CurTok; // it is key->number or char
static int getNextToken(){
	return	CurTok = gettok();
}


// LogError*
std::unique_ptr<ExprAST> LogError(const char *Str){
	fprintf(stderr, "LogError: %s\n", Str);
	return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str){
	LogError(Str);
	return nullptr;
}


// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr(){
	auto Result = std::make_unique<NumberExprAST>(NumVal);
	getNextToken();	// eat number
	return std::move(Result);
}

// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr(){
	getNextToken(); // eat (

	auto V = ParseExpression();

	if(!V)
		return nullptr;

	if(CurTok != ')')
		return LogError("Expected ')'");

	getNextToken(); // eat )
	return V;
}


// identifierexpr
// 	::= identifier
// 	::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr(){
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier

	if(CurTok != '(') // simple variable ref
		return std::make_unique<VariableExprAST> (IdName);

	// identifier + '(', we can conduce it is a fucntion call
	// Call
	getNextToken(); // eat (
	std::vector<std::unique_ptr<ExprAST>> Args;
	if(CurTok != ')'){
		// has arguments
		while(1){
			if(auto Arg = ParseExpression()){
				Args.push_back(std::move(Arg));
			}else{
				return nullptr;
			}

			if(CurTok == ')')
				break; 	// finish

			if(CurTok != ',')
				return LogError("Expected ')' or ',' in arguments list");
			getNextToken();
		}
	}

	getNextToken(); // eat ')'

	return std::make_unique<CallExprAST> (IdName, std::move(Args));
}

// varexpr ::= 'var' identifier ('=' expression)?
// 					 (',' identifier ('=' expression)?)* 'in' expression
static std::unique_ptr<ExprAST> ParseVarExpr(){
	getNextToken(); // eat var

	std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

	// At least one variable name is required
	if(CurTok != tok_identifier)
		return LogError("Expected identifier after var");

	while(1){
		std::string Name = IdentifierStr;
		getNextToken(); // eat identifier

		// read the optional initializer
		std::unique_ptr<ExprAST> Init = nullptr;
		if(CurTok == '='){
			getNextToken(); // eat '='

			Init = ParseExpression();
			if(!Init)
				return nullptr;
		}

		VarNames.push_back(std::make_pair(Name, std::move(Init)));

		// End of var list, exit loop
		if(CurTok != ',')
			break;
		getNextToken(); // eat ','

		if(CurTok != tok_identifier)
			return LogError("Expected identifier list after var");
	}

	// at this point, we have to have 'in' 
	if(CurTok != tok_in)
		return LogError("Expected 'in' keyword after 'var'");
	getNextToken(); // eat 'in'

	auto Body = ParseExpression();
	if(!Body)
		return nullptr;

	return std::make_unique<VarExprAST> (std::move(VarNames), std::move(Body));
}


// primary
// 	::= identifierexpr
// 	::= numberexpr
// 	::= parenexpr
//  ::= ifexpr
static std::unique_ptr<ExprAST> ParsePrimary(){
	switch(CurTok){
		default:
			return LogError("Unknown token when expcting an expression");
		case tok_identifier:
			return ParseIdentifierExpr();
		case tok_number:
			return ParseNumberExpr();
		case '(':
			return ParseParenExpr();
		case tok_if:
			return ParseIfExpr();
		case tok_for:
			return ParseForExpr();
		case tok_var:
			return ParseVarExpr();
	}
} 




// GetTokPrecedence
static int GetTokPrecedence(){
	if(!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop
	int TokPrec = BinopPrecedence[CurTok];
	if(TokPrec <= 0)
		return -1;
	return TokPrec;
}


// expression
// 	::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression(){
	auto LHS = ParseUnary(); // primary | unary
	if(!LHS)
		return nullptr;

	// we set 0, beause first LHS, will not be evaulated
	return ParseBinOpRHS(0, std::move(LHS));
}


// binoprhs
// 	::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS){
	while(1){
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
    	// consume it, otherwise we are done
    	// other is will be set to -1 in GetTokPrecedence, so we can checkout
    	if(TokPrec < ExprPrec)
    		return LHS;

    	// this is a binop
    	int BinOp = CurTok;
    	getNextToken(); // eate binop

    	// Parse the unary expression | primary expression
    	auto RHS = ParseUnary();
    	if(!RHS)
    		return nullptr;

    	// look ahead
    	int NextPrec = GetTokPrecedence();
    	if(TokPrec < NextPrec){
    		// we know that any sequence of pairs whose operators are all higher precedence 
    		// than "TokPrec" should be parsed together and returned as “RHS”
    		// specifying “TokPrec+1” as the minimum precedence required for it to continue
    		RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
    		if(!RHS)
    			return nullptr;
    	}

    	// merge
    	LHS = std:: make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}


// prototype
// 	::= id '(' id* ')'
//  ::= binary LETTER number? (id, id)
static std::unique_ptr<PrototypeAST> ParsePrototype(){
	std::string FnName;

	unsigned Kind = 0; // 0 => identifier, 1 => unary, 2 => binary (number)
	unsigned BinaryPrecedence = 30;

	switch(CurTok){
		default:
			return LogErrorP("Expected function name in prototype");
		case tok_identifier:
			FnName = IdentifierStr;
			Kind = 0;
			getNextToken();
			break;
		case tok_unary:
			getNextToken();
			if(!isascii(CurTok))
				return LogErrorP("Expected unary operator");
			FnName = "unary";
			FnName += (char)CurTok;
			Kind = 1;
			getNextToken();
			break;
		case tok_binary:
			getNextToken();
			if(!isascii(CurTok))
				return LogErrorP("Expected binary operator");
			FnName = "binary";
			FnName += (char)CurTok;
			Kind = 2;
			getNextToken();

			// read the precedence if present
			if(CurTok == tok_number){
				if(NumVal < 1 || NumVal > 100)
					return LogErrorP("Invalid precedence: must be 1..100");
				BinaryPrecedence = (unsigned)NumVal;
				getNextToken();
			}
			break;
	}


	if(CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	// read argument names
	std::vector<std::string> ArgNames;
	while(getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if(CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// finish
	getNextToken(); // eat ')'

	// verify right number of names for operator
	if(Kind && ArgNames.size() != Kind)
		return LogErrorP("Invalid number of operands for operator");

	return std::make_unique<PrototypeAST> (FnName, std::move(ArgNames), Kind != 0,
											BinaryPrecedence);
}


// unary
//    ::= primary
//    ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary(){
	// if current token is not an operator, it must be a primary expr
	if(!isascii(CurTok) || CurTok == '(' || CurTok == ',')
		return ParsePrimary();

	// is a unary operator
	int Opc = CurTok;
	getNextToken(); // eat
	if(auto Operand = ParseUnary())
		return std::make_unique<UnaryExprAST> (Opc, std::move(Operand));
	return nullptr;
}

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition(){
	getNextToken(); // eat def
	auto Proto = ParsePrototype();
	if(!Proto)
		return nullptr;

	if(auto E = ParseExpression())
		return std::make_unique<FunctionAST> (std::move(Proto), std::move(E));
	return nullptr;
}


// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern(){
	getNextToken(); // eat extern
	return ParsePrototype();
}


// ifexpr := 'if' expression 'then' expression 'else' expression
static std::unique_ptr<ExprAST> ParseIfExpr(){
	getNextToken(); // eat if

	// condition
	auto Cond = ParseExpression();
	if(!Cond)
		return nullptr;
	
	if(CurTok != tok_then)
		return LogError("Expected then");
	getNextToken(); // eat then

	auto Then = ParseExpression();
	if(!Then)
		return nullptr;

	if(CurTok != tok_else)
		return LogError("Expected else");
	
	getNextToken();  // eat else

	auto Else = ParseExpression();
	if(!Else)
		return nullptr;

	return std::make_unique<IfExprAST> (std::move(Cond), std::move(Then), std::move(Else));
}


/*

extern putchard(char);
def printstar(n)
  for i = 1, i < n, 1.0 in
    putchard(42);  # ascii 42 = '*

*/

// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
static std::unique_ptr<ExprAST> ParseForExpr(){
	getNextToken(); // eat for

	if(CurTok != tok_identifier)
		return LogError("Expected identifier after for");

	std::string IdName = IdentifierStr;
	getNextToken(); // eat identifier

	if(CurTok != '=')
		return LogError("Expected '=' after for");
	getNextToken(); // eat '='

	auto Start = ParseExpression();
	if(!Start)
		return nullptr;
	if(CurTok != ',')
		return LogError("Expected ','  after for start value");
	getNextToken(); // eat ','

	auto End = ParseExpression();
	if(!End)
		return nullptr;

	// Step value is optional
	std::unique_ptr<ExprAST> Step;
	if(CurTok == ','){
		getNextToken(); // eate ','
		Step = ParseExpression();
		if(!Step)
			return nullptr;
	}

	if(CurTok != tok_in)
		return LogError("Expected 'in' after for");
	getNextToken(); // eat 'in'

	auto Body = ParseExpression();
	if(!Body)
		return nullptr;

	return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), 
							std::move(Step), std::move(Body));
}




// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr(){
	if(auto E = ParseExpression()){
		// make an anonymous proto
		auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}




// Top-Level Parsing and JIT Driver
static void InitializeModuleAndPassManager() {
  // Open a new module.
  TheModule = std::make_unique<Module>("my cool jit", TheContext);
}


/*

static void InitializeModule(){
	// open a new context and module
	TheContext = std::make_unique<LLVMContext>();
	TheModule = std::make_unique<Module>("my cool jit", TheContext);

	// create a new builder
	Builder = std::make_unique<IRBuilder<>>(*TheContext);
}
*/

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    FnAST->codegen();
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

// top := definition | extern1 | expression | ';'
static void MainLoop() {
  while (true) {
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}

