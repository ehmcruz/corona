#include <corona.h>
#include <parser.h>

#include <random>
#include <algorithm>

//#define REDUCED_SCHOOL

static csv_ages_t *csv;

//static health_unit_t santa_casa_uti(100000, ST_CRITICAL);
//static health_unit_t santa_casa_enfermaria(20000000, ST_SEVERE);

static health_unit_t uti(1000000000, ST_CRITICAL);
static health_unit_t enfermaria(1000000000, ST_SEVERE);

static int32_t stages_green = 0;

static uint32_t sp_school_div = 3;

void cfg_t::scenery_setup ()
{
	this->network_type = NETWORK_TYPE_NETWORK;
	this->cycles_to_simulate = 30.0 * 12.0;

//	for (uint32_t r=RELATION_SCHOOL; r<=RELATION_SCHOOL_4; r++)
//		this->relation_type_weights[r] = 2.0;

	this->r0 = 3.0;

	this->probability_asymptomatic = 0.87;
	this->probability_mild = 0.809 * (1.0 - this->probability_asymptomatic);
	this->probability_critical = 0.044 * (1.0 - this->probability_asymptomatic);

	csv = new csv_ages_t((char*)"data/sp-grande-sp.csv");
	csv->dump();

	this->n_regions = csv->get_ncities();
//	this->n_regions = 1;

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
	//name = "SaoPaulo";

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

//	if (this->get_name() == "Paranavai")
//		this->track_stats();
}

void region_t::setup_health_units ()
{
	this->add_health_unit( &uti );
	this->add_health_unit( &enfermaria );
}

void region_t::setup_relations ()
{
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		normal_double_dist_t dist_family_size(3.0, 1.0, 1.0, 10.0);
		normal_double_dist_t dist_number_random_connections(20.0, 5.0, 5.0, 100.0);

		dprintf("creating families for city %s...\n", this->get_name().c_str());
		this->create_families(dist_family_size);

		dprintf("creating random connections for city %s...\n", this->get_name().c_str());

		std::string rname(this->get_name());
		rname += " random loading...";
		report_progress_t progress_random(rname.c_str(), this->get_npopulation(), 10000);

		this->create_random_connections(dist_number_random_connections, RELATION_UNKNOWN, &progress_random);
//return;
		dprintf("creating schools for city %s...\n", this->get_name().c_str());

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

	#ifdef REDUCED_SCHOOL
	#else
		network_create_school_relation_v2(students,
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
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		for (auto it=regions.begin(); it!=regions.end(); ++it) {
			for (auto jt=it+1; jt!=regions.end(); ++jt) {
				region_t *s, *t;
				uint64_t sn, tn;

				s = *it;
				t = *jt;

				if (s->get_name() == "SaoPaulo") {
					tn = (uint64_t)((double)t->get_npopulation() * 0.05);
					sn = tn;
				}
				else if (t->get_name() == "SaoPaulo") {
					sn = (uint64_t)((double)s->get_npopulation() * 0.05);
					tn = sn;
				}
				else {
					sn = (uint64_t)((double)s->get_npopulation() * 0.01);
					tn = (uint64_t)((double)t->get_npopulation() * 0.01);
				}

				if (tn < sn)
					sn = tn;
				else
					tn = sn;

				cprintf("creating links between cities %s-%s " PU64 "...\n", s->get_name().c_str(), t->get_name().c_str(), sn);
				
				network_create_inter_city_relation(s, t, sn, RELATION_UNKNOWN);
			}
		}
	}
//exit(1);
}

void setup_extra_relations ()
{
	if (cfg.network_type == NETWORK_TYPE_NETWORK) {
		stats_zone_t *zone = create_new_stats_zone();
		zone->get_name() = "school";

		std::bitset<NUMBER_OF_FLAGS> mask;
		mask.set(RELATION_SCHOOL);
		mask.set(RELATION_SCHOOL_0);
		mask.set(RELATION_SCHOOL_1);
		mask.set(RELATION_SCHOOL_2);
		mask.set(RELATION_SCHOOL_3);
		mask.set(RELATION_SCHOOL_4);

		for (person_t *p: population) {
			if ((network_vertex_data(p).flags & mask).any())
				zone->add_person(p);
		}
	}
}

static double backup_school_r0;

static void adjust_r_no_school (double target_r0)
{
	double family_r0, unknown_r0, factor;

	backup_school_r0 = cfg.relation_type_transmit_rate[RELATION_SCHOOL];
	cfg.relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;

	//target_r0 /= cfg.cycles_contagious;
	//target_r0 *= (double)population.size();

	/*
	f*others + family = target
	f*others = (target-family)
	*/

	family_r0 = cfg.relation_type_transmit_rate[RELATION_FAMILY] * (double)cfg.relation_type_number[RELATION_FAMILY];
	family_r0 *= cfg.cycles_contagious->get_expected();
	family_r0 *= cfg.cycle_division;
	family_r0 /= (double)population.size();

	unknown_r0 = cfg.relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg.relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg.cycles_contagious->get_expected();
	unknown_r0 *= cfg.cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f family_r0: %.2f\n", current_cycle, family_r0);
	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);
	
	factor = (target_r0 - family_r0) / unknown_r0;

	cfg.relation_type_transmit_rate[RELATION_UNKNOWN] *= factor;

	unknown_r0 = cfg.relation_type_transmit_rate[RELATION_UNKNOWN] * (double)cfg.relation_type_number[RELATION_UNKNOWN];
	unknown_r0 *= cfg.cycles_contagious->get_expected();
	unknown_r0 *= cfg.cycle_division;
	unknown_r0 /= (double)population.size();

	printf("r0 cycle %.2f unknown_r0: %.2f\n", current_cycle, unknown_r0);

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
}

static void adjust_r_open_schools ()
{
	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));

	cfg.relation_type_transmit_rate[RELATION_SCHOOL] = 3.0 * cfg.relation_type_transmit_rate[RELATION_UNKNOWN];

	printf("r0 cycle %.2f: %.2f\n", current_cycle, get_affective_r0());
	printf("r0 cycle %.2f-student: %.2f\n", current_cycle, get_affective_r0( {RELATION_SCHOOL} ));
}

void callback_before_cycle (double cycle)
{
	const uint64_t people_warmup = 600;
	const double warmup = 30.0;
	const double cycle_open_school = 210.0;
	static uint32_t day = RELATION_SCHOOL_0;
	double intpart;

	// target is 49000 infected after 47 days

	if (cycle < warmup) {
		static bool test = false;

		uint32_t summon_per_cycle = (uint64_t)((double)people_warmup / (warmup * cfg.cycle_division));

dprintf("cycle %.2f summon_per_cycle %u\n", cycle, summon_per_cycle);

		for (uint32_t i=0; i<summon_per_cycle; ) {
			person_t *p = pick_random_person();

			if (p->get_state() == ST_HEALTHY) {
				p->force_infect();
				i++;
			}
		}

//printf("r0 cycle 0-student: %.2f\n", get_affective_r0( {RELATION_SCHOOL} ));
		if (!test) {
			test = true;
			stages_green++;
		}
	}
	else if (cycle == warmup) {
//		cfg.global_r0_factor = 1.05;
		printf("r0 cycle %.2f: %.2f\n", cycle, get_affective_r0());
		adjust_r_no_school(1.5);
//		backup = cfg.relation_type_transmit_rate[RELATION_SCHOOL];
//		cfg.relation_type_transmit_rate[RELATION_SCHOOL] = 0.0;
//		cfg.global_r0_factor = 0.9 / (network_get_affective_r0_fast() / cfg.global_r0_factor);
//		cfg.global_r0_factor = 0.9 / cfg.r0;
//printf("r0 cycle 30: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (cycle == 45.0) {
		adjust_r_no_school(1.31);
		//cfg.global_r0_factor = 1.15 / (network_get_affective_r0_fast() / cfg.global_r0_factor);
		//cfg.global_r0_factor = 1.16 / cfg.r0;
//printf("r0 cycle 51: %.2f\n", get_affective_r0());
		stages_green++;
	}
	else if (cycle == cycle_open_school) {
		adjust_r_open_schools();
		stages_green++;
	}
#ifdef REDUCED_SCHOOL
	else if (cycle > cycle_open_school && modf(cycle, &intpart) == 0.0) {
		for (uint32_t r=RELATION_SCHOOL_0; r<=RELATION_SCHOOL_4; r++)
			cfg.relation_type_transmit_rate[r] = 0;

		cfg.relation_type_transmit_rate[day] = 3.0 * cfg.relation_type_transmit_rate[RELATION_UNKNOWN];
		
		day++;

		if (day >= sp_school_div)
			day= 0;
	}
#endif
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
		cprintf("relation-%s: " PU64 "\n", relation_type_str(i), cfg.relation_type_number[i]);
	}

	cprintf("amount of school relations per student: %.2f\n", (double)cfg.relation_type_number[RELATION_SCHOOL] / (double)n_students);
}

struct sp_room_t {
	std::vector<person_t*> students;
	uint32_t size;
	uint32_t i;
	uint32_t room_i;
	person_t *prof;
};

static person_t* sp_find_professor (region_t *prof_region, dist_double_t& dist_prof_age)
{
	uint32_t prof_age = (uint32_t)dist_prof_age.generate();

	for (person_t *prof: prof_region->get_people()) {
		if (prof->get_age() == prof_age && network_vertex_data(prof).flags.test(RELATION_SCHOOL) == false) {
			network_vertex_data(prof).flags.set(VFLAG_PROFESSOR);
			network_vertex_data(prof).flags.set(RELATION_SCHOOL); // to prevent selecting the same professor twice
			return prof;
		}
	}

	C_ASSERT(false)

	return nullptr;
}

static void sp_create_school_relation_professor (std::vector<person_t*>& students, person_t *prof)
{
	network_create_connection_one_to_all(prof, students, RELATION_SCHOOL);
}

void sp_create_school_relation_contingency (std::vector<person_t*>& students,
                                            uint32_t age_ini,
                                            uint32_t age_end,
                                            dist_double_t& dist_class_size,
                                            dist_double_t& dist_school_size,
                                            region_t *prof_region,
                                            dist_double_t& dist_prof_age,
                                            double intra_class_ratio,
                                            double inter_class_ratio,
                                            uint32_t room_div,
                                            report_progress_t *report)
{
	std::vector<person_t*> school_students;

	uint32_t school_i = 0;
	uint32_t school_size = (uint32_t)dist_school_size.generate();

	std::vector<sp_room_t> class_students;
	
	school_students.resize( (uint32_t)dist_school_size.get_max(), nullptr );

	C_ASSERT(school_size <= school_students.size())
	
	class_students.resize( age_end + 1 );

	dprintf("total amount of students to go to school: " PU64 "\n", students.size());

	for (uint32_t age=age_ini; age<=age_end; age++) {
		class_students[age].students.resize( (uint32_t)dist_class_size.get_max(), nullptr );
		class_students[age].size = (uint32_t)dist_class_size.generate();
		class_students[age].i = 0;
		class_students[age].room_i = 0;

		C_ASSERT(class_students[age].size <= class_students[age].students.size())

		//for (uint32_t i=0; i<class_students[age].size(); i++)
		//	class_students[age][i] = nullptr;

		//class_ocupancy[age].first = 0;     // target amount of students
		//class_ocupancy[age].second = 0;    // current amount of students
	}

	for (person_t *p: students) {
		uint32_t age = p->get_age();

		C_ASSERT(age >= age_ini && age <= age_end)
		C_ASSERT(class_students[age].i <= class_students[age].size)
		C_ASSERT(school_i <= school_size)

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

			C_ASSERT(class_students[age].room_i < room_div)

			uint32_t school_rtype = static_cast<uint32_t>(RELATION_SCHOOL_0) + class_students[age].room_i;

			if (class_students[age].room_i == 0)
				class_students[age].prof = sp_find_professor(prof_region, dist_prof_age);

			network_create_connection_between_people(class_students[age].students, static_cast<relation_type_t>(school_rtype), intra_class_ratio);

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("       %.4f\n", intra_class_ratio);
panic("in\n");
#endif

			sp_create_school_relation_professor(class_students[age].students, class_students[age].prof);

			class_students[age].room_i++;

			if (class_students[age].room_i == room_div)
				class_students[age].room_i = 0;

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
				//dprintf("created school with %u students\n", school_i);
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
			C_ASSERT(class_students[age].room_i < room_div)

			uint32_t school_rtype = static_cast<uint32_t>(RELATION_SCHOOL_0) + class_students[age].room_i;

			if (class_students[age].room_i == 0)
				class_students[age].prof = sp_find_professor(prof_region, dist_prof_age);
			
			network_create_connection_between_people(class_students[age].students, static_cast<relation_type_t>(school_rtype), intra_class_ratio);
			
			sp_create_school_relation_professor(class_students[age].students, class_students[age].prof);
		}
	}

	if (school_i > 0) {
		//dprintf("created school with %u students\n", school_i);
		network_create_connection_between_people(school_students, RELATION_SCHOOL, inter_class_ratio);
	}
}