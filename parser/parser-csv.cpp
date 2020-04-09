#include <stdlib.h>

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

#define syntax_error_printf(...) { cprintf("syntax error file %s line %u: ", this->lex.get_fname(), this->lex.get_row()); cprintf(__VA_ARGS__); cprintf("\n"); exit(1); }

void csv_t::get_token (lex_token_t *token)
{
	this->lex.get_token(token);
	dprintf("fetched token type %s\n", lex_token_str(token->type));
}

void csv_t::token_check_type (lex_token_t *token, lex_token_type_t type)
{
	if (token->type != type) {
		syntax_error_printf("expected %s, given %s", lex_token_str(type), lex_token_str(token->type));
	}
}

void csv_t::parse ()
{
	lex_token_t token;
	std::vector<lex_token_t> *line = NULL;
	std::vector<lex_token_t> *first_line = NULL;

	this->ncols = 0;
	this->nrows = 0;

	this->get_token(&token);

	do {
		if (token.type == LEX_TOKEN_INTEGER || token.type == LEX_TOKEN_LABEL || token.type == LEX_TOKEN_STRING) {
			if (line == NULL) {
				line = new std::vector<lex_token_t>;
				this->data.push_back( line );

				if (first_line == NULL)
					first_line = line;
			}

			line->push_back(token);
			this->ncols++;

			this->get_token(&token);

			if (token.type == LEX_TOKEN_COMMA) {
				this->get_token(&token);
			}
			else if (token.type == LEX_TOKEN_NEWLINE) {
				if (line->size() != first_line->size())
					syntax_error_printf("all lines should have the same number of cols");

				this->get_token(&token);
				line = NULL;
			}
			else {
				syntax_error_printf("expected token %s or %s, provided %s", lex_token_str(LEX_TOKEN_COMMA), lex_token_str(LEX_TOKEN_NEWLINE), lex_token_str(token.type));
			}
		}
		else if (token.type == LEX_TOKEN_COMMA) { // comma after comma, or comma after newline
			syntax_error_printf("comma should be after data");
		}
		else if (token.type == LEX_TOKEN_NEWLINE) { // newline after newline, or newline after comma
			if (line == NULL) // empty line
				this->get_token(&token);
			else {
				syntax_error_printf("newline should be after data");
			}
		}
		else if (token.type != LEX_TOKEN_END) {
			syntax_error_printf("something really wrong with the csv");
		}
	} while (token.type != LEX_TOKEN_END);
}

void csv_t::dump ()
{
	uint32_t i, j;	;

	for (i=0; i<this->data.size(); i++) {
		std::vector<lex_token_t>& line = *this->data[i];

		for (j=0; j<line.size(); j++) {
			lex_token_t& token = line[j];

			switch (token.type) {
				case LEX_TOKEN_INTEGER:
					cprintf("%i, ", token.data.vint);
				break;

				case LEX_TOKEN_LABEL:
					cprintf("%s, ", token.data.label);
				break;

				case LEX_TOKEN_STRING:
					cprintf("%s, ", token.data.string);
				break;

				default:
					C_ASSERT(0)
			}
		}

		cprintf("\n");
	}
}

csv_ages_t::csv_ages_t (char *fname)
	: csv_t(fname, 1)
{

}
