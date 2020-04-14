#include <iostream>
#include <cctype>
#include <stdlib.h>
using namespace std;
class Parser {
	//static int lookahead;
	int lookahead;
public:
	Parser() {
		lookahead = getchar();
	}
	void expr() {
		term();
		while (true) {
			if (lookahead == '+') {
				match('+');
				term();
				cout << '+';
			}
			else if (lookahead == '-') {
				match('-');
				term();
				cout << '-';
			}
			else {
				return;
			}
		}
	}
	void term() {
		if (isdigit(lookahead)) {
			cout << (char)lookahead;
			match(lookahead);
		}
		else {
			cout << "syntax error";
			exit(-1);
		}
	}
	void match(int t) {
		if (lookahead == t) {
			lookahead = getchar();
		}
		else {
			cout << "syntax error";
			exit(-1);
		}
	}
};
int main() {
	Parser  parser;
	parser.expr();
	cout << endl;
	return 0;
}
