#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include <corona.h>
#include <lex.h>

enum lex_state_t {
	LEX_STATE_INIT,
	LEX_STATE_WHITE_SPACE,
	LEX_STATE_CHECK_MINUS_OR_INTEGER,
	LEX_STATE_INTEGER,
	LEX_STATE_LABEL,
	LEX_STATE_STRING,
	LEX_STATE_COMMENT,
	LEX_STATE_END
};

const char* lex_token_str (lex_token_type_t type)
{
	static const char *token_str[] = {
	#define _TOKEN_TYPE_(TOKEN) #TOKEN,
	#include <lex-token-types.h>
	#undef _TOKEN_TYPE_
	};

	C_ASSERT(type < LEX_TOKEN_MAX);
	return (const char*)token_str[type];
}

void lex_t::load_file ()
{
	this->fp = fopen(this->fname, "r");

	C_ASSERT_PRINTF(this->fp != NULL, "cannot open file %s\n", this->fname)

	fseek(this->fp, 0, SEEK_END);
	this->bsize = ftell(this->fp);
	
	dprintf("file %s has %u bytes\n", this->fname, this->bsize);
	
	this->buffer = (char*)malloc(this->bsize + 1);
	C_ASSERT(this->buffer != NULL);
	rewind(this->fp);

	C_ASSERT_PRINTF( fread(this->buffer, 1, this->bsize, this->fp) == this->bsize, "error loading file %s\n", this->fname )

	this->buffer[ this->bsize ] = 0;
	this->bpos = 0;
}

char lex_t::io_fgetc_no_inc ()
{
	return this->buffer[ this->bpos ];
}

void lex_t::io_inc ()
{
	this->bpos++;
}

void lex_t::init (char *fname)
{
	this->fname = fname;
	this->load_file();

	this->col = 1;
	this->row = 1;
	this->has_white_space_token = 0;
	this->merge_white_space = 1;
	this->has_newline_token = 0;
}

void lex_t::get_token (lex_token_t *token)
{
	lex_state_t state;
	char c;
	char *p;
	int32_t number_is_negative, n;
	
	state = LEX_STATE_INIT;
	n = MAX_LABEL_LENGTH;
	
	while (state != LEX_STATE_END) {
		c = this->io_fgetc_no_inc();
		
		switch (state) {
			case LEX_STATE_INIT:
				if (c == 0) {
					token->type = LEX_TOKEN_END;
					state = LEX_STATE_END;
				}
				else if (c == '#') {
					this->io_inc();
					state = LEX_STATE_COMMENT;
				}
				else if (c == ' ' || c == '\t') {
					this->io_inc();
					if (this->has_white_space_token) {
						if (this->merge_white_space)
							state = LEX_STATE_WHITE_SPACE;
						else {
							token->type = LEX_TOKEN_WHITE_SPACE;
							state = LEX_STATE_END;
						}
					}
				}
				else if (iswdigit(c)) {
					p = token->data.label;
					n = 0;
					// *p = c;
					// p++;
					state = LEX_STATE_INTEGER;
					number_is_negative = 0;
					// io_inc(lex);
				}
				else if (iswalpha(c) || c == '_') {
					p = token->data.label;
					n = 0;
					state = LEX_STATE_LABEL;
				}
				else if (c == '\n') {
					this->io_inc();
					this->row++;
					if (this->has_newline_token) {
						token->type = LEX_TOKEN_NEWLINE;
						state = LEX_STATE_END;
					}
				}
				else if (c == '-') {
					this->io_inc();
					n = 0;
					state = LEX_STATE_CHECK_MINUS_OR_INTEGER;
				}
				else if (c == '[') {
					this->io_inc();
					token->type = LEX_TOKEN_OPEN_BRACKETS;
					state = LEX_STATE_END;
				}
				else if (c == ']') {
					this->io_inc();
					token->type = LEX_TOKEN_CLOSE_BRACKETS;
					state = LEX_STATE_END;
				}
				else if (c == ':') {
					this->io_inc();
					token->type = LEX_TOKEN_COLON;
					state = LEX_STATE_END;
				}
				else if (c == ',') {
					this->io_inc();
					token->type = LEX_TOKEN_COMMA;
					state = LEX_STATE_END;
				}
				else if (c == '.') {
					this->io_inc();
					token->type = LEX_TOKEN_DOT;
					state = LEX_STATE_END;
				}
				else if (c == ';') {
					this->io_inc();
					token->type = LEX_TOKEN_SEMICOLON;
					state = LEX_STATE_END;
				}
				else if (c == '"') {
					p = token->data.string;
					n = 0;
					state = LEX_STATE_STRING;
					this->io_inc();
				}
				else {
					cprintf("lex error line %u char %c\n", this->row, c);
					exit(1);
				}
				break;

			case LEX_STATE_COMMENT:
				this->io_inc();
				if (c == '\n') {
					this->row++;
					if (this->has_newline_token) {
						token->type = LEX_TOKEN_NEWLINE;
						state = LEX_STATE_END;
					}
					else
						state = LEX_STATE_INIT;
				}
				break;

			case LEX_STATE_CHECK_MINUS_OR_INTEGER:
				if (iswdigit(c)) {
					C_ASSERT_PRINTF(n < MAX_LABEL_LENGTH, "token max length is %u chars", MAX_LABEL_LENGTH)
					n++;

					p = token->data.label;
					*p = '-';
					p++;
					state = LEX_STATE_INTEGER;
					number_is_negative = 1;
				}
				else {
					token->type = LEX_TOKEN_MINUS;
					state = LEX_STATE_END;
				}
				break;
			
			case LEX_STATE_WHITE_SPACE:
				if (c == ' ' || c == '\t')
					this->io_inc();
				else {
					token->type = LEX_TOKEN_WHITE_SPACE;
					state = LEX_STATE_END;
				}
				break;
			
			case LEX_STATE_INTEGER:
				C_ASSERT_PRINTF(n < MAX_LABEL_LENGTH, "token max length is %u chars", MAX_LABEL_LENGTH)
				n++;

				if (iswdigit(c)) {
					*p = c;
					p++;
					this->io_inc();
				}
				else {
					*p = 0;
					token->data.vint = atoi(token->data.label);
					token->type = LEX_TOKEN_INTEGER;
					state = LEX_STATE_END;
				}
				break;
			
			case LEX_STATE_LABEL:
				C_ASSERT_PRINTF(n < MAX_LABEL_LENGTH, "token max length is %u chars", MAX_LABEL_LENGTH)
				n++;

				if (iswalnum(c) || c == '_') {
					*p = c;
					p++;
					this->io_inc();
				}
				else {
					*p = 0;
					token->type = LEX_TOKEN_LABEL;
					state = LEX_STATE_END;
				}
				break;
			
			case LEX_STATE_STRING:
				C_ASSERT_PRINTF(n < MAX_LABEL_LENGTH, "token max length is %u chars", MAX_LABEL_LENGTH)
				n++;

				if (c != '"') {
					*p = c;
					p++;
					this->io_inc();
				}
				else {
					*p = 0;
					this->io_inc();
					token->type = LEX_TOKEN_STRING;
					state = LEX_STATE_END;
				}
				break;

			default:
				C_ASSERT(0);
		}
	}
}
