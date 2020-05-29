#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK;

	this->n_regions = 2;
}

void region_t::setup_population ()
{
	std::string name;

	name = "TestCity";
	name += std::to_string(this->get_id());

	this->set_name(name);

	if (this->get_name() == "TestCity0")
		this->set_population_number( 150 );
	else
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
		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);
		normal_double_dist_t dist_number_random_connections(5.0, 5.0, 5.0, 100.0);

		this->create_families(dist_family_size);
		this->create_random_connections(dist_number_random_connections);

		normal_double_dist_t dist_school_class_size(5.0, 1.0, 2.0, 50.0);
		std::vector<region_double_pair_t> school;

		school.push_back( {this, 0.8} );
		
		network_create_school_relation(school, 20, 20, dist_school_class_size);
	}
}

void setup_inter_region_relations ()
{
	normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);
	std::vector<region_double_pair_t> school;

//	network_print_population_graph();
//	exit(0);

	dprintf("\n----------------------------------------\n");

	school.push_back( {region_t::get("TestCity0"), 0.8} );
	school.push_back( {region_t::get("TestCity1"), 0.8} );

	network_create_school_relation(school, 20, 21, dist_school_class_size, 0.5, 0.025);

	network_print_population_graph( {RELATION_SCHOOL} );
	exit(0);
}

void setup_extra_relations ()
{

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
