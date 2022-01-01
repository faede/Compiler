#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// ExprAST
class ExprAST{
public:
	virtual ~ExprAST(){};
};


// NumberExprAST
class NumberExprAST : public ExprAST{
	double Val;
public:
	NumberExprAST(double _Val) : Val(_Val) {}
};


// VariableExprAST
class VariableExprAST : public ExprAST{
	std::string Name;

public:
	VariableExprAST(const std::string &Name) : Name(Name) {}
};


// BinaryExprAST
class BinaryExprAST : public ExprAST{
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char _Op, std::unique_ptr<ExprAST> _LHS, std::unique_ptr<ExprAST> _RHS)
		: Op(_Op), LHS(std::move(_LHS)), RHS(std::move(_RHS)) {}
};


// CallExprAST
class CallExprAST : public ExprAST{
	std::string Callee;
	std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(const std::string &_Callee, std::vector<std::unique_ptr<ExprAST>> _Args)
		: Callee(_Callee), Args(std::move(_Args)) {}
};


// PrototypeAST, represent the "protype" for a  function
// since all type is double, we don't need to save it
class PrototypeAST{
	std::string Name;
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string &_Name, std::vector<std::string> _Args)
		: Name(_Name), Args(_Args) {}

	const std::string &getName() const { return Name; }
};

// FunctionAST
class FunctionAST{
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body;

public:
	FunctionAST(std::unique_ptr<PrototypeAST> _Proto, std::unique_ptr<ExprAST> _Body)
		: Proto(std::move(_Proto)), Body(std::move(_Body)) {}
};



static int getNextToken();

// LogError*
std::unique_ptr<ExprAST> LogError(const char *);
std::unique_ptr<PrototypeAST> LogErrorP(const char *);

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr();

// parenexpr := '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr();

// identifierexpr
// 	::= identifier
// 	::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr();

// primary
// 	::= identifierexpr
// 	::= numberexpr
// 	::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary();

// Operator-Precedence Parsing
// BinopPrecedence, holds the precedence for each binary operator that is defined
static std::map<char, int> BinopPrecedence; 

// GetTokPrecedence
static int GetTokPrecedence();

// expression
// 	::= primary binoprhs
static std::unique_ptr<ExprAST> ParseExpression();

// binoprhs
// 	::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int , std::unique_ptr<ExprAST> );

// prototype
// 	::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype();

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition();

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern();

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr();

// Top-Level Parsing
static void HandleDefinition();
static void HandleExtern();
static void HandleTopLevelExpression();

// top := definition | extern1 | expression | ';'
static void MainLoop();



























