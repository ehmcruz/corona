#ifndef __corona_network_h__
#define __corona_network_h__

extern pop_graph_t *pop_graph;

void network_start_population_graph ();
void network_after_all_regular_connetions ();
double network_get_affective_r0 (std::bitset<NUMBER_OF_FLAGS>& flags);
double network_get_affective_r0_fast (std::bitset<NUMBER_OF_FLAGS>& flags);
void network_create_inter_city_relation (region_t *s, region_t *t, uint64_t n, relation_type_t type=RELATION_TRAVEL);
void network_iterate_over_edges (void (*callback)(pop_vertex_data_t& s, pop_vertex_data_t& t, pop_edge_data_t& e));
pop_edge_t network_get_edge (pop_vertex_t vertex1, pop_vertex_t vertex2);
void network_delete_edge (pop_vertex_t vertex1, pop_vertex_t vertex2);

inline void network_delete_edge (person_t *s, person_t *t)
{
	network_delete_edge(s->vertex, t->vertex);
}

void network_create_school_relation (std::vector<region_double_pair_t>& regions,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist,
                                     region_t *prof_region,
                                     dist_double_t& dist_prof_age,
                                     double intra_class_ratio=1.0,
                                     double inter_class_ratio=0.025);

void network_create_school_relation_v2 (std::vector<person_t*>& students,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist_class_size,
                                     dist_double_t& dist_school_size,
                                     region_t *prof_region,
                                     dist_double_t& dist_prof_age,
                                     double intra_class_ratio=1.0,
                                     double inter_class_ratio=0.025,
                                     report_progress_t *report = nullptr);

uint64_t get_n_population_per_relation_flag (std::bitset<NUMBER_OF_FLAGS>& flags);

inline uint64_t get_n_population_per_relation_flag (std::initializer_list<relation_type_t> list)
{
	std::bitset<NUMBER_OF_FLAGS> flags;

	for (relation_type_t type: list)
		flags.set(type);
	
	return get_n_population_per_relation_flag(flags);
}

void network_print_population_graph (std::bitset<NUMBER_OF_FLAGS>& flags);

inline pop_vertex_data_t& network_vertex_data (pop_vertex_t vertex)
{
	return (*pop_graph)[vertex];
}

inline pop_vertex_data_t& network_vertex_data (person_t *p)
{
	return (*pop_graph)[p->vertex];
}

inline pop_edge_data_t& network_edge_data (pop_edge_t edge)
{
	return (*pop_graph)[edge];
}

static void network_print_population_graph (std::initializer_list<relation_type_t> list)
{
	std::bitset<NUMBER_OF_FLAGS> flags;

	for (relation_type_t type: list)
		flags.set(type);
	
	network_print_population_graph(flags);
}

inline void network_print_population_graph ()
{
	std::bitset<NUMBER_OF_FLAGS> flags;

	flags.set();
	network_print_population_graph(flags);
}

void network_delete_edge (pop_vertex_t vertex1, pop_vertex_t vertex2);

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
static void network_create_connection_one_to_all (person_t *spreader, T& people, relation_type_t type, double ratio=1.0)
{
	for (auto it=people.begin(); it!=people.end(); ++it) {
		person_t *pi = *it;

		if (pi == nullptr)
			break;

		if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(spreader, pi) == false) {
			network_create_edge(spreader, pi, type);
		}
	}
}

template <typename T>
static void network_create_connection_between_people (T& people, relation_type_t type, double ratio=1.0, void (*callback)(pop_vertex_data_t& s, pop_vertex_data_t& t, pop_edge_data_t& e)=nullptr)
{
	for (auto it=people.begin(); it!=people.end(); ) {
		person_t *pi = *it;

		if (pi == nullptr)
			break;

		for (auto jt=++it; jt!=people.end(); ++jt) {
			person_t *pj = *jt;

			if (pj == nullptr)
				break;

			if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(pi, pj) == false) {
				pop_edge_t e = network_create_edge(pi, pj, type);

				if (callback != nullptr) {
					pop_vertex_t s = boost::source(e, *pop_graph);
					pop_vertex_t t = boost::target(e, *pop_graph);
					
					(*callback)(network_vertex_data(s), network_vertex_data(t), network_edge_data(e));
				}
			}
		}
	}
}

template <typename Ta, typename Tb>
static void network_create_connection_between_people (Ta& ga, Tb& gb, relation_type_t type, double ratio=1.0)
{
	for (person_t *pa: ga) {
		for (person_t *pb: gb) {
			if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(pa, pb) == false) {
				network_create_edge(pa, pb, type);
			}
		}
	}
}

#endif