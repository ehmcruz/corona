#ifndef __corona_lib_h__
#define __corona_lib_h__

#define SANITY_CHECK
#define DEBUG

#define C_PRINTF_OUT stdout
#define cprintf(...) fprintf(C_PRINTF_OUT, __VA_ARGS__)

#define C_ASSERT(V) C_ASSERT_PRINTF(V, "bye!\n")

#define C_ASSERT_PRINTF(V, ...) \
	if (unlikely(!(V))) { \
		cprintf("sanity error!\nfile %s at line %u assertion failed!\n%s\n", __FILE__, __LINE__, #V); \
		cprintf(__VA_ARGS__); \
		exit(1); \
	}

#ifdef SANITY_CHECK
	#define SANITY_ASSERT(V) C_ASSERT(V)
#else
	#define SANITY_ASSERT(V)
#endif

#ifdef DEBUG
	#define dprintf(...) cprintf(__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PU64 "%" PRIu64

#define NON_AC_STAT   0
#define AC_STAT       1

#define OO_ENCAPSULATE(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
	inline void set_##VAR (TYPE VAR) { \
		this->VAR = VAR; \
	} \
	inline TYPE get_##VAR () { \
		return this->VAR; \
	}

#define OO_ENCAPSULATE_REFERENCE(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
	inline void set_##VAR (TYPE& VAR) { \
		this->VAR = VAR; \
	} \
	inline TYPE& get_##VAR () { \
		return this->VAR; \
	}

// read-only
#define OO_ENCAPSULATE_RO(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
	inline TYPE get_##VAR () { \
		return this->VAR; \
	}

class person_t;
class neighbor_list_network_t;

class neighbor_list_t
{
public:
	typedef std::pair<double, person_t*> pair_t;

	class iterator_t {
		friend class neighbor_list_t;
		friend class neighbor_list_fully_connected_t;
		friend class neighbor_list_network_t;
	private:
		bool check_probability_ ();
		person_t* get_person_ ();
		iterator_t& next_ ();
	protected:
		person_t *current;
		double prob;
		neighbor_list_network_t *list;

		typedef bool (neighbor_list_t::iterator_t::*prob_func_t)();
		typedef person_t* (neighbor_list_t::iterator_t::*person_func_t)();
		typedef iterator_t& (neighbor_list_t::iterator_t::*next_func_t)();

		prob_func_t ptr_check_probability;
		person_func_t ptr_get_person;
		next_func_t ptr_next;

	public:
		iterator_t();

		inline bool check_probability () {
			return (this->*ptr_check_probability)();
		}

		inline person_t* operator* () {
			return (this->*ptr_get_person)();
		}

		inline iterator_t& operator++ () {
			return (this->*ptr_next)();
		}
	};

private:
	OO_ENCAPSULATE_RO(person_t*, person)

public:
	neighbor_list_t (person_t *p);

	virtual iterator_t begin () = 0;
};

class neighbor_list_fully_connected_t: public neighbor_list_t
{
public:
	class iterator_fully_connected_t: public neighbor_list_t::iterator_t {
		friend class neighbor_list_fully_connected_t;
	private:
		bool check_probability_ ();
		person_t* get_person_ ();
		iterator_t& next_ ();

		void calc ();
	public:
		iterator_fully_connected_t ();
	};

public:
	using neighbor_list_t::neighbor_list_t;
	iterator_t begin () override;
};

class neighbor_list_network_t: public neighbor_list_t
{
public:
	class iterator_network_t: public neighbor_list_t::iterator_t {
		friend class neighbor_list_network_t;
	private:
		bool check_probability_ ();
		person_t* get_person_ ();
		iterator_t& next_ ();

		void calc ();
	public:
		iterator_network_t ();
	};

private:
	std::vector< pair_t > connected;
	uint32_t nconnected;

	person_t* pick_random_person ();

public:
	using neighbor_list_t::neighbor_list_t;
	iterator_t begin () override;
};

#endif