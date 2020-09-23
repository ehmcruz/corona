#include <stdlib.h>

#include <corona.h>

#include <iostream>
#include <random>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/graph_utility.hpp>

pop_graph_t *pop_graph;

#define vdesc(v) network_vertex_data(v)
#define edesc(e) network_edge_data(e)

void network_start_population_graph ()
{
	int32_t i;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	pop_graph = new pop_graph_t( population.size() );

	for (i=0; i<NUMBER_OF_RELATIONS; i++)
		cfg->relation_type_number[i] = 0;

// code isnt ready yet
//return;

	cprintf("nvertex: %u\n", (uint32_t)num_vertices(*pop_graph));

	C_ASSERT(num_vertices(*pop_graph) == population.size())

	i = 0;
	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		vdesc(*vi).p = population[i];
		vdesc(*vi).flags.reset();
		vdesc(*vi).school_class_room = UNDEFINED32;
		population[i]->vertex = *vi;
		i++;
	}
//exit(1);
#if 0
	for (i=0; i<this->npopulation; i++) {
		cprintf("pid: %u %p\n", (*pop_graph)[this->get_person(i)->vertex].p->get_id(), this->get_person(i)->vertex);
	}
	exit(1);
#endif
}

static uint32_t get_number_of_neighbors (pop_vertex_t vertex)
{
	return degree(vertex, *pop_graph);
}

static inline uint32_t get_number_of_neighbors (person_t *p)
{
	return get_number_of_neighbors(p->vertex);
}

static uint32_t get_number_of_neighbors (pop_vertex_t vertex, relation_type_t type)
{
	boost::graph_traits<pop_graph_t>::out_edge_iterator ei, ei_end;
	uint32_t n = 0;

	for (boost::tie(ei, ei_end) = out_edges(vertex, *pop_graph); ei != ei_end; ++ei) {
		n += (edesc(*ei).type == type);
	}
	
	return n;
}

static inline uint32_t get_number_of_neighbors (person_t *p, relation_type_t type)
{
	return get_number_of_neighbors(p->vertex, type);
}

static void print_vertex_neighbors (pop_vertex_t v, std::bitset<NUMBER_OF_FLAGS>& flags)
{
	boost::graph_traits<pop_graph_t>::adjacency_iterator vi, vi_end;
	pop_edge_t e;
	bool exist;

	cprintf("%u (city %u, age %u) (%u,%u,%u) -> ",
		vdesc(v).p->get_id(),
		vdesc(v).p->get_region()->get_id(),
		vdesc(v).p->get_age(),
		get_number_of_neighbors(v),
		get_number_of_neighbors(v, RELATION_FAMILY),
		get_number_of_neighbors(v, RELATION_UNKNOWN)
		);

	for (boost::tie(vi, vi_end) = adjacent_vertices(v, *pop_graph); vi != vi_end; ++vi) {

		boost::tie(e, exist) = boost::edge(v, *vi, *pop_graph);
		C_ASSERT(exist)

	#ifdef SANITY_CHECK
		pop_vertex_t source = boost::source(e, *pop_graph);
		pop_vertex_t target = boost::target(e, *pop_graph);

		SANITY_ASSERT((source == v && target == *vi) || (source == *vi && target == v))
	#endif

		if (flags.test(edesc(e).type))
			cprintf("%u%s%u(%u),", vdesc(*vi).p->get_id(), relation_type_str(edesc(e).type), vdesc(*vi).p->get_region()->get_id(), vdesc(*vi).p->get_age());
	}
	cprintf("\n");
}

void network_print_population_graph (std::bitset<NUMBER_OF_FLAGS>& flags)
{
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) print_vertex_neighbors(*vi, flags);
}

bool network_check_if_people_are_neighbors (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_t *edge)
{
	pop_edge_t e;
	bool b;

	boost::tie(e, b) = boost::edge(vertex1, vertex2, *pop_graph);

	if (edge != nullptr)
		*edge = e;

	return b;
}

pop_edge_t network_get_edge (pop_vertex_t vertex1, pop_vertex_t vertex2)
{
	pop_edge_t e;
	bool b;

	boost::tie(e, b) = boost::edge(vertex1, vertex2, *pop_graph);

	C_ASSERT(b != false)

	return e;
}

void network_delete_edge (pop_vertex_t vertex1, pop_vertex_t vertex2)
{
	pop_edge_t e;

	e = network_get_edge(vertex1, vertex2);
	
	cfg->relation_type_number[edesc(e).type] -= 2;
	
	boost::remove_edge(vertex1, vertex2, *pop_graph);

#ifdef SANITY_CHECK
	bool b;

	boost::tie(e, b) = boost::edge(vertex1, vertex2, *pop_graph);

	C_ASSERT(b == false)
#endif
}

void network_delete_edge (pop_edge_t e)
{
	cfg->relation_type_number[edesc(e).type] -= 2;
	
	boost::remove_edge(e, *pop_graph);
}

pop_edge_t network_create_edge (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_data_t& edge_data)
{
	pop_edge_t e;
	bool r;

	SANITY_ASSERT( network_check_if_people_are_neighbors(vertex1, vertex2) == false )
	SANITY_ASSERT( network_check_if_people_are_neighbors(vertex2, vertex1) == false )
	boost::tie(e, r) = add_edge(vertex1, vertex2, edge_data, *pop_graph);
	SANITY_ASSERT( network_check_if_people_are_neighbors(vertex1, vertex2) == true )
	SANITY_ASSERT( network_check_if_people_are_neighbors(vertex2, vertex1) == true )

	C_ASSERT(r == true)

#ifdef SANITY_CHECK
	pop_vertex_t source = boost::source(e, *pop_graph);
	pop_vertex_t target = boost::target(e, *pop_graph);

	SANITY_ASSERT((source == vertex1 && target == vertex2) || (source == vertex2 && target == vertex1))
#endif

	vdesc(vertex1).flags.set(edge_data.type);
	vdesc(vertex2).flags.set(edge_data.type);

	cfg->relation_type_number[edge_data.type] += 2;

	return e;
}

void region_t::create_families (dist_double_t& dist, report_progress_t *report)
{
	std::vector<person_t*> tmp = this->people;

	std::shuffle(tmp.begin(), tmp.end(), rgenerator);

	network_create_clusters(tmp, dist, RELATION_FAMILY, 1.0, report);
}

void region_t::create_work_relations (dist_double_t& dist, uint32_t age_ini, uint32_t age_end, double employment_rate, double relation_ratio, report_progress_t *report)
{
	uint32_t n = 0, i;

	for (person_t *p: this->people) {
		if (p->get_age() >= age_ini && p->get_age() <= age_end)
			n++;
	}

	n = static_cast<uint32_t>(static_cast<double>(n) * employment_rate);

	std::vector<person_t*> tmp_all = this->people;
	std::vector<person_t*> tmp_workers;
	
	std::shuffle(tmp_all.begin(), tmp_all.end(), rgenerator);

	tmp_workers.reserve(n);

	i = 0;
	for (person_t *p: tmp_all) {
		C_ASSERT(i <= n)

		if (bunlikely(i == n))
			break;

		if (p->get_age() >= age_ini && p->get_age() <= age_end) {
			tmp_workers.push_back(p);
			i++;
		}
	}

	network_create_clusters(tmp_workers, dist, RELATION_WORK, relation_ratio, report);
}

person_t* region_t::pick_random_person_not_neighbor (person_t *p)
{
	person_t *neighbor;

	do {
		neighbor = this->pick_random_person();
	} while (network_check_if_people_are_neighbors(p, neighbor) == true);

	return neighbor;
}

void region_t::create_random_connections (dist_double_t& dist, relation_type_t type, report_progress_t *report)
{
	person_t *neighbor;
	int32_t i, n;

	for (person_t *p: this->people) {
		n = (int32_t)(dist.generate() + 0.5);

		// remove the amount of connections that p already have
		n -= (int32_t)get_number_of_neighbors(p, RELATION_UNKNOWN);

		if (report != nullptr)
			report->check_report(1);

		if (n <= 0)
			continue;

		for (i=0; i<n; i++) {
			neighbor = this->pick_random_person_not_neighbor(p);
			network_create_edge(p, neighbor, type);
		}
	}
}

uint64_t get_n_population_per_relation_flag (std::bitset<NUMBER_OF_FLAGS>& flags)
{
	uint64_t n = 0;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		n += ((vdesc(*vi).flags & flags).any() == true);
	}

	return n;
}

void network_iterate_over_edges (std::function<void (pop_vertex_t s, pop_vertex_t t, pop_edge_t e)> callback)
{
	boost::graph_traits<pop_graph_t>::edge_iterator ei, ei_end;

	for (boost::tie(ei,ei_end) = edges(*pop_graph); ei != ei_end; ++ei) {
		pop_edge_t e = *ei;
		pop_vertex_t s = boost::source(e, *pop_graph);
		pop_vertex_t t = boost::target(e, *pop_graph);

		callback(s, t, e);
	}
}

void network_iterate_over_vertices (std::function<void (pop_vertex_t v)> callback)
{
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		callback(*vi);
	}
}

static void calibrate_rate_per_type ()
{
	uint32_t i;
	boost::graph_traits<pop_graph_t>::edge_iterator ei, ei_end;

#ifdef SANITY_CHECK
{
	uint64_t test[NUMBER_OF_RELATIONS];
	boost::graph_traits<pop_graph_t>::adjacency_iterator vin, vin_end;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	pop_edge_t e;
	bool exist;

	// now we verify

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		test[i] = 0;
	}

	for (boost::tie(ei,ei_end) = edges(*pop_graph); ei != ei_end; ++ei) {
		edesc(*ei).foo = 0;
	}

	for (boost::tie(ei,ei_end) = edges(*pop_graph); ei != ei_end; ++ei) {
		C_ASSERT( edesc(*ei).foo == 0 )
		edesc(*ei).foo = 1;

		test[ edesc(*ei).type ] += 2; // we add two to count for both
	}

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		SANITY_ASSERT(test[i] == cfg->relation_type_number[i])
	}
	
	// now we verify again
	// yeah, I'm super concerned about sanity
	// and I'm not very familiar with boost graph library

	for (i=0; i<NUMBER_OF_RELATIONS; i++)
		test[i] = 0;

	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		for (boost::tie(vin, vin_end) = adjacent_vertices(*vi, *pop_graph); vin != vin_end; ++vin) {
			boost::tie(e, exist) = boost::edge(*vi, *vin, *pop_graph);
			C_ASSERT(exist)

			test[ edesc(e).type ]++;
		}
	}

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		SANITY_ASSERT(test[i] == cfg->relation_type_number[i])
	}
}
#endif

	adjust_weights_to_fit_mean<double, uint64_t, NUMBER_OF_RELATIONS> (
		cfg->relation_type_weights,
		cfg->relation_type_number,
		cfg->probability_infect_per_cycle_step * (double)population.size(),
		cfg->relation_type_transmit_rate
		);
}

void network_create_inter_city_relation (region_t *s, region_t *t, uint64_t n, relation_type_t type)
{
	uint64_t i;
	person_t *sp, *tp;

	i = 0;
	
	while (i < n) {
		sp = s->pick_random_person();
		tp = t->pick_random_person();

		if (network_check_if_people_are_neighbors(sp, tp) == false) {
			network_create_edge(sp, tp, type);
			i++;
		}
	}
}

static person_t* network_create_school_relation_professor (std::vector<person_t*>& students,
                                                          region_t *prof_region,
                                                          dist_double_t& dist_prof_age,
                                                          relation_type_t type_prof)
{
	person_t *the_prof = nullptr;

	do {
		uint32_t prof_age = (uint32_t)dist_prof_age.generate();

		for (person_t *prof: prof_region->get_people()) {
			if (prof->get_age() == prof_age && vdesc(prof).flags.test(VFLAG_PROFESSOR) == false) {
				the_prof = prof;
				vdesc(prof).flags.set(VFLAG_PROFESSOR);
				network_create_connection_one_to_all(prof, students, type_prof);
				break;
			}
		}
	} while (the_prof == nullptr);

	return the_prof;
}

void network_create_school_relation (std::vector<person_t*>& students,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist_class_size,
                                     dist_double_t& dist_school_size,
                                     region_t *prof_region,
                                     dist_double_t& dist_prof_age,
                                     double intra_class_ratio,
                                     double inter_class_ratio,
                                     relation_type_t type_same_room,
                                     relation_type_t type_prof,
                                     relation_type_t type_other_room,
                                     report_progress_t *report)
{
	struct network_school_room_t {
		std::vector<person_t*> students;
		uint32_t size;
		uint32_t i;
	};

	static uint32_t class_room_id = 0;

	std::vector<person_t*> school_students;

	uint32_t school_i = 0;
	uint32_t school_size = (uint32_t)dist_school_size.generate();

	std::vector<network_school_room_t> class_students;
	
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

			network_create_connection_between_people(class_students[age].students, type_same_room, intra_class_ratio);

			for (person_t *p: class_students[age].students) {
				if (p == nullptr)
					break;

				network_vertex_data(p).school_class_room = class_room_id;
			}

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("       %.4f\n", intra_class_ratio);
panic("in\n");
#endif

			person_t *prof = network_create_school_relation_professor(class_students[age].students, prof_region, dist_prof_age, type_prof);

			network_vertex_data(prof).school_class_room = class_room_id;

			class_room_id++;

#if 0
network_print_population_graph( {RELATION_SCHOOL} );
for (auto p: class_students[age].students) if (p) dprintf("%i, ", p->get_id());
dprintf("\n");
panic("in\n");
#endif

			// clear room
			for (auto& p: class_students[age].students)
				p = nullptr;

			class_students[age].size = (uint32_t)dist_class_size.generate();
			class_students[age].i = 0;

			C_ASSERT(class_students[age].size <= class_students[age].students.size())

			if (school_i == school_size) {
				//dprintf("created school with %u students\n", school_i);
				network_create_connection_between_people(school_students, type_other_room, inter_class_ratio);

				// clear school
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

			network_create_connection_between_people(class_students[age].students, type_same_room, intra_class_ratio);

			for (person_t *p: class_students[age].students) {
				if (p == nullptr)
					break;

				network_vertex_data(p).school_class_room = class_room_id;
			}

			person_t *prof = network_create_school_relation_professor(class_students[age].students, prof_region, dist_prof_age, type_prof);

			network_vertex_data(prof).school_class_room = class_room_id;

			class_room_id++;
		}
	}

	if (school_i > 0) {
		//dprintf("created school with %u students\n", school_i);
		network_create_connection_between_people(school_students, type_other_room, inter_class_ratio);
	}
}

void network_after_all_regular_connetions ()
{
	calibrate_rate_per_type();

#if 0
	print_population_graph();
	//cprintf("--------------------------------------------\n");
	//print_graph(*pop_graph);
	cprintf("end\n");
//	exit(0);
#endif
}

static inline pop_vertex_t get_neighbor (pop_vertex_t v, pop_edge_t e)
{
	pop_vertex_t s = boost::source(e, *pop_graph);
	pop_vertex_t t = boost::target(e, *pop_graph);
	pop_vertex_t r;

	if (v == s)
		r = t;
	else if (v == t)
		r = s;
	else {
		C_ASSERT(0)
		r = s; // whatever, just to avoid warning
	}

	return r;
}

static inline person_t* get_neighbor (person_t *p, pop_edge_t e)
{
	return vdesc(get_neighbor(p->vertex, e)).p;
}

double network_get_affective_r0 (std::bitset<NUMBER_OF_FLAGS>& flags)
{
	boost::graph_traits<pop_graph_t>::adjacency_iterator vin, vin_end;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	pop_edge_t e;
	bool exist;
	double r0, r0_person;
	uint64_t n;

	r0 = 0.0;
	n = 0;

	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		if ((vdesc(*vi).flags & flags).any()) {
			r0_person = 0.0;
			n++;

			for (boost::tie(vin, vin_end) = adjacent_vertices(*vi, *pop_graph); vin != vin_end; ++vin) {
				boost::tie(e, exist) = boost::edge(*vi, *vin, *pop_graph);
				C_ASSERT(exist)

				r0_person += cfg->relation_type_transmit_rate[ edesc(e).type ];
			}

			//r0_person *= cfg->global_r0_factor;
			r0 += r0_person;
		}
	}

	r0 *= cfg->global_r0_factor;
	r0 *= cfg->cycles_contagious->get_expected();
	r0 *= cfg->cycle_division;
	r0 /= (double)n;

	return r0;
}

double network_get_affective_r0_fast ()
{
	double r0 = 0.0;

	for (uint32_t i=0; i<NUMBER_OF_RELATIONS; i++)
		r0 += cfg->relation_type_transmit_rate[i] * (double)cfg->relation_type_number[i];

	r0 *= cfg->global_r0_factor;
	r0 *= cfg->cycles_contagious->get_expected();
	r0 *= cfg->cycle_division;
	r0 /= (double)population.size();

	return r0;
}

// -----------------------------------------------------------------------

//double blah = 0.0;

neighbor_list_t::iterator_t neighbor_list_network_simple_t::begin ()
{
	neighbor_list_network_simple_t::iterator_network_simple_t it;

	it.list = this;

	boost::tie(it.edge_i, it.edge_i_end) = out_edges(this->get_person()->vertex, *pop_graph);

	it.calc();

	return it;
}

neighbor_list_network_simple_t::iterator_network_simple_t::iterator_network_simple_t ()
{
	static_assert(sizeof(neighbor_list_network_simple_t::iterator_network_simple_t) == sizeof(neighbor_list_t::iterator_t));

	this->ptr_check_probability = (prob_func_t) &neighbor_list_network_simple_t::iterator_network_simple_t::check_probability_;
	this->ptr_get_person = (person_func_t) &neighbor_list_network_simple_t::iterator_network_simple_t::get_person_;
	this->ptr_next = (next_func_t) &neighbor_list_network_simple_t::iterator_network_simple_t::next_;

	this->current = nullptr;
}

void neighbor_list_network_simple_t::iterator_network_simple_t::calc ()
{
	this->current = nullptr;

	while (this->edge_i != this->edge_i_end && this->current == nullptr) {
		person_t *neighbor = get_neighbor(this->list->get_person(), *this->edge_i);

		if (neighbor->get_state() == ST_HEALTHY) {
			relation_type_t type;
			double infect_prob;

			type = edesc(*this->edge_i).type;

			infect_prob = cfg->relation_type_transmit_rate[type] * cfg->get_factor_per_relation_group(type, this->list->get_person());
			infect_prob *= cfg->global_r0_factor;
			infect_prob *= cfg->r0_factor_per_group[ this->list->get_person()->get_infected_state() ];

			if (roll_dice(infect_prob))
				this->current = neighbor;
		}

		++this->edge_i;
	}
}

bool neighbor_list_network_simple_t::iterator_network_simple_t::check_probability_ ()
{
	return (this->current != nullptr);
}

person_t* neighbor_list_network_simple_t::iterator_network_simple_t::get_person_ ()
{
	return this->current;
}

neighbor_list_t::iterator_t& neighbor_list_network_simple_t::iterator_network_simple_t::next_ ()
{
	this->calc();

	return *this;
}

// -----------------------------------------------------------------------

//double blah = 0.0;

neighbor_list_t::iterator_t neighbor_list_network_t::begin ()
{
	neighbor_list_network_t::iterator_network_t it;
	boost::graph_traits<pop_graph_t>::out_edge_iterator ei, ei_end;
	relation_type_t type;
	int32_t i;

	//C_ASSERT(this->get_person()->get_state() == ST_INFECTED)

	it.list = this;

	this->nconnected = get_number_of_neighbors( this->get_person() );

	if (this->connected.size() < this->nconnected) {
		this->connected.reserve(this->nconnected);

		for (i=this->connected.size(); this->connected.size() < this->nconnected; i++) {
			neighbor_list_t::pair_t pair(0.0, nullptr);
			this->connected.push_back( pair );
		}
	}

	it.prob = 0.0;

	i = 0;
	for (boost::tie(ei, ei_end) = out_edges(this->get_person()->vertex, *pop_graph); ei != ei_end; ++ei) {
		C_ASSERT(i < this->nconnected)

		neighbor_list_t::pair_t& pair = this->connected[i];

		type = edesc(*ei).type;
		it.prob += cfg->relation_type_transmit_rate[type] * cfg->get_factor_per_relation_group(type, this->get_person());

		pair.first = it.prob;
		pair.second = get_neighbor(this->get_person(), *ei);

		i++;
	}

	C_ASSERT(i == this->nconnected)

	for (i=0; i < this->nconnected; i++) {
		neighbor_list_t::pair_t& pair = this->connected[i];
		pair.first /= it.prob;

//		cprintf("%.4f(%u) ", pair.first, pair.second->get_id());
	}
//blah += it.prob; cprintf(" (total %.4f)\n", it.prob); //exit(1);

	it.prob *= cfg->global_r0_factor;
	it.prob *= cfg->r0_factor_per_group[ this->get_person()->get_infected_state() ];

	if (blikely(it.prob > 0.0))
		it.calc();
	else
		it.current = nullptr;

	return it;
}

neighbor_list_network_t::iterator_network_t::iterator_network_t ()
{
	static_assert(sizeof(neighbor_list_network_t::iterator_network_t) == sizeof(neighbor_list_t::iterator_t));

	this->ptr_check_probability = (prob_func_t) &neighbor_list_network_t::iterator_network_t::check_probability_;
	this->ptr_get_person = (person_func_t) &neighbor_list_network_t::iterator_network_t::get_person_;
	this->ptr_next = (next_func_t) &neighbor_list_network_t::iterator_network_t::next_;

	this->current = nullptr;
}

void neighbor_list_network_t::iterator_network_t::calc ()
{
	if (this->prob > 0.0) {
		double p;
		bool end = false;

		do {
			if (this->prob <= 1.0) {
				p = this->prob;
				this->prob = 0.0;
			}
			else {
				p = 1.0;
				this->prob -= 1.0;
			}

			if (roll_dice(p)) { // ok, I will infect someone, let's find someone
				person_t *p = static_cast<neighbor_list_network_t*>(this->list)->pick_random_person();

				if (p->get_state() == ST_HEALTHY) {
					this->current = p;
					end = true;
				}
				else if (this->prob == 0.0) { // The person is not suscetible, so I won't infect, and I'm out of probability to try again
					this->current = nullptr;
					end = true;
				}
			}
			else if (this->prob == 0.0) { // I won't infect and I'm out of probability to try again
				this->current = nullptr;
				end = true;
			}
		} while (!end);
	}
	else
		this->current = nullptr;
}

bool neighbor_list_network_t::iterator_network_t::check_probability_ ()
{
	return (this->current != nullptr);
}

person_t* neighbor_list_network_t::iterator_network_t::get_person_ ()
{
	return this->current;
}

neighbor_list_t::iterator_t& neighbor_list_network_t::iterator_network_t::next_ ()
{
	this->calc();

	return *this;
}

person_t* neighbor_list_network_t::pick_random_person ()
{
	double dice, test;
	uint32_t i;
	person_t *chosen = nullptr;

	dice = generate_random_between_0_and_1();

	for (i=0; i<this->nconnected; i++) {
		neighbor_list_t::pair_t& pair = this->connected[i];

		chosen = pair.second;

		if (dice <= pair.first)
			break;
	}

	C_ASSERT(chosen != nullptr)

	return chosen;
}