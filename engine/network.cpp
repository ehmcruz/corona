#include <stdlib.h>

#include <corona.h>

#include <iostream>
#include <random>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/undirected_graph.hpp>
#include <boost/graph/graph_utility.hpp>

pop_graph_t *pop_graph;

static std::normal_distribution<double> *distribution;

void start_population_graph ()
{
	pop_graph = new pop_graph_t( region->get_npopulation() );
}

void region_t::add_to_population_graph ()
{
	int32_t i;
	boost::graph_traits<pop_graph_t>::vertex_iterator vi, vend;

// code isnt ready yet
return;

	cprintf("nvertex: %i\n", (int)num_vertices(*pop_graph));

	for (boost::tie(vi,vend) = vertices(*pop_graph), i=0; vi != vend; ++vi, i++) {
		(*pop_graph)[*vi].p = this->get_person(i);
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

print_graph(*pop_graph);
exit(0);
}

static uint32_t calc_family_size (region_t *region, uint32_t filled)
{
	int32_t r, max;

	r = (int32_t)((*distribution)(rgenerator));
	max = (int32_t)(cfg.family_size_mean + cfg.family_size_stddev*4.0);
	
	if (unlikely(r < 1))
		r = 1;
	else if (unlikely(r > max))
		r = max;

	if (unlikely((filled+r) > region->get_npopulation()))
		r = region->get_npopulation() - filled;
	
	return r;
}

void region_t::create_families ()
{
	uint32_t n, family_size, i, j;
	pop_edge_data_t edge_data;

	edge_data.probability = 0.0;
	edge_data.relation = RELATION_FAMILY;

	distribution = new std::normal_distribution<double>(cfg.family_size_mean, cfg.family_size_stddev);

	for (n=0; n<this->get_npopulation(); n+=family_size) {
		family_size = calc_family_size(this, n);

		// create a fully connected sub-graph for a family_size

		for (i=n; i<(n+family_size); i++) {
			for (j=i+1; j<(n+family_size); j++) {
				add_edge(this->get_person(i)->vertex, this->get_person(j)->vertex, edge_data, *pop_graph);
			}
		}
	}

	delete distribution;
}
