#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>



#include "lexer.cpp"
#include "Paser.hpp"



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

// parenexpr := '(' expression ')'
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


// primary
// 	::= identifierexpr
// 	::= numberexpr
// 	::= parenexpr
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
	auto LHS = ParsePrimary();
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

    	auto RHS = ParsePrimary();
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
static std::unique_ptr<PrototypeAST> ParsePrototype(){
	if(CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken();

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

	return std::make_unique<PrototypeAST> (FnName, std::move(ArgNames));
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


// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr(){
	if(auto E = ParseExpression()){
		// make an anonymous proto
		auto Proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}




// Top-Level Parsing
static void HandleDefinition(){
	if(ParseDefinition()){
		fprintf(stderr, "Parsed a function definition.\n");
	}else{
		// Skip token for error recovery
		getNextToken();
	}
}

static void HandleExtern(){
	if(ParseExtern()){
		fprintf(stderr, "Parsed an extern\n");
	}else{
		// Skip token for error recovery
		getNextToken();
	}
}


static void HandleTopLevelExpression(){
	// Evaluate a top-level expression into an anonymous function
	if(ParseTopLevelExpr()){
		fprintf(stderr, "Parsed a top-level expr\n");
	}else{
		// Skip token for error recovery
		getNextToken();
	}
}

// top := definition | extern1 | expression | ';'
static void MainLoop(){
	while(1){
		fprintf(stderr, "ready>");
		switch(CurTok){
			case tok_eof:
				return ;
			case ';': // ignore
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

