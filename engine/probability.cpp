#include <random>

#include <cmath>

#include <corona.h>

std::mt19937_64 rgenerator;

static std::uniform_real_distribution<double> rdistribution(0.0, 1.0);

void generate_entropy ()
{
	std::random_device rd;

	rgenerator.seed( rd() );
}

double generate_random_between_0_and_1 ()
{
	//return ( (double)rand() / (double)RAND_MAX );
	return rdistribution(rgenerator);
}

/*
	probability between 0.0 and 1.0
*/

bool roll_dice (double probability)
{
	return (generate_random_between_0_and_1() <= probability);
}

person_t* pick_random_person ()
{
	std::uniform_int_distribution<uint64_t> distribution(0, population.size()-1);

	return population[ distribution(rgenerator) ];
}

person_t* pick_random_person (state_t state)
{
	uint64_t total = 0;
	person_t *p = nullptr;

	for (person_t *p: population) {
		total += (p->get_state() == state);
	}

	if (likely(total > 0)) {
		uint64_t random;

		std::uniform_int_distribution<uint64_t> distribution(0, total-1);

		random = distribution(rgenerator);

		for (auto it=population.begin(); it!=population.end(); ++it) {
			if ((*it)->get_state() == state) {
				if (unlikely(random == 0)) {{
					p = *it;
					break;
				}
	
				random--;}
			}
		}
	}

	C_ASSERT((total == 0 && p == nullptr) || (total > 0 && p->get_state() == state))

	return p;
}


/*
probability to die = death_rate_condition_in_hospital

probability to die = 1 - (probability to live_per_cycle)^n

probability to live_per_cycle = (1 - death_rate_condition_in_hospital_per_cycle)

probability to die = 1 - (1 - death_rate_condition_in_hospital_per_cycle)^n
1 - (1 - death_rate_condition_in_hospital_per_cycle)^n = probability to die
- (1 - death_rate_condition_in_hospital_per_cycle)^n = probability to die - 1
(1 - death_rate_condition_in_hospital_per_cycle)^n = 1 - probability to die
1 - death_rate_condition_in_hospital_per_cycle = (1 - probability to die)^(1/n)
- death_rate_condition_in_hospital_per_cycle = (1 - probability to die)^(1/n) - 1
death_rate_condition_in_hospital_per_cycle = 1 - (1 - probability to die)^(1/n)
*/

double calc_death_probability_per_cycle_step (double death_prob, double n_tries)
{
	double r = 1.0 - pow(1 - death_prob, 1.0 / n_tries);

//DMSG("death prob " << death_prob << " r " << r << std::endl)

	return r;
}

/**************************************************/

const_double_dist_t::const_double_dist_t (double value)
{
	this->value = value;
}

double const_double_dist_t::generate ()
{
	return this->value;
}

double const_double_dist_t::get_expected ()
{
	return this->value;
}

double const_double_dist_t::get_min ()
{
	return this->value;
}

double const_double_dist_t::get_max ()
{
	return this->value;
}

void const_double_dist_t::print_params (FILE *fp)
{
	fprintf(fp, "%.2f", this->value);
}


/**************************************************/

normal_double_dist_t::normal_double_dist_t (double mean, double stddev, double min, double max)
	: dist_double_mmm_t(mean, min, max),
	  distribution(mean, stddev)
{
	this->stddev = stddev;
}

double normal_double_dist_t::generate_ ()
{
//this->print_params(stdout); for (int i=0; i<100; i++) cprintf(" %.2f", this->distribution(rgenerator)); cprintf("\n"); exit(1);
	return this->distribution(rgenerator);
}

void normal_double_dist_t::print_params (FILE *fp)
{
	fprintf(fp, "mean(%.2f) stddev(%.2f)", this->mean, this->stddev);
}

/**************************************************/

lognormal_double_dist_t::lognormal_double_dist_t (double mean, double stddev, double min, double max)
	: dist_double_mmm_t(mean, min, max),
	  distribution(calc_log_mean(mean, stddev), calc_log_stddev(mean, stddev))
{
	this->stddev = stddev;
}

double lognormal_double_dist_t::calc_log_mean (double mean, double stddev)
{
	// mX    =  2 * math.log(mY) - math.log(sigY ELEVADO 2 + mY ELEVADO 2) / 2
	return 2.0 * log(mean) - log(stddev*stddev + mean*mean) / 2.0;
}

double lognormal_double_dist_t::calc_log_stddev (double mean, double stddev)
{
	// sigX2 = -2 * math.log(mY) + math.log(sigY ELEVADO 2 + mY ELEVADO 2)
	// sigX = math.sqrt(sigX2)
	return sqrt( -2.0 * log(mean) + log(stddev*stddev + mean*mean) );
}

double lognormal_double_dist_t::generate_ ()
{
//this->print_params(stdout); for (int i=0; i<100; i++) cprintf(" %.2f", this->distribution(rgenerator)); cprintf("\n"); exit(1);
	return this->distribution(rgenerator);
}

void lognormal_double_dist_t::print_params (FILE *fp)
{
	fprintf(fp, "mean(%.2f) stddev(%.2f)", this->mean, this->stddev);
}

/**************************************************/

gamma_double_dist_t::gamma_double_dist_t (double mean, double stddev, double min, double max)
	: dist_double_mmm_t(mean, min, max),
	  distribution(calc_alpha(mean, stddev), calc_betha(mean, stddev))
{
	this->stddev = stddev;
}

double gamma_double_dist_t::generate_ ()
{
//this->print_params(stdout); for (int i=0; i<100; i++) cprintf(" %.2f", this->distribution(rgenerator)); cprintf("\n"); exit(1);
	return this->distribution(rgenerator);
}

void gamma_double_dist_t::print_params (FILE *fp)
{
	fprintf(fp, "mean(%.2f) stddev(%.2f) alpha(%.2f) betha(%.2f)", this->mean, this->stddev, this->calc_alpha(this->mean, this->stddev), this->calc_betha(this->mean, stddev));
}