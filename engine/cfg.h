// primary cfg

CORONA_CFG(double, "%.4f", r0)

CORONA_CFG_OBJ(dist_double_t, cycles_contagious)
CORONA_CFG_OBJ(dist_double_t, cycles_pre_symptomatic)

CORONA_CFG(double, "%.4f", cycles_to_simulate)
CORONA_CFG(double, "%.4f", cycle_division)

CORONA_CFG_OBJ(dist_double_t, cycles_incubation)

CORONA_CFG_OBJ(dist_double_t, cycles_severe_in_hospital)
CORONA_CFG_OBJ(dist_double_t, cycles_critical_in_icu)
CORONA_CFG_OBJ(dist_double_t, cycles_before_hospitalization)

CORONA_CFG(double, "%.4f", global_r0_factor)
CORONA_CFG(double, "%.4f", probability_summon_per_cycle_)

CORONA_CFG(double, "%.4f", death_rate_severe_in_hospital_)
CORONA_CFG(double, "%.4f", death_rate_critical_in_icu_)
CORONA_CFG(double, "%.4f", death_rate_severe_outside_hospital_)
CORONA_CFG(double, "%.4f", death_rate_critical_outside_icu_)

CORONA_CFG(double, "%.4f", probability_asymptomatic)
CORONA_CFG(double, "%.4f", probability_mild)
CORONA_CFG(double, "%.4f", probability_critical)

CORONA_CFG_VECTOR(double, "%.4f", relation_type, relation_type_weights, NUMBER_OF_RELATIONS)

CORONA_CFG_VECTOR(double, "%.4f", infected_state, r0_factor_per_group, NUMBER_OF_INFECTED_STATES)

CORONA_CFG_VECTOR(double, "%.4f", critical_per_age, r0_factor_per_age_cat, AGE_CATS_N)

CORONA_CFG_MATRIX(double, "%.4f", factor_per_relation_group_str, factor_per_relation_group, NUMBER_OF_RELATIONS, NUMBER_OF_INFECTED_STATES)

CORONA_CFG(uint32_t, "%u", network_type)

CORONA_CFG(uint32_t, "%u", n_regions)

/***********************************************/

// derived cfg

CORONA_CFG(double, "%.4f", step)

CORONA_CFG(double, "%.4f", probability_infect_per_cycle_step)
CORONA_CFG(double, "%.4f", probability_severe)

CORONA_CFG(double, "%.4f", probability_summon_per_cycle_step)

CORONA_CFG(double, "%.4f", prob_ac_asymptomatic)
CORONA_CFG(double, "%.4f", prob_ac_mild)
CORONA_CFG(double, "%.4f", prob_ac_severe)
CORONA_CFG(double, "%.4f", prob_ac_critical)

CORONA_CFG_VECTOR(uint64_t, PU64, relation_type, relation_type_number, NUMBER_OF_RELATIONS)
CORONA_CFG_VECTOR(double, "%.4f", relation_type, relation_type_transmit_rate, NUMBER_OF_RELATIONS)
