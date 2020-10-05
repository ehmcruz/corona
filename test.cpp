#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{

}

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK;

	this->n_regions = 1;
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
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);
		normal_double_dist_t dist_work_size(5.0, 2.0, 1.0, 10.0);
		normal_double_dist_t dist_number_random_connections(5.0, 5.0, 5.0, 100.0);

		//this->create_families(dist_family_size);
		//this->create_work_relations(dist_work_size, 0, 100, 1.0, 0.5);
//network_print_population_graph();
//exit(1);
		//this->create_random_connections(dist_number_random_connections);

	#if 0
		normal_double_dist_t dist_school_class_size(5.0, 1.0, 2.0, 50.0);
		std::vector<region_double_pair_t> school;

		school.push_back( {this, 0.8} );
		
		network_create_school_relation(school, 20, 20, dist_school_class_size);
	#else

//boost::write_graphviz(std::cout, graph);
//exit(1);
		auto e1 = network_create_edge(population[0], population[1], RELATION_TRAVEL);
		auto e2 = network_create_edge(population[0], population[1], RELATION_BUDDY, true);
		//network_create_edge(population[0], population[1], RELATION_BUDDY, false);

network_print_population_graph();

	std::bitset<NUMBER_OF_FLAGS> mask;
	mask.set(RELATION_TRAVEL);
	mask.set(RELATION_BUDDY);

uint32_t count;
//network_delete_edges_by_type(mask, &count);

network_delete_edge(population[0], population[1]);
//std::cout << e1 << std::endl << e2 << std::endl;
//exit(1);

		network_print_population_graph();

		CMSG("deleted " << count << " edges" << std::endl);
		exit(1);


		std::vector<person_t*> students;
		uint32_t age_ini = 20;
		uint32_t age_end = 20;
		double school_ratio = 0.8;
		normal_double_dist_t dist_school_class_size(5.0, 1.0, 3.0, 20.0);
		const_double_dist_t dist_school_size(25.0);

		struct school_per_age_t {
			uint32_t age;
			uint32_t n, i;
		};

		std::vector<school_per_age_t> v;
		v.resize(age_end+1);

		for (uint32_t age=age_ini; age<=age_end; age++) {
			v[age].n = (uint32_t)((double)this->get_region_people_per_age(age) * school_ratio);
			v[age].age = age;
			v[age].i = 0;
		}

		students.reserve(this->people.size());

		std::vector<person_t*> people_ = this->people;

		std::shuffle(people_.begin(), people_.end(), rgenerator);

		for (person_t *p: people_) {
			uint32_t age = p->get_age();

			if (age >= age_ini && age <= age_end && v[age].i < v[age].n) {
				students.push_back(p);
				v[age].i++;
			}
		}

		const_double_dist_t dist_school_prof_age(21.0);

		network_create_school_relation(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          1.0,
                                          0.1,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL_4,
                                          nullptr);
	#endif
	}

	network_print_population_graph( {RELATION_SCHOOL} );
	panic("out\n");
}

void setup_inter_region_relations ()
{
#if 0
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
#endif
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
