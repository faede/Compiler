// ExprAST - Base class for all expression nodes.
class ExprAST{
public:
	virtual ~ExprAST(){} = default;
};

// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST{
	double Val;

public:
	NumberExprAST(double _Val) : Val(_Val){}
};

// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST{
	std::string Name;

public:
	VariableExprAST(const std::string &_Name) : Name(_Name){}
};

// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST{
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char _op, std::unique_ptr<ExprAST> _LHS, 
					std::unique_ptr<ExprAST> _RHS)
		: Op(_op), LHS(std::mov(_LHS)), RHS(std::move(_RHS)){}
};

// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST{
	std::string Callee;
	std::std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(cosnt std::string &_Callee,
				std::std::vector<std::unique_ptr<ExprAST>> _Args)
		: Callee(_Callee), Args(std::move(_Args)) {}
};

// PrototypeAST - This class represents the "prototype" for a function,
// which captures its name, and its argument names (thus implicitly the number
// of arguments the function takes).
class PrototypeAST{
	std::string Name;
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string & _name, std::vector<std::string> _Args)
		: Name(_name), Args(std::move(_Args)) {}

	const std::string &getName() const{ return Name; }
};
 
// FunctionAST - This class represents a function definition itself.
class FunctionAST{
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body;

public:
	FunctionAST(std::unique_ptr<PrototypeAST> _Proto,
				std::unique_ptr<ExprAST> _Body)
		: Proto(std::move(_Proto)), Body(std::move(_Body)) {}
};