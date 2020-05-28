// primary cfg

CORONA_CFG(double, "%.4f", r0)
CORONA_CFG(double, "%.4f", cycles_contagious)
CORONA_CFG(double, "%.4f", cycles_to_simulate)

CORONA_CFG_OBJ(dist_double_t, cycles_incubation)

CORONA_CFG(double, "%.4f", cycles_severe_in_hospital)
CORONA_CFG(double, "%.4f", cycles_critical_in_icu)
CORONA_CFG(double, "%.4f", cycles_before_hospitalization)
CORONA_CFG(double, "%.4f", global_r0_factor)
CORONA_CFG(double, "%.4f", probability_summon_per_cycle)

CORONA_CFG(double, "%.4f", death_rate_severe_in_hospital)
CORONA_CFG(double, "%.4f", death_rate_critical_in_hospital)
CORONA_CFG(double, "%.4f", death_rate_severe_outside_hospital)
CORONA_CFG(double, "%.4f", death_rate_critical_outside_hospital)

CORONA_CFG(double, "%.4f", probability_asymptomatic)
CORONA_CFG(double, "%.4f", probability_mild)
CORONA_CFG(double, "%.4f", probability_critical)

CORONA_CFG(double, "%.4f", r0_asymptomatic_factor)

CORONA_CFG_VECTOR(uint32_t, "%u", relation_type, relation_type_weights, NUMBER_OF_RELATIONS)

CORONA_CFG(uint32_t, "%u", network_type)

CORONA_CFG(uint32_t, "%u", n_regions)

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

CORONA_CFG_VECTOR(uint64_t, PU64, relation_type, relation_type_number, NUMBER_OF_RELATIONS)
CORONA_CFG_VECTOR(double, "%.4f", relation_type, relation_type_transmit_rate, NUMBER_OF_RELATIONS)
