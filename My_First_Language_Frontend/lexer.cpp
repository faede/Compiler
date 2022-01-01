#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "lexer.hpp"

static int gettok(){
	static int LastChar = ' ';

	// skip whitespace
	while(isspace(LastChar))
		LastChar = getchar();

	// Identifier: [a-zA-Z][a-zA-Z0-9]*
	if(isalpha(LastChar)){
		IdentifierStr = LastChar;	// name of identifer (may be a key name)
		while(isalnum((LastChar = getchar())))
			IdentifierStr += LastChar;

		if(IdentifierStr == "def")
			return tok_def;
		if(IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}

	// Number[0-9.]+
	if(isdigit(LastChar) || LastChar == '.'){
		std::string NumStr;
		do{
			NumStr += LastChar;
			LastChar = getchar();
		}while(isdigit(LastChar) || LastChar == '.');

		// TODO: add check for 1.12.12. 
		NumVal = strtod(NumStr.c_str(), 0);
		return	tok_number;
	}

	// Commmets
	if(LastChar == '#'){
		// Comment line
		do{
			LastChar = getchar();
		}while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		// we just need to skip one char , recursion will handdle
		if(LastChar != EOF)
			return gettok();	// continue process to get a token
	}
		
	// Check EOF
	if(LastChar == EOF)
		return tok_eof;

	// return ascii value.
	// beause only this chars return positive value, we could reconize them
	// such as '<' , '+' , ...
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}