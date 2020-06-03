#include <stdlib.h>

#include <corona.h>

#include <iostream>
#include <random>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/graph_utility.hpp>

static pop_graph_t *pop_graph;

static inline pop_vertex_data_t& vdesc (pop_vertex_t vertex)
{
	return (*pop_graph)[vertex];
}

static inline pop_vertex_data_t& vdesc (person_t *p)
{
	return vdesc(p->vertex);
}

static inline pop_edge_data_t& edesc (pop_edge_t edge)
{
	return (*pop_graph)[edge];
}

void network_start_population_graph ()
{
	int32_t i;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	pop_graph = new pop_graph_t( population.size() );

	for (i=0; i<NUMBER_OF_RELATIONS; i++)
		cfg.relation_type_number[i] = 0;

// code isnt ready yet
//return;

	cprintf("nvertex: %u\n", (uint32_t)num_vertices(*pop_graph));

	for (boost::tie(vi,vend) = vertices(*pop_graph), i=0; vi != vend; ++vi, i++) {
		vdesc(*vi).p = population[i];
		vdesc(*vi).flags.reset();
		population[i]->vertex = *vi;
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

static void print_vertex_neighbors (pop_vertex_t v, std::bitset<NUMBER_OF_RELATIONS>& flags)
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

void network_print_population_graph (std::bitset<NUMBER_OF_RELATIONS>& flags)
{
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) print_vertex_neighbors(*vi, flags);
}

bool network_check_if_people_are_neighbors (pop_vertex_t vertex1, pop_vertex_t vertex2)
{
	boost::graph_traits<pop_graph_t>::adjacency_iterator vi, vi_end;

	for (boost::tie(vi, vi_end) = adjacent_vertices(vertex1, *pop_graph); vi != vi_end; ++vi) {
		if (*vi == vertex2)
			return true;
	}
	
	return false;
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

	cfg.relation_type_number[edge_data.type] += 2;

	return e;
}

static uint32_t calc_family_size (dist_double_t& dist, region_t *region, uint32_t filled)
{
	int32_t r;

	r = (int32_t)(dist.generate() + 0.5);
	
	if (unlikely(r < 1))
		r = 1;

	if (unlikely((filled+r) > region->get_npopulation()))
		r = region->get_npopulation() - filled;
	
	return r;
}

void region_t::create_families (dist_double_t& dist, report_progress_t *report)
{
	uint32_t n, family_size, i, j;

	for (n=0; n<this->get_npopulation(); n+=family_size) {
		family_size = calc_family_size(dist, this, n);

		// create a fully connected sub-graph for a family_size

		for (i=n; i<(n+family_size); i++) {
			for (j=i+1; j<(n+family_size); j++) {
				network_create_edge(this->get_person(i), this->get_person(j), RELATION_FAMILY);
			}
		}

		if (report != nullptr)
			report->check_report(family_size);
	}
}

person_t* region_t::pick_random_person_not_neighbor (person_t *p)
{
	person_t *neighbor;

	do {
		neighbor = this->pick_random_person();
	} while (network_check_if_people_are_neighbors(p, neighbor) == true);

	return neighbor;
}

void region_t::create_random_connections (dist_double_t& dist, report_progress_t *report)
{
	person_t *neighbor;
	int32_t i, n;

	for (person_t *p: this->people) {
		n = (int32_t)(dist.generate() + 0.5);

		// remove the amount of connections that p already have
		n -= (int32_t)get_number_of_neighbors(p, RELATION_UNKNOWN);

		if (n <= 0)
			continue;

		for (i=0; i<n; i++) {
			neighbor = this->pick_random_person_not_neighbor(p);
			network_create_edge(p, neighbor, RELATION_UNKNOWN);
		}

		if (report != nullptr)
			report->check_report(1);
	}
}

uint64_t get_n_population_per_relation_flag (relation_type_t relation)
{
	uint64_t n = 0;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) {
		n += vdesc(*vi).flags.test(relation);
	}

	return n;
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
		SANITY_ASSERT(test[i] == cfg.relation_type_number[i])
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
		SANITY_ASSERT(test[i] == cfg.relation_type_number[i])
	}
}
#endif

	adjust_weights_to_fit_mean<uint32_t, uint64_t, NUMBER_OF_RELATIONS> (
		cfg.relation_type_weights,
		cfg.relation_type_number,
		cfg.probability_infect_per_cycle * (double)population.size(),
		cfg.relation_type_transmit_rate
		);
}

void network_create_inter_city_relation (region_t *s, region_t *t, uint64_t n)
{
	uint64_t i;
	person_t *sp, *tp;

	i = 0;
	
	while (i < n) {
		sp = s->pick_random_person();
		tp = t->pick_random_person();

		if (network_check_if_people_are_neighbors(sp, tp) == false) {
			network_create_edge(sp, tp, RELATION_TRAVEL);
			i++;
		}
	}
}

void network_create_school_relation (std::vector<region_double_pair_t>& regions,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist,
                                     double intra_class_ratio,
                                     double inter_class_ratio)
{
	struct region_int_pair_t {
		region_t *region;
		int64_t n;
	};

	struct class_room_t {
		std::vector<region_int_pair_t> n_per_region;
		std::vector<region_int_pair_t> has_per_region;
		std::list<person_t*> students;
		uint64_t size;
	};

	struct class_per_age_t {
		std::list<class_room_t> rooms;
		std::list<class_room_t>::iterator next_room_it;
	};

	class_per_age_t class_per_age[AGES_N];
	uint64_t total_per_age[AGES_N];
	uint64_t ri, i, students, total_students;
	uint32_t age, nrooms;

	struct stc_calc_students_t: public region_double_pair_t {
		double weight;
		int64_t value;
	};

	std::vector<stc_calc_students_t> v;

	dprintf("school loading stage 1...\n");

	v.resize( regions.size() );

	for (i=0; i<regions.size(); i++) {
		v[i].region = regions[i].region;
		v[i].ratio = regions[i].ratio;
	}

	for (age=age_ini; age<=age_end; age++) {
		total_per_age[age] = 0;

		for (auto& r: v)
			total_per_age[age] += (uint64_t)((double)r.region->get_region_people_per_age(age) * r.ratio);

		for (auto& r: v)
			r.weight = ((double)r.region->get_region_people_per_age(age) * r.ratio) / (double)total_per_age[age];

		students = 0;

		while (students < total_per_age[age]) {
			class_per_age[age].rooms.emplace_back();
			class_room_t& room = class_per_age[age].rooms.back();

			room.size = (uint64_t)dist.generate();
			adjust_values_to_fit_mean(v, room.size);

			for (auto& r: v) {
				if (r.value == 0) {
					r.value = 1;
					room.size++;
				}
			}

			room.n_per_region.reserve( regions.size() );
			room.has_per_region.reserve( regions.size() );

			for (auto& r: v) {
				room.n_per_region.push_back( {r.region, r.value} );
				room.has_per_region.push_back( {r.region, 0} );
			}

			students += room.size;
		}
	}

	for (age=age_ini; age<=age_end; age++) {
		dprintf("total_per_age[%u]: " PU64 "\n", age, total_per_age[age]);

		for (auto& room: class_per_age[age].rooms) {
			dprintf("class_size: " PU64 " age %u\n", room.size, age);

			for (auto& r: room.n_per_region) {
				dprintf("\tregion %s students " PU64 "\n", r.region->get_name().c_str(), r.n);
			}
		}
	}

	total_students = 0;
	ri = 0;
	for (auto& r: regions) {
		for (age=age_ini; age<=age_end; age++)
			class_per_age[age].next_room_it = class_per_age[age].rooms.begin();

		for (person_t *p: r.region->get_people()) {
			age = p->get_age();

			if (age >= age_ini && age <= age_end && vdesc(p).flags.test(RELATION_SCHOOL) == false) {
				do {
					if (class_per_age[age].next_room_it == class_per_age[age].rooms.end())
						break;

					class_room_t& room = *class_per_age[age].next_room_it;

					C_ASSERT(room.has_per_region[ri].region == p->get_region())
					C_ASSERT(room.has_per_region[ri].region == r.region)
					C_ASSERT(room.n_per_region[ri].region == p->get_region())
					C_ASSERT(room.n_per_region[ri].region == r.region)

					C_ASSERT(room.has_per_region[ri].n <= room.n_per_region[ri].n)

					if (room.has_per_region[ri].n == room.n_per_region[ri].n)
						++class_per_age[age].next_room_it;
					else
						break;
				} while (true);

				if (class_per_age[age].next_room_it != class_per_age[age].rooms.end()) {
					class_room_t& room = *class_per_age[age].next_room_it;

					C_ASSERT(room.has_per_region[ri].n < room.n_per_region[ri].n)

					room.has_per_region[ri].n++;
					total_students++;
					room.students.push_back(p);
				}
			}
		}

		ri++;
	}

	dprintf("school loading stage 2...\n");

	// create intra-classroom relations

	nrooms = 0;
	for (age=age_ini; age<=age_end; age++) {
		nrooms += class_per_age[age].rooms.size();

		for (auto& room: class_per_age[age].rooms) {
			network_create_connection_between_people(room.students, RELATION_SCHOOL, intra_class_ratio);
		}
	}

	dprintf("school loading stage 3 nrooms=%u total_students=" PU64 "...\n", nrooms, total_students);

	// create inter-classroom relations

	report_progress_t progress_3("school loading stage 3...", nrooms*nrooms, 10000);

	for (uint32_t age1=age_ini; age1<=age_end; age1++) {
		for (auto& room1: class_per_age[age1].rooms) {
			for (uint32_t age2=age_ini; age2<=age_end; age2++) {
				for (auto& room2: class_per_age[age2].rooms) {
					network_create_connection_between_people(room1.students, room2.students, RELATION_SCHOOL, inter_class_ratio);
					progress_3.check_report(1);
				}
			}
		}
	}

	dprintf("school loaded\n");
//	print_population_graph();
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

double network_get_affective_r0 (std::bitset<NUMBER_OF_RELATIONS>& flags)
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

				r0_person += cfg.relation_type_transmit_rate[ edesc(e).type ];
			}

			//r0_person *= cfg.global_r0_factor;
			r0 += r0_person;
		}
	}

	r0 *= cfg.global_r0_factor;
	r0 *= cfg.cycles_contagious;
	r0 /= (double)n;

	return r0;
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
		it.prob += cfg.relation_type_transmit_rate[type];

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

	it.prob *= cfg.global_r0_factor;
	it.prob *= r0_factor_per_group[ this->get_person()->get_infected_state() ];

	if (likely(it.prob > 0.0))
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
				person_t *p = this->list->pick_random_person();

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