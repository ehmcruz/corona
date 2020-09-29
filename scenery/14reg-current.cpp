#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

static csv_ages_t *csv;

//static health_unit_t santa_casa_uti(100000, ST_CRITICAL);
//static health_unit_t santa_casa_enfermaria(20000000, ST_SEVERE);

static health_unit_t santa_casa_uti(10, ST_CRITICAL);
static health_unit_t santa_casa_enfermaria(20, ST_SEVERE);

static int32_t stages_green = 0;

/**********************************************************************/

void check_vaccine (double cycle);

#define X_VACCINE_STRATEGY   \
	X_VACCINE_STRATEGY_(none) \
	X_VACCINE_STRATEGY_(random) \
	X_VACCINE_STRATEGY_(priorities)

enum class vaccine_plan_t {
	#define X_VACCINE_STRATEGY_(S) S,
	X_VACCINE_STRATEGY
	#undef X_VACCINE_STRATEGY_
};

static vaccine_plan_t vaccine_plan = vaccine_plan_t::random;

static double vaccine_cycle = 30.0; // default no vaccine

static double vaccine_immune_cycles = 14.0;

static double vaccine_immunity_rate = 0.9;

static uint32_t vaccine_per_cycle = 200;

static double sp_ratio_student_intra_class_contingency = 0.5;

enum class sp_plan_t {
	phase_0,
	phase_33,
	phase_66,
	phase_100
};

static const char* sp_plan_phase_str (sp_plan_t phase)
{
	static const char *list[] = {
		"phase_0",
		"phase_33",
		"phase_66",
		"phase_100"
	};

	C_ASSERT(static_cast<uint32_t>(phase) < 4)

	return list[ static_cast<uint32_t>(phase) ];
}

static sp_plan_t sp_plan = sp_plan_t::phase_0;

/*****************************/

static std::vector<person_t*> school_people;
static std::vector< std::pair<person_t*, person_t*> > original_school_relations;

static void sp_reconfigure_class_room_phase_33 (std::vector<person_t*>::iterator& it_begin, std::vector<person_t*>::iterator& it_end)
{
	uint32_t n_students = it_end - it_begin - 1; // 1 professor

//	DMSG("TAG0: sp_reconfigure_class_room phase 33: " << (network_vertex_data(*it_begin).school_class_room) << " students " << n_students << std::endl)

	// find professor

	std::vector<person_t*>::iterator it_prof = school_people.end();

	for (auto it=it_begin; it!=it_end; ++it) {
		if (network_vertex_data(*it).flags.test(VFLAG_PROFESSOR))
			it_prof = it;
	}

	C_ASSERT(it_prof != school_people.end())

	// move professor to the head

	person_t *tmp = *it_begin;
	person_t *prof = *it_prof;

	*it_begin = prof;
	*it_prof = tmp;

	C_ASSERT(*it_begin == prof)

	++it_begin;

	// if the class room is already small, no need to divide
	if (n_students < 10) {
		network_create_connection_between_people(it_begin, it_end, RELATION_SCHOOL, sp_ratio_student_intra_class_contingency);

		network_create_connection_one_to_all(prof, it_begin, it_end, RELATION_SCHOOL);

		return;
	}

	// re-create connections between students

	uint32_t new_room_size = n_students / 3;

	network_create_connection_between_people(it_begin, it_begin+new_room_size, RELATION_SCHOOL_0, sp_ratio_student_intra_class_contingency);
	network_create_connection_between_people(it_begin+new_room_size, it_begin+new_room_size*2, RELATION_SCHOOL_1, sp_ratio_student_intra_class_contingency);
	network_create_connection_between_people(it_begin+new_room_size*2, it_end, RELATION_SCHOOL_2, sp_ratio_student_intra_class_contingency);

	// re-create connections for professor

	network_create_connection_one_to_all(prof, it_begin, it_begin+new_room_size, RELATION_SCHOOL_0);
	network_create_connection_one_to_all(prof, it_begin+new_room_size, it_begin+new_room_size*2, RELATION_SCHOOL_1);
	network_create_connection_one_to_all(prof, it_begin+new_room_size*2, it_end, RELATION_SCHOOL_2);
}

static void sp_reconfigure_class_room_phase_66 (std::vector<person_t*>::iterator& it_begin, std::vector<person_t*>::iterator& it_end)
{
	static std::vector<person_t*> last_room;
	uint32_t n_students = it_end - it_begin - 1; // 1 professor
	uint32_t i;

//	DMSG("TAG0: sp_reconfigure_class_room phase 66: " << (network_vertex_data(*it_begin).school_class_room) << " students " << n_students << std::endl)

	// find professor

	std::vector<person_t*>::iterator it_prof = school_people.end();

	for (auto it=it_begin; it!=it_end; ++it) {
		if (network_vertex_data(*it).flags.test(VFLAG_PROFESSOR))
			it_prof = it;
	}

	C_ASSERT(it_prof != school_people.end())

	// move professor to the head

	person_t *tmp = *it_begin;
	person_t *prof = *it_prof;

	*it_begin = prof;
	*it_prof = tmp;

	C_ASSERT(*it_begin == prof)

	++it_begin;

	// if the class room is already small, no need to divide
	if (n_students < 10) {
		network_create_connection_between_people(it_begin, it_end, RELATION_SCHOOL, sp_ratio_student_intra_class_contingency);

		network_create_connection_one_to_all(prof, it_begin, it_end, RELATION_SCHOOL);

		return;
	}

	// re-create connections between students

	uint32_t div = n_students / 3;

	i = 0;

	for (auto it=it_begin+div*2; it!=it_end; ++it) {
		if (i >= last_room.size())
			last_room.resize((last_room.size()+1) * 2); // increase the vector size exponentially

		C_ASSERT(i < last_room.size())

		last_room[i] = *it;
	}

	for (auto it=it_begin+1; it!=it_begin+div; ++it) {
		if (i >= last_room.size())
			last_room.resize((last_room.size()+1) * 2); // increase the vector size exponentially

		C_ASSERT(i < last_room.size())

		last_room[i] = *it;
	}

	network_create_connection_between_people(it_begin, it_begin+div*2, RELATION_SCHOOL_0, sp_ratio_student_intra_class_contingency);

	network_create_connection_between_people(it_begin+div, it_end, RELATION_SCHOOL_1, sp_ratio_student_intra_class_contingency);

	network_create_connection_between_people(last_room.begin(), last_room.end(), RELATION_SCHOOL_2, sp_ratio_student_intra_class_contingency);

	// re-create connections for professor

	network_create_connection_one_to_all(prof, it_begin, it_begin+div*2, RELATION_SCHOOL_0);
	network_create_connection_one_to_all(prof, it_begin+div, it_end, RELATION_SCHOOL_1);
	network_create_connection_one_to_all(prof, last_room.begin(), last_room.end(), RELATION_SCHOOL_2);
}

static void sp_reconfigure_class_room (sp_plan_t phase, std::vector<person_t*>::iterator& it_begin, std::vector<person_t*>::iterator& it_end)
{
	switch (phase) {
		case sp_plan_t::phase_33:
			sp_reconfigure_class_room_phase_33(it_begin, it_end);
		break;

		case sp_plan_t::phase_66:
			sp_reconfigure_class_room_phase_66(it_begin, it_end);
		break;

		case sp_plan_t::phase_100:
			C_ASSERT(0)
		break;

		default:
			C_ASSERT(0)
	}
}

void sp_backup_original_school_intra_class_relations ()
{
	uint32_t n = 0;

	network_iterate_over_edges ([&n] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		n += (network_edge_data(e).type == RELATION_SCHOOL);
	});

	original_school_relations.reserve(n);

	network_iterate_over_edges ([] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		if (network_edge_data(e).type == RELATION_SCHOOL) {
			original_school_relations.push_back( std::make_pair( network_vertex_data(s).p, network_vertex_data(t).p ) );
		}
	});

	DMSG("TAG0: backed up " << n << " original intra-class relations" << std::endl)
}

void sp_configure_school (sp_plan_t phase)
{
	DMSG("TAG0:" << std::endl)
	DMSG("TAG0: cycle " << current_cycle << " reconfiguring schools for phase " << sp_plan_phase_str(phase) << std::endl)

	if (phase == sp_plan_t::phase_33) {
		// first find how many school people we have

		uint32_t n_school_people = 0;

		for (person_t *p: population) {
			if (network_vertex_data(p).school_class_room != UNDEFINED32)
				n_school_people++;
		}

		if (n_school_people < 2)
			return;

		DMSG("TAG0: found " << n_school_people << " students" << std::endl)

		// now create a vector with all school people
		
		school_people.reserve(n_school_people);

		for (person_t *p: population) {
			if (network_vertex_data(p).school_class_room != UNDEFINED32)
				school_people.push_back(p);
		}

		C_ASSERT(school_people.size() == n_school_people)

		// now sort people acording to their class room

		std::sort(school_people.begin(), school_people.end(), [] (person_t *a, person_t *b) -> bool {
			return (network_vertex_data(a).school_class_room < network_vertex_data(b).school_class_room);
		});

		// now identify each class room

	//	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
	//		cfg->relation_type_transmit_rate[r] = 2.0 * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

//		printf("TAG0: r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
	//	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
	}

	// count school relations

	uint32_t rm, count_intra_class, count_other_rooms, total_inter_class;

	std::bitset<NUMBER_OF_FLAGS> mask_intra_class;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_2; r++)
		mask_intra_class.set(r);
	
	count_intra_class = 0;
	count_other_rooms = 0;
	network_iterate_over_edges ([&count_intra_class, &count_other_rooms, &mask_intra_class] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count_intra_class += (mask_intra_class.test(network_edge_data(e).type));
		count_other_rooms += (network_edge_data(e).type == RELATION_SCHOOL_4);
	});

	DMSG("TAG0: there are " << count_intra_class << " intra-class school relations, with other rooms: " << count_other_rooms << std::endl)

	// now we remove all intra-class edges

	DMSG("TAG0: removing intra-class relations ..." << std::endl)

	network_delete_edges_by_type(mask_intra_class, &rm);

	DMSG("TAG0: removed " << rm << " intra-class relations" << std::endl)

	// now restore the original inter class relations

	count_other_rooms = 0;
	total_inter_class = 0;
	network_iterate_over_edges ([&count_other_rooms, &total_inter_class] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		if (network_edge_data(e).type == RELATION_SCHOOL_FOO) {
			network_edge_data(e).type = RELATION_SCHOOL_4;
			count_other_rooms++;
		}
		total_inter_class += (network_edge_data(e).type == RELATION_SCHOOL_4);
	});

	DMSG("TAG0: restored " << count_other_rooms << " inter-class relations total=" << total_inter_class << std::endl)

	// now we recreate class relations

	switch (phase) {
		case sp_plan_t::phase_33:
		case sp_plan_t::phase_66: {
			auto it = school_people.begin() + 1;
			auto it_begin = school_people.begin();

			// reconfigure class rooms

			while (true) {
				if (it == school_people.end()) {
					sp_reconfigure_class_room(phase, it_begin, it);
					break;
				}
				else if (network_vertex_data(*it).school_class_room != network_vertex_data(*it_begin).school_class_room) {
					sp_reconfigure_class_room(phase, it_begin, it);
					it_begin = it;
				}

				++it;
			}
		}
		break;

		case sp_plan_t::phase_100:
			DMSG("TAG0: restoring original intra-class relations..." << std::endl)

			for (auto& pair: original_school_relations) {
				network_create_edge(pair.first, pair.second, RELATION_SCHOOL);
			}
		break;

		default:
			C_ASSERT(0)
	}

	double pcent_to_rm = 0.0;

	switch (phase) {
		case sp_plan_t::phase_33:
			pcent_to_rm = 0.66;
		break;

		case sp_plan_t::phase_66:
			pcent_to_rm = 0.33;
		break;

		case sp_plan_t::phase_100:
			pcent_to_rm = 0.0;
		break;

		default:
			C_ASSERT(0)
	}

	if (pcent_to_rm > 0.0) {
		rm = 0;

		network_iterate_over_edges ([&rm, pcent_to_rm] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
			if (network_edge_data(e).type == RELATION_SCHOOL_4 && roll_dice(pcent_to_rm)) {
				network_edge_data(e).type = RELATION_SCHOOL_FOO;
				rm++;
			}
		});

		DMSG("TAG0: removed " << rm << " inter-class relations" << std::endl)
	}

	cfg->relation_type_transmit_rate[RELATION_SCHOOL_FOO] = 0.0;

	count_intra_class = 0;
	count_other_rooms = 0;
	network_iterate_over_edges ([&count_intra_class, &count_other_rooms, &mask_intra_class] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count_intra_class += (mask_intra_class.test(network_edge_data(e).type));
		count_other_rooms += (network_edge_data(e).type == RELATION_SCHOOL_4);
	});

	DMSG("TAG0: after re-organizing, there are " << count_intra_class << " intra-class school relations, with other rooms: " << count_other_rooms << std::endl)

//	printf("TAG0: r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0_fast());
	//printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
//exit(1);
}


/**********************************************************************/

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{

}

void cfg_t::scenery_setup ()
{
	//this->network_type = NETWORK_TYPE_NETWORK_SIMPLE;
	this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = 540.0;
	this->relation_type_weights[RELATION_SCHOOL] = 2.0;
	this->cycle_division = 1.0;
	//this->cycles_pre_symptomatic = 3.0;
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

	if (this->get_name() == "Paranavai")
		this->track_stats();
}

void region_t::setup_health_units ()
{
	this->add_health_unit( &santa_casa_uti );
	this->add_health_unit( &santa_casa_enfermaria );
}

void region_t::setup_relations ()
{
	if (check_if_network()) {
		dprintf("setting up %s relations...\n", this->get_name().c_str());

		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);


		normal_double_dist_t dist_number_random_connections(20.0, 5.0, 5.0, 100.0);

		this->create_families(dist_family_size);

		std::string rname(this->get_name());
		rname += " random loading...";
		report_progress_t progress_random(rname.c_str(), this->get_npopulation(), 10000);

		this->create_random_connections(dist_number_random_connections, RELATION_UNKNOWN, &progress_random);
//return;
		std::vector<person_t*> students;
		uint32_t age_ini = 4;
		uint32_t age_end = 18;
		double school_ratio = 0.9;

//		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);
//		const_double_dist_t dist_school_size(2000.0);

		gamma_double_dist_t dist_school_size(438.9, 449.3, 200.0, 5000.0);
		gamma_double_dist_t dist_school_class_size(22.1, 10.5, 10.0, 50.0);


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

		normal_double_dist_t dist_school_prof_age(40.0, 10.0, 25.0, 70.0);

		rname = this->get_name();
		rname += " school loading...";
		report_progress_t progress_school(rname.c_str(), students.size(), 10000);

		/*
			30*(30/2) * p = 7*30;
			15p = 7
			p = 0.47 ~ 0.5

			2000*(2000/2) * p = 2*2000;
			1000p = 2
			p = 0.002
		*/

		network_create_school_relation(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          0.5,
                                          0.005,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL,
                                          RELATION_SCHOOL_4,
                                          &progress_school);
	}
}

void setup_inter_region_relations ()
{
	if (check_if_network()) {
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
				
				network_create_inter_city_relation(s, t, sn, RELATION_UNKNOWN);
			}
		}
	}
//exit(1);
}

void setup_extra_relations ()
{
	if (check_if_network()) {
		stats_zone_t *zone = create_new_stats_zone();
		zone->get_name() = "school";

		for (person_t *p: population) {
			if (network_vertex_data(p).flags.test(RELATION_SCHOOL))
				zone->add_person(p);
		}

		zone = create_new_stats_zone();
		zone->get_name() = "professors";

		for (person_t *p: population) {
			if (network_vertex_data(p).flags.test(VFLAG_PROFESSOR))
				zone->add_person(p);
		}

		sp_backup_original_school_intra_class_relations();
	}
}

//static double backup_school_r0;

static void adjust_r_no_school (double target_r0)
{
	double family_r0, unknown_r0, factor;

	//backup_school_r0 = cfg->relation_type_transmit_rate[RELATION_SCHOOL];
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		cfg->relation_type_transmit_rate[r] = 0.0;

	//target_r0 /= cfg->cycles_contagious;
	//target_r0 *= (double)population.size();

	/*
	f*others + family = target
	f*others = (target-family)
	*/

	family_r0 = cfg->relation_type_transmit_rate[RELATION_FAMILY] * (double)cfg->relation_type_number[RELATION_FAMILY];
	family_r0 *= cfg->cycles_contagious->get_expected();
	family_r0 *= cfg->cycle_division;
	family_r0 /= (double)population.size();

	unknown_r0 = cfg->relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg->relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg->cycles_contagious->get_expected();
	unknown_r0 *= cfg->cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f family_r0: %.2f\n", current_cycle, family_r0);
	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);
	
	factor = (target_r0 - family_r0) / unknown_r0;

	cfg->relation_type_transmit_rate[RELATION_UNKNOWN] *= factor;

	unknown_r0 = cfg->relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg->relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg->cycles_contagious->get_expected();
	unknown_r0 *= cfg->cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);

	DMSG("r0 cycle " << current_cycle << ": " << get_affective_r0() << " (fast " << get_affective_r0_fast() << ")" << std::endl);
//exit(1);
}

static void adjust_r_open_schools ()
{
	printf("r0 cycle %.2f: %.2f (fast %.2f)\n", current_cycle, get_affective_r0(), get_affective_r0_fast());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 2.0 * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f (fast %.2f)\n", current_cycle, get_affective_r0(), get_affective_r0_fast());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
}

void callback_before_cycle (double cycle)
{
//	static double backup;
	check_vaccine(cycle);

	if (cycle == 0.0) {
		region_t::get("Paranavai")->pick_random_person()->force_infect();
printf("r0 cycle 0: %.2f\n", get_affective_r0());

printf("r0 cycle 0-student: %.2f\n", get_affective_r0( {RELATION_SCHOOL} ));
		stages_green++;
	}
	else if (cycle == 30.0) {
//{ person_t *p; do { p = region_t::get("Paranavai")->pick_random_person(); if (p->get_state() == ST_HEALTHY) { p->force_infect(); break;} } while (1); }
		cfg->global_r0_factor = 1.05;
		adjust_r_no_school(1.0);
//		backup = cfg->relation_type_transmit_rate[RELATION_SCHOOL];
//		cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;
//		cfg->global_r0_factor = 0.9 / (network_get_affective_r0_fast() / cfg->global_r0_factor);
		//cfg->global_r0_factor = 0.9 / cfg->r0;
//printf("r0 cycle 30: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (cycle == 51.0) {
		adjust_r_no_school(1.3);
		//cfg->global_r0_factor = 1.15 / (network_get_affective_r0_fast() / cfg->global_r0_factor);
		//cfg->global_r0_factor = 1.16 / cfg->r0;
		//cfg->global_r0_factor = 1.16 / cfg->r0;
//printf("r0 cycle 51: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (cycle == 180.0) {
		//adjust_r_open_schools();
		sp_configure_school(sp_plan_t::phase_33);
		stages_green++;
	}
	else if (cycle == 181.0) {
		//adjust_r_open_schools();
		sp_configure_school(sp_plan_t::phase_66);
		stages_green++;
	}
	else if (cycle == 182.0) {
		//adjust_r_open_schools();
		sp_configure_school(sp_plan_t::phase_100);
		stages_green++;
	}
}

void callback_after_cycle (double cycle)
{

}

void callback_end ()
{
	uint32_t i;
	uint64_t n_students, n_profs;

//	C_ASSERT(stages_green == 4)

	n_profs = get_n_population_per_relation_flag( {VFLAG_PROFESSOR} );

	std::bitset<NUMBER_OF_FLAGS> mask;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		mask.set(r);

	n_students = get_n_population_per_relation_flag(mask);

	cprintf("amount of professors: " PU64 "\n", n_profs);
	cprintf("amount of students: " PU64 "\n", n_students);

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		cprintf("relation-%s: " PU64 "\n", relation_type_str(i), cfg->relation_type_number[i]);
	}

	uint64_t n_relations;

	n_relations = 0;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
		n_relations += cfg->relation_type_number[r];

	cprintf("amount of school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);

	n_relations = 0;
	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_3; r++)
		n_relations += cfg->relation_type_number[r];

	cprintf("amount of intra-class school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);

	n_relations = cfg->relation_type_number[RELATION_SCHOOL_4];

	cprintf("amount of inter-class school relations per student: %.2f  (students " PU64 "  rel " PU64 ")\n", (double)n_relations / (double)n_students, n_students, n_relations);

	uint32_t count_rel_students = 0;
	network_iterate_over_edges([&count_rel_students, &mask] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count_rel_students += (mask.test(network_edge_data(e).type)
			               && network_vertex_data(s).flags.test(VFLAG_PROFESSOR) == false
			               && network_vertex_data(t).flags.test(VFLAG_PROFESSOR) == false);
	});

	uint32_t count_rel_school = 0;
	network_iterate_over_edges([&count_rel_school, &mask] (pop_vertex_t s, pop_vertex_t t, pop_edge_t e) {
		count_rel_school += mask.test(network_edge_data(e).type);
	});

	uint32_t count_students = 0;
	network_iterate_over_vertices([&count_students, &mask] (pop_vertex_t v) {
		count_students += ((network_vertex_data(v).flags & mask).any() && network_vertex_data(v).flags.test(VFLAG_PROFESSOR) == false);
	});

	uint32_t count_school = 0;
	network_iterate_over_vertices([&count_school, &mask] (pop_vertex_t v) {
		count_school += (network_vertex_data(v).flags & mask).any();
	});

	double rel_per_student = static_cast<double>(count_rel_students) / static_cast<double>(count_students);
	double rel_per_school = static_cast<double>(count_rel_school) / static_cast<double>(count_school);

	DMSG("there are " << count_students << " students and " << count_rel_students << " students relations, relations per student: " << rel_per_student << std::endl)
	DMSG("there are " << count_school << " school people and " << count_rel_school << " school relations, relations per school people: " << rel_per_school << std::endl)
}

void check_vaccine (double cycle)
{
	if (vaccine_plan != vaccine_plan_t::none && cycle >= vaccine_cycle) {
		static bool first = false;
		static std::vector<person_t*>::iterator it;
		static std::vector<person_t*> pop;

		uint32_t i;

		if (!first) {
			first = true;

			switch (vaccine_plan) {
				case vaccine_plan_t::random:
					pop = population;
					std::shuffle(pop.begin(), pop.end(), rgenerator);
				break;

				case vaccine_plan_t::priorities: {
					static std::vector<person_t*> tmp = population;

					std::shuffle(tmp.begin(), tmp.end(), rgenerator);

					pop.reserve( population.size() );

					std::bitset<NUMBER_OF_FLAGS> mask_school;
					for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
						mask_school.set(r);

					// set that no one took the vaccine

					for (person_t *p: tmp)
						p->set_foo(0);

					// first to get the vaccine are the old

					for (person_t *p: tmp) {
						if (p->get_age() >= 50) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}

					// now the professors

					for (person_t *p: tmp) {
						if (p->get_foo() == 0 && (network_vertex_data(p).flags.test(VFLAG_PROFESSOR))) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}

					// now the students

					for (person_t *p: tmp) {
						if (p->get_foo() == 0 && (network_vertex_data(p).flags & mask_school).any()) {
							pop.push_back(p);
							p->set_foo(1);
						}
					}

					// now the rest

					for (person_t *p: tmp) {
						if (p->get_foo() == 0) {
							pop.push_back(p);
							//p->set_foo(1);
						}
					}
				}
				break;

				default:
					C_ASSERT(0)
			}

			C_ASSERT(pop.size() == population.size())

			it = pop.begin();
		}

		i = 0;
		while (it != pop.end()) {
			C_ASSERT(i <= vaccine_per_cycle)

			if (bunlikely(i == vaccine_per_cycle))
				break;

			person_t *p = *it;

			i += p->take_vaccine(vaccine_immunity_rate);

			++it;
		}
	}
}
