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

template <typename Tweight, typename Tvalue, unsigned N>
static void adjust_weights_to_fit_mean (Tweight *orig_weights_, Tvalue *values, double mean, double *adj_weights)
{
	/*
		(k * (sum orig_weight*values) = mean
		k * (sum orig_weight*values) = mean
		k = mean * / (sum orig_weight*values)
	*/

	double orig_weights[N];
	double sum, k;
	uint32_t i;

	for (i=0; i<N; i++) {
		if (values[i] == 0)
			orig_weights[i] = 0.0;
		else
			orig_weights[i] = (double)orig_weights_[i];
	}
	
	sum = 0.0;
	for (i=0; i<N; i++) {
		if (values[i] != 0)
			adj_weights[i] = orig_weights[i] / (double)values[i];
		else
			adj_weights[i] = 0.0;
		
		//sum += (double)adj_weights[i] * (double)values[i];
		sum += (double)orig_weights[i];
	}

	k = mean / sum;

//	dprintf("sum:%.4f k:%.4f\n", sum, k);

	for (i=0; i<N; i++)
		adj_weights[i] *= k;
}

#endif