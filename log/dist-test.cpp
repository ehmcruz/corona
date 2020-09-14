#include <random>
#include <iostream>

#include <boost/random.hpp>

std::mt19937 rgenerator;

int main ()
{
	int32_t i, n=20;

	double mean = 3.5, stddev = 1.5;
	std::normal_distribution<double> ndistribution (mean, stddev);

	for (i=0; i<n; i++)
		std::cout << "normal_distribution: " << ndistribution(rgenerator) << std::endl;

	std::cout << "-------------------------------------------" << std::endl;

	mean = 4.58;
	stddev = 3.24;

	double alpha, betha;
	betha = (stddev*stddev) / mean;
	alpha = mean / betha;

	std::gamma_distribution<double> gdistribution (alpha, betha);

	for (i=0; i<n; i++)
		std::cout << "gamma_distribution alpha=" << alpha << " betha=" << betha << ": " << gdistribution(rgenerator) << std::endl;

	std::uniform_real_distribution<double> udistribution(0.0, 1.0);

	for (i=0; i<5; i++)
		std::cout << "uniform_distribution: " << udistribution(rgenerator) << std::endl;

	std::lognormal_distribution<double> lndistribution(2.55, 1.66);

	for (i=0; i<20; i++)
		std::cout << "lognormal_distribution: " << lndistribution(rgenerator) << std::endl;

/*	boost::lognormal_distribution<double> blndistribution(0.75, 12.4);

	for (i=0; i<20; i++)
		std::cout << "boost_lognormal_distribution: " << blndistribution(rgenerator) << std::endl;
*/

	return 0;
}