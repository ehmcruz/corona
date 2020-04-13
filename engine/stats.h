CORONA_STAT(uint64_t, PU64, deaths, NON_AC_STAT)
CORONA_STAT(uint64_t, PU64, infected, NON_AC_STAT)
CORONA_STAT(uint64_t, PU64, immuned, NON_AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, infected_state, NUMBER_OF_INFECTED_STATES, NON_AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, ac_infected_state, NUMBER_OF_INFECTED_STATES, AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, ac_critical_per_age, AGE_CATS_N, AC_STAT)

CORONA_STAT(uint64_t, PU64, ac_deaths, AC_STAT)
CORONA_STAT(uint64_t, PU64, ac_infected, AC_STAT)
CORONA_STAT(uint64_t, PU64, ac_immuned, AC_STAT)
CORONA_STAT(uint64_t, PU64, ac_healthy, AC_STAT)

CORONA_STAT(double, "%.4f", sir_s, NON_AC_STAT)
CORONA_STAT(double, "%.4f", sir_i, NON_AC_STAT)
CORONA_STAT(double, "%.4f", sir_r, NON_AC_STAT)

CORONA_STAT_VECTOR(uint64_t, PU64, critical_per_age, peak_critical_per_age, AGE_CATS_N, AC_STAT)
CORONA_STAT_VECTOR(uint64_t, PU64, infected_state, peak, NUMBER_OF_INFECTED_STATES, AC_STAT)