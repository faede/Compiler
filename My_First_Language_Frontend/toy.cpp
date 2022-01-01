#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Paser.cpp"


int main(){
	// Install standard binary operators.
	// 1 is loweset precedence
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40; // highest precedence

	// Prime the first token
	fprintf(stderr, "ready>");
	getNextToken();

	// make the module
	InitializeModule();

	// run
	MainLoop();

	// print out all of the generated code
	TheModule->print(errs(), nullptr);

	return 0;
}