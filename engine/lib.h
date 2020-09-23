#ifndef __corona_lib_h__
#define __corona_lib_h__

#include <iostream>

#include <stdlib.h>

#define SANITY_CHECK
#define DEBUG

#define C_PRINTF_OUT stdout
#define cprintf(...) fprintf(C_PRINTF_OUT, __VA_ARGS__)
#define CMSG(STR) { std::cout << STR; }

#define C_ASSERT(V) C_ASSERT_PRINTF(V, "bye!\n")

#define C_ASSERT_PRINTF(V, ...) \
	{ if (unlikely(!(V))) { \
		cprintf("sanity error!\nfile %s at line %u assertion failed!\n%s\n", __FILE__, __LINE__, #V); \
		cprintf(__VA_ARGS__); \
		exit(1); \
	} }

#define C_ASSERT_P(V, STR) \
	{ if (unlikely(!(V))) { \
		std::cout << "sanity error!" << std::endl << "file " << __FILE__ << " at line " << __LINE__ << " assertion failed!" << std::endl << #V << std::endl; \
		std::cout << STR << std::endl; \
		exit(1); \
	} }

#ifdef SANITY_CHECK
	#define SANITY_ASSERT(V) C_ASSERT(V)
	#define SANITY_ASSERT_PRINTF(V, ...) C_ASSERT_PRINTF(V, __VA_ARGS__)
#else
	#define SANITY_ASSERT(V)
	#define SANITY_ASSERT_PRINTF(V, ...)
#endif

#ifdef DEBUG
	#define dprintf(...) cprintf(__VA_ARGS__)
	#define DMSG(STR) { std::cout << STR; }
#else
	#define dprintf(...)
	#define DMSG(STR)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define PU64 "%" PRIu64

#define NON_AC_STAT   0
#define AC_STAT       1

#define UNDEFINED32 0xFFFFFFFF

#define CREATE_PAIR_STC(NAME, TYPE_A, NAME_A, TYPE_B, NAME_B) \
	struct NAME { \
		TYPE_A NAME_A; \
		TYPE_B NAME_B; \
	};

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

#define OO_ENCAPSULATE_REFERENCE_RO(TYPE, VAR) \
	private: \
	TYPE VAR; \
	public: \
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

template <typename T>
inline T multiply_int_by_double (T v, double f)
{
	return static_cast<T>( static_cast<double>(v) * f );
}

template <typename T>
T** matrix_malloc (uint32_t nrows, uint32_t ncols)
{
	T **p;
	uint32_t i;
	
	p = (T**)calloc(nrows, sizeof(T*));
	C_ASSERT(p != NULL);

	p[0] = (T*)calloc(nrows*ncols, sizeof(T));
	C_ASSERT(p[0] != NULL);
	for (i=1; i<nrows; i++)
		p[i] = p[0] + i*ncols;
	
	return p;
}

template <typename T>
void matrix_free (T **m)
{
	free(m[0]);
	free(m);
}

class report_progress_t
{
private:
	uint64_t i;
	uint64_t n;
	uint64_t step;
	const char *msg;
public:
	inline report_progress_t (const char *msg, uint64_t n, uint64_t step, uint64_t ini = 0) {
		this->setup(msg, n, step, ini);
	}

	inline report_progress_t (const char *msg) {
		this->setup(msg, 0, 0);
	}

	inline void setup (const char *msg, uint64_t n, uint64_t step, uint64_t ini = 0) {
		this->msg = msg;
		this->setup(n, step, ini);
	}

	inline void setup (uint64_t n, uint64_t step, uint64_t ini = 0) {
		this->i = ini;
		this->n = n;
		this->step = step;
	}

	void check_report (uint64_t inc);
};

class person_t;

#endif