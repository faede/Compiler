expr &rarr;  term rest

rest &rarr;  + term {print('+')} rest

​		|    - term {print('-')} rest

​		|    $\epsilon$ 

term &rarr; 0 {print('0')}

​		  | 1 {print('1')}

​			....

​		  | 9 {print('9')}

```cpp
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

```

