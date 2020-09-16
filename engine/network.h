#ifndef __corona_network_h__
#define __corona_network_h__

extern pop_graph_t *pop_graph;

void network_start_population_graph ();
void network_after_all_regular_connetions ();
double network_get_affective_r0 (std::bitset<NUMBER_OF_FLAGS>& flags);
double network_get_affective_r0_fast ();
void network_create_inter_city_relation (region_t *s, region_t *t, uint64_t n, relation_type_t type=RELATION_TRAVEL);
void network_iterate_over_edges (std::function<void (pop_vertex_t s, pop_vertex_t t, pop_edge_t e)> callback);
void network_iterate_over_vertices (std::function<void (pop_vertex_t v)> callback);
pop_edge_t network_get_edge (pop_vertex_t vertex1, pop_vertex_t vertex2);
void network_delete_edge (pop_vertex_t vertex1, pop_vertex_t vertex2);
void network_delete_edge (pop_edge_t e);

inline void network_delete_edge (person_t *s, person_t *t)
{
	network_delete_edge(s->vertex, t->vertex);
}

void network_create_school_relation (std::vector<person_t*>& students,
                                     uint32_t age_ini,
                                     uint32_t age_end,
                                     dist_double_t& dist_class_size,
                                     dist_double_t& dist_school_size,
                                     region_t *prof_region,
                                     dist_double_t& dist_prof_age,
                                     double intra_class_ratio=1.0,
                                     double inter_class_ratio=0.025,
                                     relation_type_t type_same_room=RELATION_SCHOOL,
                                     relation_type_t type_prof=RELATION_SCHOOL,
                                     relation_type_t type_other_room=RELATION_SCHOOL,
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

bool network_check_if_people_are_neighbors (pop_vertex_t vertex1, pop_vertex_t vertex2, pop_edge_t *edge=nullptr);

static inline bool network_check_if_people_are_neighbors (person_t *p1, person_t *p2, pop_edge_t *edge=nullptr)
{
	return network_check_if_people_are_neighbors(p1->vertex, p2->vertex, edge);
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
static void network_create_connection_one_to_all (person_t *spreader, T it_begin, T it_end, relation_type_t type, double ratio=1.0)
{
	for (auto it=it_begin; it!=it_end; ++it) {
		person_t *pi = *it;

		if (pi == nullptr)
			break;

		if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(spreader, pi) == false) {
			network_create_edge(spreader, pi, type);
		}
	}
}

template <typename T>
static void network_create_connection_one_to_all (person_t *spreader, T& people, relation_type_t type, double ratio=1.0)
{
	network_create_connection_one_to_all(spreader, people.begin(), people.end(), type, ratio);
}

template <typename T>
static void network_create_connection_between_people (T it_begin, T it_end, relation_type_t type, double ratio=1.0)
{
	for (auto it=it_begin; it!=it_end; ) {
		person_t *pi = *it;

		if (pi == nullptr)
			break;

		for (auto jt=++it; jt!=it_end; ++jt) {
			person_t *pj = *jt;

			if (pj == nullptr)
				break;

			if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(pi, pj) == false) {
				network_create_edge(pi, pj, type);
			}
		}
	}
}

template <typename T>
static void network_create_connection_between_people (T& people, relation_type_t type, double ratio=1.0)
{
	network_create_connection_between_people(people.begin(), people.end(), type, ratio);
}

template <typename Ta, typename Tb>
static void network_create_connection_between_2_groups (Ta& ga, Tb& gb, relation_type_t type, double ratio=1.0)
{
	for (person_t *pa: ga) {
		for (person_t *pb: gb) {
			if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(pa, pb) == false) {
				network_create_edge(pa, pb, type);
			}
		}
	}
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

template <typename T>
void network_create_clusters (T& people, dist_double_t& dist, std::initializer_list<relation_type_t> list_types, double ratio=1.0, report_progress_t *report=nullptr)
{
	uint32_t n, cluster_size, i, j;
	auto type_it = list_types.begin();

	C_ASSERT(list_types.size() > 0);

	for (n=0; n<people.size(); n+=cluster_size) {
		relation_type_t type;

		if (type_it == list_types.end())
			type_it = list_types.begin();

		type = *type_it;
		++type_it;

		cluster_size = (int32_t)(dist.generate() + 0.5); // round to nearest

		if (unlikely(cluster_size < 1))
			cluster_size = 1;

		if (unlikely((n+cluster_size) > people.size()))
			cluster_size = people.size() - n;

		// create a fully connected sub-graph for a family_size

		for (i=n; i<(n+cluster_size); i++) {
			for (j=i+1; j<(n+cluster_size); j++) {
				if ((ratio >= 1.0 || generate_random_between_0_and_1() <= ratio) && network_check_if_people_are_neighbors(people[i], people[j]) == false)
					network_create_edge(people[i], people[j], type);
			}
		}

		if (report != nullptr)
			report->check_report(cluster_size);
	}
}

template <typename T>
void network_create_clusters (T& people, dist_double_t& dist, relation_type_t type, double ratio=1.0, report_progress_t *report=nullptr)
{
	network_create_clusters(people, dist, {type}, ratio, report);
}

#endif