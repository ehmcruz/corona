#include <corona.h>

stats_t::stats_t (double cycle)
{
	this->reset();
	this->cycle = cycle;
}

void stats_t::reset ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) this->STAT = 0;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) this->STAT[i] = 0; }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR
}

void stats_t::copy_ac (stats_t *from)
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) if (AC == AC_STAT) this->STAT = from->STAT;
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) if (AC == AC_STAT) { int32_t i; for (i=0; i<N; i++) this->STAT[i] = from->STAT[i]; }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR
}

void stats_t::dump ()
{
	#define CORONA_STAT(TYPE, PRINT, STAT, AC) cprintf(#STAT ":" PRINT " ", this->STAT);
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) cprintf(#STAT ".%s:" PRINT " ", LIST##_str(i), this->STAT[i]); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	cprintf("\n");
}

void stats_t::dump_csv_header (FILE *fp)
{
	fprintf(fp, "cycle,");

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, #STAT ",");
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) fprintf(fp, #STAT "_%s,", LIST##_str(i)); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	fprintf(fp, "\n");
}

void stats_t::dump_csv (FILE *fp)
{
	fprintf(fp, "%.2f,", this->cycle);

	#define CORONA_STAT(TYPE, PRINT, STAT, AC) fprintf(fp, PRINT ",", this->STAT);
	#define CORONA_STAT_VECTOR(TYPE, PRINT, LIST, STAT, N, AC) { int32_t i; for (i=0; i<N; i++) fprintf(fp, PRINT ",", this->STAT[i]); }
	#include "stats.h"
	#undef CORONA_STAT
	#undef CORONA_STAT_VECTOR

	fprintf(fp, "\n");
}