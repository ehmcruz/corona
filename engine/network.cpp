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

static inline pop_edge_data_t& edesc (pop_edge_t edge)
{
	return (*pop_graph)[edge];
}

void network_start_population_graph ()
{
	int32_t i;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

	pop_graph = new pop_graph_t( population.size() );

// code isnt ready yet
//return;

	cprintf("nvertex: %u\n", (uint32_t)num_vertices(*pop_graph));

	for (boost::tie(vi,vend) = vertices(*pop_graph), i=0; vi != vend; ++vi, i++) {
		vdesc(*vi).p = population[i];
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

static void print_vertex_neighbors (pop_vertex_t v)
{
	boost::graph_traits<pop_graph_t>::adjacency_iterator vi, vi_end;
	pop_edge_t e;
	bool exist;

	cprintf("%u (%u,%u,%u) -> ",
		vdesc(v).p->get_id(),
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

		cprintf("%u%s,", vdesc(*vi).p->get_id(), relation_type_str(edesc(e).type));
	}
	cprintf("\n");
}

static void print_population_graph ()
{
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	for (boost::tie(vi,vend) = vertices(*pop_graph); vi != vend; ++vi) print_vertex_neighbors(*vi);
}

static bool check_if_people_are_neighbors (pop_vertex_t vertex1, pop_vertex_t vertex2)
{
	boost::graph_traits<pop_graph_t>::adjacency_iterator vi, vi_end;

	for (boost::tie(vi, vi_end) = adjacent_vertices(vertex1, *pop_graph); vi != vi_end; ++vi) {
		if (*vi == vertex2)
			return true;
	}
	
	return false;
}

static inline bool check_if_people_are_neighbors (person_t *p1, person_t *p2)
{
	return check_if_people_are_neighbors(p1->vertex, p2->vertex);
}

static pop_edge_t create_edge (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_data_t& edge_data)
{
	pop_edge_t e;
	bool r;

	SANITY_ASSERT( check_if_people_are_neighbors(vertex1, vertex2) == false )
	SANITY_ASSERT( check_if_people_are_neighbors(vertex2, vertex1) == false )
	boost::tie(e, r) = add_edge(vertex1, vertex2, edge_data, *pop_graph);
	SANITY_ASSERT( check_if_people_are_neighbors(vertex1, vertex2) == true )
	SANITY_ASSERT( check_if_people_are_neighbors(vertex2, vertex1) == true )

	C_ASSERT(r == true)

#ifdef SANITY_CHECK
	pop_vertex_t source = boost::source(e, *pop_graph);
	pop_vertex_t target = boost::target(e, *pop_graph);

	SANITY_ASSERT((source == vertex1 && target == vertex2) || (source == vertex2 && target == vertex1))
#endif

	return e;
}

static inline pop_edge_t create_edge (person_t *p1, person_t *p2, relation_type_t type)
{
	pop_edge_data_t edge_data;
	edge_data.type = type;
	return create_edge(p1->vertex, p2->vertex, edge_data);
}

static uint32_t calc_family_size (region_t *region, uint32_t filled)
{
	int32_t r;

	r = (int32_t)(cfg.family_size_dist->generate() + 0.5);
	
	if (unlikely(r < 1))
		r = 1;

	if (unlikely((filled+r) > region->get_npopulation()))
		r = region->get_npopulation() - filled;
	
	return r;
}

void region_t::create_families ()
{
	uint32_t n, family_size, i, j;

	for (n=0; n<this->get_npopulation(); n+=family_size) {
		family_size = calc_family_size(this, n);

		// create a fully connected sub-graph for a family_size

		for (i=n; i<(n+family_size); i++) {
			for (j=i+1; j<(n+family_size); j++) {
				create_edge(this->get_person(i), this->get_person(j), RELATION_FAMILY);
			}
		}
	}
}

person_t* region_t::pick_random_person_not_neighbor (person_t *p)
{
	person_t *neighbor;

	do {
		neighbor = this->pick_random_person();
	} while (check_if_people_are_neighbors(p, neighbor) == true);

	return neighbor;
}

void region_t::create_random_connections ()
{
	person_t *neighbor;
	int32_t i, n;

	for (person_t *p: this->people) {
		n = (int32_t)(cfg.number_random_connections_dist->generate() + 0.5);

		// remove the amount of connections that p already have
		n -= (int32_t)get_number_of_neighbors(p, RELATION_UNKNOWN);

		if (n <= 0)
			continue;

		for (i=0; i<n; i++) {
			neighbor = this->pick_random_person_not_neighbor(p);
			create_edge(p, neighbor, RELATION_UNKNOWN);
		}
	}
}

static void calibrate_rate_per_type ()
{
	uint32_t i;
	boost::graph_traits<pop_graph_t>::edge_iterator ei, ei_end;

	for (i=0; i<NUMBER_OF_RELATIONS; i++) {
		cfg.relation_type_number[i] = 0;
		cfg.relation_type_transmit_rate[i] = 0.0;
	}

	for (boost::tie(ei,ei_end) = edges(*pop_graph); ei != ei_end; ++ei) {
		edesc(*ei).foo = 0;
	}

	for (boost::tie(ei,ei_end) = edges(*pop_graph); ei != ei_end; ++ei) {
		C_ASSERT( edesc(*ei).foo == 0 )
		edesc(*ei).foo = 1;

		cfg.relation_type_number[ edesc(*ei).type ] += 2; // we add two to count for both
	}

#ifdef SANITY_CHECK
{
	uint64_t test[NUMBER_OF_RELATIONS];
	boost::graph_traits<pop_graph_t>::adjacency_iterator vin, vin_end;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;
	pop_edge_t e;
	bool exist;
	
	// now we verify
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

		if ( check_if_people_are_neighbors(sp, tp) == false ) {
			create_edge(sp, tp, RELATION_TRAVEL);
			i++;
		}
	}
}

void network_after_all_connetions ()
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