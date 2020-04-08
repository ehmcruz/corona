#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <vector>

#include <lex.h>

class csv_t
{
private:
	lex_t lex;
	std::vector data;
	std::vector header;
	int32_t has_header;

public:
	csv_t (char *fname, int32_t has_header);
	~csv_t ();

private:
	void parse ();
};

#endif