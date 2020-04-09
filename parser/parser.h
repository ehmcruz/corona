#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <vector>
#include <string>

#include <lex.h>

class csv_t
{
private:
	lex_t lex;
	std::vector< std::vector<lex_token_t>* > data;
	int32_t has_header;
	uint32_t ncols;
	uint32_t nrows;

public:
	csv_t (char *fname, int32_t has_header);
	~csv_t ();
	void dump ();

private:
	void parse ();
	void get_token (lex_token_t *token);
	void token_check_type (lex_token_t *token, lex_token_type_t type);
};

#endif