#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>
#include <vector>
#include <string>

#include <lex.h>
#include <corona.h>

class csv_t
{
	OO_ENCAPSULATE_RO(uint32_t, ncols)
	OO_ENCAPSULATE_RO(uint32_t, nrows)
private:
	lex_t lex;
	std::vector< std::vector<lex_token_t>* > data;
	int32_t has_header;

public:
	csv_t (char *fname, int32_t has_header);
	~csv_t ();
	void dump ();
	lex_token_t* get_cell (uint32_t row, uint32_t col);

private:
	void parse ();
	void get_token (lex_token_t *token);
	void token_check_type (lex_token_t *token, lex_token_type_t type);
};

class csv_ages_t: public csv_t
{
	OO_ENCAPSULATE_RO(uint32_t, first_age)
	OO_ENCAPSULATE_RO(uint32_t, last_age)
private:
	uint32_t nages;
	uint32_t ncities;
	uint32_t **matrix;

public:
	csv_ages_t (char *fname);
	uint32_t get_population_per_age (char *city_name, uint32_t age);
	uint32_t get_population (char *city_name);

	inline uint32_t get_population_per_age (std::string& city_name, uint32_t age) {
		return this->get_population_per_age((char*)city_name.c_str(), age);
	}

	inline uint32_t get_population (std::string& city_name) {
		return this->get_population((char*)city_name.c_str());
	}

private:
	void validate ();
	void validate_again ();
	void expand ();

	int32_t get_city_row (char *city_name);
};

#endif