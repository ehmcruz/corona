#ifndef __LEX_H__
#define __LEX_H__

#include <stdint.h>
#include <string.h>

#define MAX_LABEL_LENGTH 30

enum lex_token_type_t {
	#define _TOKEN_TYPE_(TOKEN) LEX_TOKEN_##TOKEN,
	#include <lex-token-types.h>
	#undef _TOKEN_TYPE_
	LEX_TOKEN_MAX
};

class lex_token_t
{
public:
	union {
		char label[MAX_LABEL_LENGTH];
		char string[MAX_LABEL_LENGTH];
		int32_t vint;
	} data;
	
	lex_token_type_t type;

	inline void copy (lex_token_t *from) {
		memcpy(this, from, sizeof(lex_token_t));
	}
};

class lex_t
{
private:
	char *fname;
	char *buffer;
	uint32_t bsize;
	uint32_t bpos;

	uint32_t col;
	uint32_t row;
	uint32_t has_white_space_token; // default: 0
	uint32_t merge_white_space; // default: 1
	uint32_t has_newline_token; // default: 0

public:
	lex_t (char *fname, uint32_t has_white_space_token, uint32_t merge_white_space, uint32_t has_newline_token);
	~lex_t ();
	void get_token (lex_token_t *token);

	inline char* get_fname () {
		return this->fname;
	}

	inline uint32_t get_col () {
		return this->col;
	}

	inline uint32_t get_row () {
		return this->row;
	}

private:
	void load_file ();
	char io_fgetc_no_inc ();
	void io_inc ();
};

const char* lex_token_str (lex_token_type_t type);

#endif
