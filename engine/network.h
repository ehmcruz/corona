#ifndef __corona_network_h__
#define __corona_network_h__

void network_start_population_graph ();
void network_after_all_connetions ();
void network_create_inter_city_relation (region_t *s, region_t *t, uint64_t n);
void network_create_school_relation (std::vector<region_double_pair_t>& regions, uint32_t age_ini, uint32_t age_end, dist_double_t& dist);
void network_print_population_graph ();

pop_edge_t network_create_edge (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_data_t& edge_data);

bool network_check_if_people_are_neighbors (pop_vertex_t vertex1, pop_vertex_t vertex2);

static inline bool network_check_if_people_are_neighbors (person_t *p1, person_t *p2)
{
	return network_check_if_people_are_neighbors(p1->vertex, p2->vertex);
}

pop_edge_t network_create_edge (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_data_t& edge_data);

static inline pop_edge_t network_create_edge (person_t *p1, person_t *p2, relation_type_t type)
{
	pop_edge_data_t edge_data;
	edge_data.type = type;
	return network_create_edge(p1->vertex, p2->vertex, edge_data);
}

enum {
	NETWORK_TYPE_FULLY_CONNECTED,
	NETWORK_TYPE_NETWORK
};

template <typename T>
static void network_create_connection_between_people (T& people, relation_type_t type)
{
	for (auto it=people.begin(); it!=people.end(); ) {
		person_t *pi = *it;

		for (auto jt=++it; jt!=people.end(); ++jt) {
			person_t *pj = *jt;

			if (network_check_if_people_are_neighbors(pi, pj) == false) {
				network_create_edge(pi, pj, type);
			}
		}
	}
}

#endif