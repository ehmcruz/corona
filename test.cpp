#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK;

	delete this->number_random_connections_dist;
	this->number_random_connections_dist = new normal_double_dist_t(5.0, 1.0, 1.0, 10.0);

	this->n_regions = 2;
}

void region_t::setup_population ()
{
	std::string name;

	name = "TestCity";
	name += std::to_string(this->get_id());

	this->set_name(name);

	this->set_population_number( 100 );

	cprintf("total de habitantes: " PU64 "\n", this->get_npopulation());
	
	this->add_people(50, 20);
	this->add_people(this->get_npopulation() - 50, 21);

	//std::shuffle(this->people.begin(), this->people.end(), rgenerator);
}

void region_t::setup_health_units ()
{

}

void region_t::setup_relations ()
{
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		normal_double_dist_t dist_school_class_size(5.0, 1.0, 2.0, 50.0);
		std::vector<region_double_pair_t> school;

//		this->create_families();
//		this->create_random_connections();

		school.push_back( {this, 0.8} );
		
		network_create_school_relation(school, 20, 20, dist_school_class_size);
	}
}

void setup_inter_region_relations ()
{
	normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);
	std::vector<region_double_pair_t> school;

	dprintf("\n----------------------------------------\n");

	school.push_back( {region_t::get("TestCity0"), 0.8} );
	school.push_back( {region_t::get("TestCity1"), 0.8} );

//	network_create_school_relation(school, 20, 20, dist_school_class_size);
	
	exit(0);
}

void callback_before_cycle (double cycle)
{

}

void callback_after_cycle (double cycle)
{

}

void callback_end ()
{

}
