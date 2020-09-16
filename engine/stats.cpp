#include <corona.h>

stats_obj_mean_t::stats_obj_mean_t ()
{
	this->reset();
}

void stats_obj_mean_t::reset ()
{
	this->sum = 0.0;
	this->n = 0;
}

void stats_obj_mean_t::print (FILE *fp)
{
	fprintf(fp, "%.4f", this->calc());
}

void stats_obj_mean_t::print (std::ostream& out)
{
	out << this->calc();
}

void stats_obj_mean_t::print ()
{
	this->print(std::cout);
}

/****************************************************/

void global_stats_t::print (std::ostream& out)
{
	#define myprint(var) \
		out <<  "global_stats." #var ": "; \
		this->var.print(out); \
		out << std::endl;

	myprint(cycles_between_generations)
	myprint(cycles_critical)
	myprint(cycles_severe)

	#undef myprint
}

void global_stats_t::print ()
{
	this->print(std::cout);
}

/****************************************************/

stats_t::stats_t ()
{
	this->reset();
	this->cycle = current_cycle;
}

void stats_t::reset ()
{
	this->n = 0;

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) this->STAT = 0; this->n++;
	#define CORONA_STAT_OBJ(TYPE, STAT) this->STAT.reset(); this->n++;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) { this->STAT[i] = 0; this->n++; } }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_OBJ
	#undef CORONA_STAT_VECTOR
}

void stats_t::copy_ac (stats_t *from)
{
	this->zone = from->zone;

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) if (AC == AC_STAT) this->STAT = from->STAT;
	#define CORONA_STAT_OBJ(TYPE, STAT)
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) if (AC == AC_STAT) { int32_t i; for (i=0; i<N; i++) this->STAT[i] = from->STAT[i]; }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_OBJ
	#undef CORONA_STAT_VECTOR
}

void stats_t::dump ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) cprintf(#STAT ":" PRINT " ", this->STAT);
	#define CORONA_STAT_OBJ(TYPE, STAT) cprintf(#STAT ":"); this->STAT.print(stdout); cprintf(" ");
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) cprintf(#STAT ".%s:" PRINT " ", LIST##_str(i), this->STAT[i]); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_OBJ
	#undef CORONA_STAT_VECTOR

	cprintf("\n");
}

void stats_t::dump_csv_header (FILE *fp)
{
	uint32_t j = 0;

	fprintf(fp, "cycle,");

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, #STAT); if (++j < this->n) { fprintf(fp, ","); }
	#define CORONA_STAT_OBJ(TYPE, STAT) fprintf(fp, #STAT); if (++j < this->n) { fprintf(fp, ","); }
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) { fprintf(fp, #STAT "_%s", LIST##_str(i)); if (++j < this->n) { fprintf(fp, ","); } } }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_OBJ
	#undef CORONA_STAT_VECTOR

	fprintf(fp, "\n");
}

void stats_t::dump_csv (FILE *fp)
{
	uint32_t j = 0;

	fprintf(fp, "%.2f,", this->cycle);

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, PRINT, this->STAT); if (++j < this->n) { fprintf(fp, ","); }
	#define CORONA_STAT_OBJ(TYPE, STAT) this->STAT.print(fp); if (++j < this->n) { fprintf(fp, ","); }
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) { fprintf(fp, PRINT, this->STAT[i]); if (++j < this->n) { fprintf(fp, ","); } } }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_OBJ
	#undef CORONA_STAT_VECTOR

	fprintf(fp, "\n");
}

stats_zone_t::stats_zone_t ()
{
	static uint32_t number_of_stats = 0;

	this->sid = number_of_stats++;
	this->population_size = 0;
}

void stats_zone_t::add_person (person_t *p)
{
	SANITY_ASSERT(p->get_state() == ST_HEALTHY)

	this->population_size++;
	p->add_sid(this->sid);
}
