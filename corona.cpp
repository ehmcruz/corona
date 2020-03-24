#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define status_t uint32_t

double cfg_r0 = 2.4;
double cfg_death_rate = 0.02;

class person_t
{
private:
	status_t status;

public:
	person_t();
};

person_t::person_t ()
{

}

class region_t
{
private:

public:
	region_t();
};

static void simulate ()
{

}

int main ()
{
	simulate();

	return 0;
}