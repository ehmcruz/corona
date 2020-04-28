#ifndef __corona_probability_h__
#define __corona_probability_h__

#include <stdio.h>

#include <random>

#include <lib.h>

extern std::mt19937_64 rgenerator;

void start_dice_engine ();
double generate_random_between_0_and_1 ();
int roll_dice (double probability);
void load_gdistribution_incubation (double mean, double stddev);
double calculate_incubation_cycles ();
person_t* pick_random_person ();
person_t* pick_random_person (state_t state);

template <typename T>
class dist_t
{
	OO_ENCAPSULATE(T, min)
	OO_ENCAPSULATE(T, max)

protected:
	virtual T generate_ () = 0;

public:
	dist_t (T min, T max)
	{
		this->min = min;
		this->max = max;
	}

	virtual void print_params (FILE *fp) = 0;

	inline T generate ()
	{
		T r;

		r = this->generate_();

		if (r < this->min)
			r = this->min;
		else if (r > this->max)
			r = this->max;

		return r;
	}
};

typedef dist_t<double> dist_double_t;

class normal_double_dist_t: public dist_double_t
{
	OO_ENCAPSULATE(double, mean)
	OO_ENCAPSULATE(double, stddev)

private:
	std::uniform_real_distribution<double> distribution;

protected:
	double generate_ () override;

public:
	normal_double_dist_t (double mean, double stddev, double min, double max);

	void print_params (FILE *fp) override;
};

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