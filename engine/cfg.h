// primary cfg

CORONA_CFG(double, "%.4f", r0)
CORONA_CFG(double, "%.4f", cycles_contagious)
CORONA_CFG(double, "%.4f", cycles_to_simulate)

CORONA_CFG(double, "%.4f", cycles_incubation_mean)
CORONA_CFG(double, "%.4f", cycles_incubation_stddev)

CORONA_CFG(double, "%.4f", cycles_severe_in_hospital)
CORONA_CFG(double, "%.4f", cycles_critical_in_icu)
CORONA_CFG(double, "%.4f", cycles_before_hospitalization)
CORONA_CFG(double, "%.4f", global_r0_factor)
CORONA_CFG(double, "%.4f", probability_summon_per_cycle)

CORONA_CFG(double, "%.4f", death_rate_severe_in_hospital)
CORONA_CFG(double, "%.4f", death_rate_critical_in_hospital)
CORONA_CFG(double, "%.4f", death_rate_severe_outside_hospital)
CORONA_CFG(double, "%.4f", death_rate_critical_outside_hospital)

CORONA_CFG(double, "%.4f", family_size_mean)
CORONA_CFG(double, "%.4f", family_size_stddev)

CORONA_CFG(double, "%.4f", probability_asymptomatic)
CORONA_CFG(double, "%.4f", probability_mild)
CORONA_CFG(double, "%.4f", probability_critical)

CORONA_CFG(double, "%.4f", r0_asymptomatic_factor)

/***********************************************/

// derived cfg

CORONA_CFG(double, "%.4f", probability_infect_per_cycle)
CORONA_CFG(double, "%.4f", probability_severe)

CORONA_CFG(double, "%.4f", prob_ac_asymptomatic)
CORONA_CFG(double, "%.4f", prob_ac_mild)
CORONA_CFG(double, "%.4f", prob_ac_severe)
CORONA_CFG(double, "%.4f", prob_ac_critical)

CORONA_CFG(double, "%.4f", death_rate_severe_in_hospital_per_cycle)
CORONA_CFG(double, "%.4f", death_rate_critical_in_hospital_per_cycle)
CORONA_CFG(double, "%.4f", death_rate_severe_outside_hospital_per_cycle)
CORONA_CFG(double, "%.4f", death_rate_critical_outside_hospital_per_cycle)