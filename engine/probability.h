#ifndef __corona_probability_h__
#define __corona_probability_h__

#include <random>

extern std::mt19937_64 rgenerator;

void start_dice_engine ();
double generate_random_between_0_and_1 ();
int roll_dice (double probability);
void load_gdistribution_incubation (double mean, double stddev);
double calculate_incubation_cycles ();
person_t* pick_random_person ();
person_t* pick_random_person (state_t state);

template <typename Tweight, typename Tvalue>
static void adjust_weight_to_fit_mean (Tweight *orig_weights, Tvalue *values, double mean, double *adj_weights, uint32_t n)
{
	/*
		(k * (sum orig_weight*values) = mean
		k * (sum orig_weight*values) = mean
		k = mean * / (sum orig_weight*values)
	*/
#if 0
	double sum;
	
	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
		if (people_per_age[i])
			deaths_weight_per_age[i] = (double)reported_deaths_per_age[i] / (double)people_per_age[i];
		else
			deaths_weight_per_age[i] = 0.0;
		
		//sum += (double)deaths_weight_per_age[i] * (double)people_per_age[i];
		sum += (double)reported_deaths_per_age[i];
		
		dprintf("city %s people_per_age %s: " PU64 " death weight:%.4f\n", this->name.c_str(), critical_per_age_str(i), people_per_age[i], deaths_weight_per_age[i]);
	}

	/*
		(k * (sum weight*people_per_age) / total_people) = critical_rate
		k * (sum weight*people_per_age) = critical_rate * total_people
		k = (critical_rate * total_people) / (sum weight*people_per_age)
	*/

	k = (cfg.probability_critical * (double)this->get_npopulation()) / sum;

	dprintf("sum:%.4f k:%.4f\n", sum, k);

	sum = 0.0;
	for (i=0; i<AGE_CATS_N; i++) {
		deaths_weight_per_age[i] *= k;
#endif
}

#endif