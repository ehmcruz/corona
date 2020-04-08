#include <corona.h>
#include <parser.h>

csv_t::csv_t (char *fname, int32_t has_header)
	: lex(fname, 0, 1, 1)
{
	this->has_header = has_header;
	this->parse();
}

csv_t::~csv_t ()
{

}

enum parser_state_t {
	PSTATE_HEADER,
};

void csv_t::parse ()
{
	parser_state_t state;
	lex_token_t token;

	state = PSTATE_HEADER;

	switch (state) {
		case PSTATE_HEADER:
			this->lex.get_token(&token);
		break;

		default:
			C_ASSERT(0)
	}
}