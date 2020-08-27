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

static void sp_reconfigure_class_room (std::vector<person_t*>& school_people, std::vector<person_t*>::iterator& it_begin, std::vector<person_t*>::iterator& it_end, uint32_t& rm)
{
//	DMSG("sp_reconfigure_class_room: " << (network_vertex_data(*it_begin).school_class_room) << " students " << (it_end - it_begin) << std::endl)

	for (auto ita=it_begin; ita!=it_end; ++ita) {
		for (auto itb=ita+1; itb!=it_end; ++itb) {
			if (network_check_if_people_are_neighbors(*ita, *itb)) {
				network_delete_edge(*ita, *itb);
				rm++;
//				DMSG("delete edge between " << ((*ita)->get_id()) << " and " << ((*itb)->get_id()) << std::endl)
			}
		}
	}
}

void sp_configure_school ()
{
	// first find how many school people we have

	uint32_t n_school_people = 0;

	for (person_t *p: population) {
		if (network_vertex_data(p).school_class_room != UNDEFINED32) {
			n_school_people++;
		}
	}

	if (n_school_people < 2)
		return;

	DMSG("found " << n_school_people << " students" << std::endl)

	// now create a vector with all school people

	std::vector<person_t*> school_people;
	
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

	auto it = school_people.begin() + 1;
	auto it_begin = school_people.begin();

cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 2.0 * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	uint32_t rm = 0;

	while (true) {
		if (it == school_people.end()) {
			sp_reconfigure_class_room(school_people, it_begin, it, rm);
			break;
		}
		else if (network_vertex_data(*it).school_class_room != network_vertex_data(*it_begin).school_class_room) {
			sp_reconfigure_class_room(school_people, it_begin, it, rm);
			it_begin = it;
		}

		++it;
	}

	DMSG("removed " << rm << " school relations" << std::endl)

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
exit(1);
}

static std::deque<person_t*> sp_professors;

struct sp_school_room_t {
	std::vector<person_t*> students;
	uint32_t size;
	uint32_t i;
};

static person_t* sp_create_school_relation_v2_professor (std::vector<person_t*>& students,
                                                         region_t *prof_region,
                                                         dist_double_t& dist_prof_age)
{
	do {
		uint32_t prof_age = (uint32_t)dist_prof_age.generate();

		for (person_t *prof: prof_region->get_people()) {
			if (prof->get_age() == prof_age && network_vertex_data(prof).flags.test(RELATION_SCHOOL) == false) {
				network_vertex_data(prof).flags.set(VFLAG_PROFESSOR);
				network_create_connection_one_to_all(prof, students, RELATION_SCHOOL);
				
				return prof;
			}
		}
	} while (true);

	return nullptr;
}

void sp_create_school_relation_v2 (std::vector<person_t*>& students,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist_class_size,
                                     dist_double_t& dist_school_size,
                                     region_t *prof_region,
                                     dist_double_t& dist_prof_age,
                                     double intra_class_ratio,
                                     double inter_class_ratio,
                                     report_progress_t *report)
{
	std::vector<person_t*> school_students;

	uint32_t school_i = 0;
	uint32_t school_size = (uint32_t)dist_school_size.generate();

	std::vector<sp_school_room_t> class_students;
	
	school_students.resize( (uint32_t)dist_school_size.get_max(), nullptr );

	C_ASSERT(school_size <= school_students.size())
	
	class_students.resize( age_end + 1 );

	dprintf("total amount of students to go to school: " PU64 "\n", students.size());

	for (uint32_t age=age_ini; age<=age_end; age++) {
		class_students[age].students.resize( (uint32_t)dist_class_size.get_max(), nullptr );
		class_students[age].size = (uint32_t)dist_class_size.generate();
		class_students[age].i = 0;

		C_ASSERT(class_students[age].size <= class_students[age].students.size())

		//for (uint32_t i=0; i<class_students[age].size(); i++)
		//	class_students[age][i] = nullptr;

		//class_ocupancy[age].first = 0;     // target amount of students
		//class_ocupancy[age].second = 0;    // current amount of students
	}


	for (person_t *p: students) {
		uint32_t age = p->get_age();

		C_ASSERT(age >= age_ini && age <= age_end)

		C_ASSERT(class_students[age].i < class_students[age].size)
		C_ASSERT(school_i < school_size)

		C_ASSERT(class_students[age].i < class_students[age].students.size())
		C_ASSERT(school_i < school_students.size())

		class_students[age].students[ class_students[age].i++ ] = p;
		school_students[ school_i++ ] = p;

		if (report != nullptr)
			report->check_report(1);

		if (class_students[age].i == class_students[age].size || school_i == school_size) {
		#ifdef SANITY_ASSERT
		{
			bool was_null = false;
			for (auto p: class_students[age].students) {
				if (was_null == false) {
					was_null = (p == nullptr);
				}
				else {
					C_ASSERT(p == nullptr)
				}
			}
		}
		#endif

//			dprintf("created class room with %u students\n", class_students[age].i);

			network_create_connection_between_people(class_students[age].students, RELATION_SCHOOL, intra_class_ratio);

			for (auto it=class_students[age].students.begin(); it!=class_students[age].students.end(); ++it) {
				person_t *p = *it;
				if (p == nullptr)
					break;

				network_vertex_data(p).school_class_room = sp_professors.size();
			}

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("       %.4f\n", intra_class_ratio);
panic("in\n");
#endif

			person_t *prof = sp_create_school_relation_v2_professor(class_students[age].students, prof_region, dist_prof_age);

			network_vertex_data(prof).school_class_room = sp_professors.size();

			sp_professors.push_back(prof);

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("\n");
panic("in\n");
#endif

			for (auto& p: class_students[age].students)
				p = nullptr;

			class_students[age].size = (uint32_t)dist_class_size.generate();
			class_students[age].i = 0;

			C_ASSERT(class_students[age].size <= class_students[age].students.size())

			if (school_i == school_size) {
//				dprintf("created school with %u students\n", school_i);
				network_create_connection_between_people(school_students, RELATION_SCHOOL, inter_class_ratio);

				for (auto& p: school_students)
					p = nullptr;

				school_i = 0;
				school_size = (uint32_t)dist_school_size.generate();

				C_ASSERT(school_size <= school_students.size())
			}
		}
	}

	for (uint32_t age=age_ini; age<=age_end; age++) {
		if (class_students[age].i > 0) {
//			dprintf("created class room with %u students\n", class_students[age].i);
			for (person_t *p: class_students[age].students) {
				if (p == nullptr)
					break;
				network_vertex_data(p).school_class_room = sp_professors.size();
			}

			network_create_connection_between_people(class_students[age].students, RELATION_SCHOOL, intra_class_ratio);
			
			person_t *prof = sp_create_school_relation_v2_professor(class_students[age].students, prof_region, dist_prof_age);

			network_vertex_data(prof).school_class_room = sp_professors.size();

			sp_professors.push_back(prof);
		}
	}

	if (school_i > 0) {
		//dprintf("created school with %u students\n", school_i);
		network_create_connection_between_people(school_students, RELATION_SCHOOL, inter_class_ratio);
	}
}

/**********************************************************************/

void setup_cmd_line_args (boost::program_options::options_description& cmd_line_args)
{

}

void cfg_t::scenery_setup ()
{
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
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		dprintf("setting up %s relations...\n", this->get_name().c_str());

		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);


		normal_double_dist_t dist_number_random_connections(20.0, 5.0, 5.0, 100.0);

		this->create_families(dist_family_size);

		std::string rname(this->get_name());
		rname += " random loading...";
		report_progress_t progress_random(rname.c_str(), this->get_npopulation(), 10000);

		this->create_random_connections(dist_number_random_connections, RELATION_UNKNOWN, &progress_random);
//return;
	#if 0
		std::vector<region_double_pair_t> school;
		uint32_t i, n_schools, age;
		uint64_t n_students, school_max_students = 2000;
		double school_ratio;

		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);

		school_ratio = 0.9;
		n_students = 0;
		for (age=4; age<=18; age++) {
			n_students += (uint64_t)((double)this->get_region_people_per_age(age) * school_ratio);
		}

		n_schools = n_students / school_max_students;

		n_schools++;

		school.push_back( {this, school_ratio / (double)n_schools} );

		printf("%s n_schools:%u school_ratio_per_school:%.2f students_per_school:" PU64 "\n",
		       this->get_name().c_str(),
		       n_schools,
		       school_ratio / (double)n_schools,
		       (uint64_t)((school_ratio / (double)n_schools)*(double)n_students));

		normal_double_dist_t dist_school_prof_age(40.0, 10.0, 25.0, 70.0);

		for (i=0; i<n_schools; i++)
			network_create_school_relation(school, 4, 18, dist_school_class_size, this, dist_school_prof_age, 0.2, 0.001);

		school.clear();

		school.push_back( {this, 0.2} );

		network_create_school_relation(school, 19, 23, dist_school_class_size, this, dist_school_prof_age, 0.2, 0.002);
	#else
		std::vector<person_t*> students;
		uint32_t age_ini = 4;
		uint32_t age_end = 18;
		double school_ratio = 0.9;
		normal_double_dist_t dist_school_class_size(30.0, 5.0, 10.0, 50.0);
		const_double_dist_t dist_school_size(2000.0);

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

		// network_create_school_relation_v2
		sp_create_school_relation_v2(students,
		                                  age_ini,
		                                  age_end,
		                                  dist_school_class_size,
                                          dist_school_size,
                                          this,
                                          dist_school_prof_age,
                                          0.2,
                                          0.003,
                                          &progress_school);
	#endif
	}
}

void setup_inter_region_relations ()
{
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
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
	if (cfg->network_type == NETWORK_TYPE_NETWORK) {
		stats_zone_t *zone = create_new_stats_zone();
		zone->get_name() = "school";

		for (person_t *p: population) {
			if (network_vertex_data(p).flags.test(RELATION_SCHOOL))
				zone->add_person(p);
		}
	}
}

//static double backup_school_r0;

static void adjust_r_no_school (double target_r0)
{
	double family_r0, unknown_r0, factor;

	//backup_school_r0 = cfg->relation_type_transmit_rate[RELATION_SCHOOL];
	cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;

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
	printf("r0 cycle %.2f-student: %.2f (fast %.2f)\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ), get_affective_r0_fast( {RELATION_SCHOOL} ));

	cfg->relation_type_transmit_rate[RELATION_SCHOOL] = 2.0 * cfg->relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f (fast %.2f)\n", current_cycle, get_affective_r0(), get_affective_r0_fast());
	printf("r0 cycle %.2f-student: %.2f (fast %.2f)\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ), get_affective_r0_fast( {RELATION_SCHOOL} ));
}

void callback_before_cycle (double cycle)
{
//	static double backup;

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
		sp_configure_school();
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

	C_ASSERT(stages_green == 4)

	n_profs = get_n_population_per_relation_flag( {VFLAG_PROFESSOR} );
	n_students = get_n_population_per_relation_flag( {RELATION_SCHOOL} );

	cprintf("amount of professors: " PU64 "\n", n_profs);
	cprintf("amount of students: " PU64 "\n", n_students);

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		cprintf("relation-%s: " PU64 "\n", relation_type_str(i), cfg->relation_type_number[i]);
	}

	cprintf("amount of school relations per student: %.2f\n", (double)cfg->relation_type_number[RELATION_SCHOOL] / (double)n_students);
}
