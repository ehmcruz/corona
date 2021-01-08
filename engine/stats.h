CORONA_STAT_VECTOR(uint64_t, PU64, state, state, NUMBER_OF_STATES, NON_AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, state, ac_state, NUMBER_OF_STATES, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, infected_state, NUMBER_OF_INFECTED_STATES, NON_AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, ac_infected_state, NUMBER_OF_INFECTED_STATES, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_ac_critical, AGE_CATS_N, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_peak_critical, AGE_CATS_N, AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, peak, NUMBER_OF_INFECTED_STATES, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_total_infected, AGE_CATS_N, AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_total_reported_infected, AGE_CATS_N, AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_total_critical, AGE_CATS_N, AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, per_age_total_deaths, AGE_CATS_N, AC_STAT)

CORONA_STAT_OBJ(stats_obj_mean_t, r)

CORONA_STAT(uint64_t, PU64, reported, NON_AC_STAT)
CORONA_STAT(uint64_t, PU64, ac_total_reported, AC_STAT)

CORONA_STAT(uint64_t, PU64, reported_recovered, NON_AC_STAT)
CORONA_STAT(uint64_t, PU64, ac_total_reported_recovered, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, ac_total_infected_state, NUMBER_OF_INFECTED_STATES, AC_STAT)