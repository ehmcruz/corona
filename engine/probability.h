#ifndef __corona_probability_h__
#define __corona_probability_h__

#include <stdio.h>

#include <random>
#include <thread>

//#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/atomic.hpp>

#include <lib.h>

extern std::mt19937_64 rgenerator;

void generate_entropy ();
double generate_random_between_0_and_1 ();
bool roll_dice (double probability);
person_t* pick_random_person ();
person_t* pick_random_person (state_t state);

template <typename T>
class dist_t
{
public:
	virtual void print_params (FILE *fp) = 0;
	virtual T generate () = 0;
	virtual T get_expected () = 0;
	virtual T get_min () = 0;
	virtual T get_max () = 0;
};

template <typename T>
class dist_mmm_t: public dist_t<T>
{
protected:
	T min;
	T max;
	T mean;
	virtual T generate_ () = 0;

public:
	dist_mmm_t (T mean, T min, T max)
	{
		this->mean = mean;
		this->min = min;
		this->max = max;
	}

	T generate () override
	{
		T r;

		r = this->generate_();

		if (r < this->min)
			r = this->min;
		else if (r > this->max)
			r = this->max;

		return r;
	}

	T get_expected () override {
		return this->mean;
	}

	T get_min () override {
		return this->min;
	}
	
	T get_max () override {
		return this->max;
	}
};

typedef dist_t<double> dist_double_t;
typedef dist_mmm_t<double> dist_double_mmm_t;

/*******************************************/

class const_double_dist_t: public dist_double_t
{
private:
	double value;

public:
	const_double_dist_t (double value);

	double generate () override;
	void print_params (FILE *fp) override;
	double get_expected () override;
	double get_min () override;
	double get_max () override;
};

/*******************************************/

class normal_double_dist_t: public dist_double_mmm_t
{
	OO_ENCAPSULATE_RO(double, stddev)

private:
	std::normal_distribution<double> distribution;

protected:
	double generate_ () override;

public:
	normal_double_dist_t (double mean, double stddev, double min, double max);

	void print_params (FILE *fp) override;
};

/*******************************************/

class gamma_double_dist_t: public dist_double_mmm_t
{
	OO_ENCAPSULATE_RO(double, stddev)

private:
	std::gamma_distribution<double> distribution;

protected:
	double generate_ () override;

public:
	gamma_double_dist_t (double mean, double stddev, double min, double max);

	static inline double calc_alpha (double mean, double stddev) {
		return ( (mean*mean) / (stddev*stddev) );
	}

	static inline double calc_betha (double mean, double stddev) {
		return ( (stddev*stddev) / mean );
	}


	void print_params (FILE *fp) override;
};

/*******************************************/

template <typename Tweight, typename Tvalue>
struct adjust_values_to_fit_mean_t {
	Tweight weight;
	Tvalue value;
};

template <typename T>
static void adjust_values_to_fit_mean (T& v, double mean)
{
	/*
		(sum weights*values) = mean
	*/

	double sum, k;
	
	sum = 0.0;
	for (auto& e: v)
		sum += (double)e.weight;

	k = mean / sum;

//dprintf("sum:%.4f k:%.4f\n", sum, k);

	for (auto& e: v)
		e.value = ((double)e.weight * k);
}

template <typename Tweight, typename Tvalue>
struct adjust_weights_to_fit_mean_t {
	Tweight orig_weight;
	Tvalue value;
	double adj_weight;
};

template <typename Tweight, typename Tvalue>
static void adjust_weights_to_fit_mean (std::vector< adjust_weights_to_fit_mean_t<Tweight,Tvalue> >& v, double mean)
{
	/*
		(k * (sum orig_weight*values) = mean
		k * (sum orig_weight*values) = mean
		k = mean * / (sum orig_weight*values)
	*/

	double sum, k;
	uint32_t i;
	
	sum = 0.0;
	for (i=0; i<v.size(); i++)
		sum += (double)v[i].orig_weight * (double)v[i].value;

	k = mean / sum;

//dprintf("sum:%.4f k:%.4f\n", sum, k);

	for (i=0; i<v.size(); i++)
		v[i].adj_weight = (double)v[i].orig_weight * k;
}

template <typename Tweight, typename Tvalue, unsigned N>
static void adjust_weights_to_fit_mean (Tweight *orig_weights, Tvalue *values, double mean, double *adj_weights)
{
	/*
		(k * (sum orig_weight*values) = mean
		k * (sum orig_weight*values) = mean
		k = mean * / (sum orig_weight*values)
	*/

	double sum, k;
	uint32_t i;
	
	sum = 0.0;
	for (i=0; i<N; i++)
		sum += (double)orig_weights[i] * (double)values[i];

	k = mean / sum;

//dprintf("sum:%.4f k:%.4f\n", sum, k);

	for (i=0; i<N; i++)
		adj_weights[i] = (double)orig_weights[i] * k;
}

template <typename Tweight, typename Tvalue, unsigned N>
static void adjust_biased_weights_to_fit_mean (Tweight *orig_weights_, Tvalue *values, double mean, double *adj_weights)
{
	double orig_weights[N];
	uint32_t i;

	for (i=0; i<N; i++) {
		if (values[i] == 0)
			orig_weights[i] = 0.0;
		else
			orig_weights[i] = (double)orig_weights_[i] / (double)values[i];
	}

	adjust_weights_to_fit_mean<double, Tvalue, N>(
		orig_weights,
		values,
		mean, 
		adj_weights);
}

/****************************************************/



#endif