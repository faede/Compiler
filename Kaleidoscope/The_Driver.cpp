/// top ::= definition | external | expression | ';'
static void MainLoop(){
	while(1){
		fprintf(stderr, "ready>");
		switch(CurTok){
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