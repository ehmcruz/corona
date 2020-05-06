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
				this->nrows++;

				if (first_line == NULL)
					first_line = line;
			}

			line->push_back(token);

			if (this->nrows == 1)
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

	C_ASSERT(this->nrows == this->data.size())
	C_ASSERT(this->ncols == this->data[0]->size())
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

lex_token_t* csv_t::get_cell (uint32_t row, uint32_t col)
{
	C_ASSERT(row < this->nrows)
	C_ASSERT(col < this->ncols)

	std::vector<lex_token_t>& line = *this->data[row];

	return &line[col];
}

csv_ages_t::csv_ages_t (char *fname)
	: csv_t(fname, 1)
{
	this->validate();
	this->expand();
	this->validate_again();
}

void csv_ages_t::validate ()
{
	int32_t i, j, m, total;
	lex_token_t *token, *token2;

	C_ASSERT(this->get_nrows() >= 3) // header, city*, total
	C_ASSERT(this->get_ncols() >= 4) // city code, city name, age*, total

	// check header
	token = this->get_cell(0, 0);
	C_ASSERT( token->type == LEX_TOKEN_LABEL && strcmp(token->data.label, "Codigo") == 0 )
	token = this->get_cell(0, 1);
	C_ASSERT( token->type == LEX_TOKEN_LABEL && strcmp(token->data.label, "Municipio") == 0 )
	m = this->get_ncols() - 1;
	token = this->get_cell(0, m);
	C_ASSERT( token->type == LEX_TOKEN_LABEL && strcmp(token->data.label, "TOTAL") == 0 )

	// check if first line has ages
	for (i=2; i<m; i++) {
		token = this->get_cell(0, i);
		C_ASSERT( token->type == LEX_TOKEN_INTEGER )
	}
	// check if next age is higher than previous
	for (i=3; i<m; i++) {
		token = this->get_cell(0, i);
		token2 = this->get_cell(0, i-1);

		C_ASSERT( token->data.vint > token2->data.vint )
	}

	// check if first col has codes
	for (i=1; i<this->get_nrows(); i++) {
		token = this->get_cell(i, 0);
		C_ASSERT( token->type == LEX_TOKEN_INTEGER )
	}

	// check if second col has names
	for (i=1; i<this->get_nrows(); i++) {
		token = this->get_cell(i, 1);
		C_ASSERT( token->type == LEX_TOKEN_LABEL )
	}
	// check if last row has total
	m = this->get_nrows() - 1;
	token = this->get_cell(m, 1);
	C_ASSERT( token->type == LEX_TOKEN_LABEL && strcmp(token->data.label, "TOTAL") == 0 )

	// check if all other values are numbers
	for (i=1; i<this->get_nrows(); i++) {
		for (j=2; j<this->get_ncols(); j++) {
			token = this->get_cell(i, j);
			C_ASSERT( token->type == LEX_TOKEN_INTEGER )
		}
	}

	// check total of rows
	m = this->get_ncols() - 1;
	for (i=1; i<this->get_nrows(); i++) {
		total = 0;
		for (j=2; j<m; j++) {
			token = this->get_cell(i, j);
			total += token->data.vint;
		}
		token = this->get_cell(i, m);
		C_ASSERT( total == token->data.vint )
	}

	// check total of cols
	m = this->get_nrows() - 1;
	for (j=2; j<this->get_ncols(); j++) {
		total = 0;
		for (i=1; i<m; i++) {
			token = this->get_cell(i, j);
			total += token->data.vint;
		}
		token = this->get_cell(m, j);
		C_ASSERT( total == token->data.vint )
	}
}

void csv_ages_t::expand ()
{
	uint32_t i, j, k, row, col, total, age, prev_age, diff, backup;
	lex_token_t *token;

	token = this->get_cell(0, 2);
	this->first_age = token->data.vint;
	token = this->get_cell(0, this->get_ncols()-2);
	this->last_age = token->data.vint;
	dprintf("csv first age %u last age %u\n", this->first_age, this->last_age);

	C_ASSERT(this->first_age == 0)
	
	this->nages = (this->last_age - this->first_age) + 1;
	this->ncities = this->get_nrows() - 2; // first row is header, last row is TOTAL

	this->matrix = matrix_malloc<uint32_t>(this->ncities, this->nages);

	col = 2;
	for (i=0, row=1; i<this->ncities; i++, row++) {
		token = this->get_cell(row, col);
		this->matrix[i][0] = token->data.vint;
	}

	for (i=0, row=1; i<this->ncities; i++, row++) {
		// only for debug - START
		j = 0;
		col = 2;
		age = this->get_cell(0, col)->data.vint;
		token = this->get_cell(row, col);
		dprintf("set age:%u j:%u -> %u\n", age, j, token->data.vint);
		// only for debug - END

		for (j=1, col=3; col<=this->get_ncols()-2; j++, col++) {
			token = this->get_cell(row, col);
			age = this->get_cell(0, col)->data.vint;
			prev_age = this->get_cell(0, col-1)->data.vint;

			if (age > (prev_age + 1)) {
				diff = age - prev_age;
				dprintf("found gap age=%i prev_age=%u j=%u col=%u\n", age, prev_age, j, col);

				total = 0;
				backup = this->matrix[i][prev_age];
				for ( ; j<age; j++) {
					this->matrix[i][j] = this->matrix[i][prev_age] / diff;
					total += this->matrix[i][j];

					dprintf("extrapolate age-j:%u from age:%u -> %u\n", j, prev_age, this->matrix[i][j]);
				}

				this->matrix[i][prev_age] -= total;
				dprintf("fixed prev_age %u from %u to %u\n", prev_age, backup, this->matrix[i][prev_age]);

				total = 0;
				for (k=prev_age; k<age; k++)
					total += this->matrix[i][k];
				
				C_ASSERT(total == backup)
			}

			dprintf("set age:%u j:%u -> %u\n", age, j, token->data.vint);

			this->matrix[i][j] = token->data.vint;
		}

		C_ASSERT(j == this->nages)
	}

	C_ASSERT(row == (this->get_nrows() - 1))
}

void csv_ages_t::validate_again ()
{
	uint32_t i, j, total;
	lex_token_t *token;

	for (i=0; i<this->ncities; i++) {
		total = 0;
		for (j=0; j<this->nages; j++) {
			total += this->matrix[i][j];
		}

		token = this->get_cell(i+1, this->get_ncols()-1);
		
		C_ASSERT_PRINTF(total == token->data.vint, "i:%u total:%u cmp:%u\n", i, total, token->data.vint)
	}
}

int32_t csv_ages_t::get_city_row (char *city_name)
{
	int32_t i;

	lex_token_t *token;

	for (i=0; i<this->ncities; i++) {
		token = this->get_cell(i+1, 1);

		if (strcmp(token->data.label, city_name) == 0)
			return i;
	}

	C_ASSERT_PRINTF(0, "city %s not found\n", city_name)
	return -1;
}

uint32_t csv_ages_t::get_population_per_age (char *city_name, uint32_t age)
{
	uint32_t i;

	i = this->get_city_row(city_name);

	if (age < this->first_age || age > this->last_age)
		return 0;

	return this->matrix[i][age];
}

uint32_t csv_ages_t::get_population (char *city_name)
{
	uint32_t i, j, total;

	i = this->get_city_row(city_name);

	total = 0;
	for (j=0; j<this->nages; j++)
		total += this->matrix[i][j];

	return total;
}

char* csv_ages_t::get_city_name (uint32_t row)
{
	lex_token_t *token;

	C_ASSERT(row < this->ncities)

	token = this->get_cell(row+1, 1);

	return token->data.label;
}