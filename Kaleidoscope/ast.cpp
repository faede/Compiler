class ExprAST{
public:
	virtual ~ExprAST(){}
};

class NumberExprAST : public ExprAST{
	double Val;

public:
	NumberExprAST(double _Val) : Val(_Val){}
};

class VariableExprAST : public ExprAST{
	std::string Name;

public:
	VariableExprAST(const std::string &_Name) : Name(_Name){}
};

class BinaryExprAST : public ExprAST{
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char _op, std::unique_ptr<ExprAST> _LHS, 
					std::unique_ptr<ExprAST> _RHS)
		: Op(_op), LHS(std::mov(RHS)), RHS(std::move(RHS)){}
};

class CallExprAST : public ExprAST{
	std::string Callee;
	std::std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(cosnt std::string &_Callee,
				std::std::vector<std::unique_ptr<ExprAST>> _Args)
		: Callee(_Callee), Args(std::move(_Args)) {}
};