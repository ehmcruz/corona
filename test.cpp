#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

void cfg_t::scenery_setup ()
{
	delete this->number_random_connections_dist;
	this->number_random_connections_dist = new normal_double_dist_t(5.0, 1.0, 1.0, 10.0);
}

void region_t::setup_region ()
{
	std::string name;

	name = "TestCity";

	this->set_name(name);

	this->set_population_number( 50 );

	cprintf("total de habitantes: " PU64 "\n", this->get_npopulation());
	
	this->add_people(this->get_npopulation(), 50);

	//std::shuffle(this->people.begin(), this->people.end(), rgenerator);
}

void region_t::callback_before_cycle (uint32_t cycle)
{

}

void region_t::callback_after_cycle (uint32_t cycle)
{

}

void region_t::callback_end ()
{

}
