// BinopPrecedence - This holds the precedence for each binary operator that is
// defined.
static std::map<char, int> BinopPrecedence;

// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence(){
	if(!isascii(CurTok))
		retunr -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	if(TokPrec <= 0)
		return -1;
	return TokPrec;
}

int main(){
	// Install standard binary operators.
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40; // highest.
}

// expression
//   ::= primary binoprhs
//
static std::unique_ptr<ExprAST> ParseExpression(){
	auto LHS = ParsePrimary();
	if(!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS));
}

// binoprhs
//   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
												std::unique_ptr<ExprAST> LHS){
	// If this is a binop, find its precedence
	while(1){
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
    	// consume it, otherwise we are done.
    	if(TokPrec < ExprPrec){
    		return LHS;
    	}

    	// Okay, we know this is a binop.
    	int BinOp = CurTok;
    	getNextToken(); // eat binop

    	// Parse the primary expression after the binary.
    	auto RHS = ParsePrimary();
    	if(!RHS)
    		return nullptr;

    	// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if(!RHS)
				return nullptr;
		}
		// Merge LHS/RHS.
		LHS	= std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));

	} // loop around to the top of the while loop.
}


// prototype
//   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype(){
	if(CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");
	std::string FnName = IdentifierStr;
	getNextToken();

	if(CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	// Read the list of argument names.
	std::vector<std::string> ArgNames;
	while(getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if(CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success.
	getNextToken(); // eat ')'.

	return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition(){
	getNextToken(); // eat def.
	auto Proto = ParsePrototype();
	if(!Proto)
		return nullptr;
	if(auto E = ParseExpression())
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern(){
	getNextToken(); // eat extern.
	return ParsePrototype();
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr(){
	if(auto E = ParseExpression()){
		// Make an anonymous proto.
		auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

















