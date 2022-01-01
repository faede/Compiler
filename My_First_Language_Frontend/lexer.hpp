#include <string>
enum Token
{
	tok_eof = -1,

	// commans
	tok_def = -2,
	tok_extern = -3,

	// primary
	tok_identifier = -4,
	tok_number = -5,
};

// save current string / number
static std::string IdentifierStr; 
static double NumVal;