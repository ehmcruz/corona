#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

static csv_ages_t *csv;

static health_unit_t santa_casa_uti(100000, ST_CRITICAL);
static health_unit_t santa_casa_enfermaria(20000000, ST_SEVERE);

static int32_t stages_green = 0;

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = 540.0;
	//this->r0 = 10.0;

	csv = new csv_ages_t((char*)"data/distribuicao-etaria-paranavai.csv");
	csv->dump();

	this->n_regions = csv->get_ncities();

	cprintf("number of cities: %u\n", this->n_regions);
}

void region_t::setup_population ()
{
	uint32_t i, n;

	uint32_t reported_deaths_per_age[AGE_CATS_N] = { // deaths according to the "Boletim da Pandemia"
		2,         // 0-9
		2,         // 10-19
		9,         // 20-29
		40,        // 30-39
		59,        // 40-49
		104,       // 50-59
		198,       // 60-69
		211,       // 70-79
		176,       // 80-89
		48         // 90+
	};

	std::string name;

	name = csv->get_city_name( this->get_id() );

	this->set_name(name);

	this->set_population_number( csv->get_population(this->get_name()) );

	cprintf("%s total de habitantes: " PU64 "\n", this->get_name().c_str(), this->get_npopulation());

	for (i=csv->get_first_age(); i<=csv->get_last_age(); i++) {
		n = csv->get_population_per_age(this->get_name(), i);
		//cprintf("%s habitantes idade %i -> %i\n", this->get_name().c_str(), i, n);
		this->add_people(n, i);
	}
	//exit(1);

	this->adjust_population_infection_state_rate_per_age(reported_deaths_per_age);
}

void region_t::setup_health_units ()
{
	this->add_health_unit( &santa_casa_uti );
	this->add_health_unit( &santa_casa_enfermaria );
}

void region_t::setup_relations ()
{
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);
		normal_double_dist_t dist_number_random_connections(20.0, 5.0, 5.0, 100.0);

		this->create_families(dist_family_size);
		this->create_random_connections(dist_number_random_connections);
	}
}

void setup_inter_region_relations ()
{
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		for (auto it=regions.begin(); it!=regions.end(); ++it) {
			for (auto jt=it+1; jt!=regions.end(); ++jt) {
				region_t *s, *t;
				uint64_t sn, tn;

				s = *it;
				t = *jt;

				sn = (uint64_t)((double)s->get_npopulation() * 0.01);
				tn = (uint64_t)((double)t->get_npopulation() * 0.01);

				if (tn < sn)
					sn = tn;

				cprintf("%s-%s " PU64 "\n", s->get_name().c_str(), t->get_name().c_str(), sn);
				
				network_create_inter_city_relation(s, t, sn);
			}
		}
	}
//exit(1);
}

void setup_extra_relations ()
{
	std::vector<region_double_pair_t> school;
	uint32_t i;

	for (region_t *r: regions) {
		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);

		school.clear();

		school.push_back( {r, 0.15} );
		
		for (i=0; i<6; i++)
			network_create_school_relation(school, 4, 18, dist_school_class_size, 0.25, 0.02);

		school.clear();

		school.push_back( {r, 0.2} );

		network_create_school_relation(school, 19, 23, dist_school_class_size, 0.25, 0.02);
	}
}

void callback_before_cycle (double cycle)
{
	static double backup;

	if (cycle == 0.0) {
		region_t::get("Paranavai")->pick_random_person()->force_infect();

		backup = cfg.relation_type_transmit_rate[RELATION_SCHOOL];
		cfg.relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;

		stages_green++;
	}
	else if (cycle == 30.0) {
		cfg.global_r0_factor = 1.1 / cfg.r0;
		stages_green++;
	}
	else if (cycle == 120.0) {
//		cfg.relation_type_transmit_rate[RELATION_SCHOOL] = backup;
		stages_green++;
	}
}

void callback_after_cycle (double cycle)
{

}

void callback_end ()
{
	C_ASSERT(stages_green == 3)
}
