#include <stdlib.h>

#include <corona.h>

#include <iostream>
#include <random>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/graph_utility.hpp>

pop_graph_t *pop_graph;

void start_population_graph ()
{
	pop_graph = new pop_graph_t( region->get_npopulation() );
}

static inline pop_vertex_data_t& vdesc (pop_vertex_t vertex)
{
	return (*pop_graph)[vertex];
}

static inline pop_edge_data_t& edesc (pop_edge_t edge)
{
	return (*pop_graph)[edge];
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
		//auto source = boost::source ( *ei, g );
		//auto target = boost::target ( *ei, g );
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

	cprintf("%u (%u,%u) -> ", vdesc(v).p->get_id(), get_number_of_neighbors(v), get_number_of_neighbors(v, RELATION_FAMILY));
	for (boost::tie(vi, vi_end) = adjacent_vertices(v, *pop_graph); vi != vi_end; ++vi) {

		boost::tie(e, exist) = boost::edge(v, *vi, *pop_graph);
		C_ASSERT(exist)

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

void region_t::add_to_population_graph ()
{
	int32_t i;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

// code isnt ready yet
return;

	cprintf("nvertex: %i\n", (int)num_vertices(*pop_graph));

	for (boost::tie(vi,vend) = vertices(*pop_graph), i=0; vi != vend; ++vi, i++) {
		vdesc(*vi).p = this->get_person(i);
		this->people[i]->vertex = *vi;
	}
//exit(1);
#if 0
	for (i=0; i<this->npopulation; i++) {
		cprintf("pid: %u %p\n", (*pop_graph)[this->get_person(i)->vertex].p->get_id(), this->get_person(i)->vertex);
	}
	exit(1);
#endif
	this->create_families();
	this->create_random_connections();

#if 1
	print_population_graph();
	//cprintf("--------------------------------------------\n");
	//print_graph(*pop_graph);
	exit(0);
#endif
}

static void create_edge (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_data_t& edge_data)
{
	SANITY_ASSERT( check_if_people_are_neighbors(vertex1, vertex2) == false )
	add_edge(vertex1, vertex2, edge_data, *pop_graph);
	SANITY_ASSERT( check_if_people_are_neighbors(vertex1, vertex2) == true )
}

static inline void create_edge (person_t *p1, person_t *p2, pop_edge_data_t& edge_data)
{
	create_edge(p1->vertex, p2->vertex, edge_data);
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
	pop_edge_data_t edge_data;

	edge_data.type = RELATION_FAMILY;

	for (n=0; n<this->get_npopulation(); n+=family_size) {
		family_size = calc_family_size(this, n);

		// create a fully connected sub-graph for a family_size

		for (i=n; i<(n+family_size); i++) {
			for (j=i+1; j<(n+family_size); j++) {
				create_edge(this->get_person(i), this->get_person(j), edge_data);
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
	pop_edge_data_t edge_data;

	edge_data.type = RELATION_UNKNOWN;

	for (person_t *p: this->people) {
		n = (int32_t)(cfg.number_random_connections_dist->generate() + 0.5);

		// remove the amount of connections that p already have
		n -= (int32_t)get_number_of_neighbors(p, RELATION_UNKNOWN);

		if (n <= 0)
			continue;

		for (i=0; i<n; i++) {
			neighbor = this->pick_random_person_not_neighbor(p);
			create_edge(p, neighbor, edge_data);
		}
	}
}
