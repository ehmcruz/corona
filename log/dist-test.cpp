#include <random>
#include <stdio.h>

std::mt19937 rgenerator;

int main ()
{
	int32_t i, n=20;

	double mean = 3.5, stddev = 1.5;
	std::normal_distribution<double> ndistribution (mean, stddev);

	for (i=0; i<n; i++)
		printf("normal_distribution: %.2f\n", ndistribution(rgenerator));

	printf("-------------------------------------------\n");

	mean = 4.58;
	stddev = 3.24;

	double alpha, betha;
	betha = (stddev*stddev) / mean;
	alpha = mean / betha;

	std::gamma_distribution<double> gdistribution (alpha, betha);

	for (i=0; i<n; i++)
		printf("gamma_distribution alpha=%.2f betha=%.2f: %.2f\n", alpha, betha, gdistribution(rgenerator));

	std::uniform_real_distribution<double> rdistribution(0.0, 1.0);

	for (i=0; i<5; i++)
		printf("random_distribution: %.2f\n", rdistribution(rgenerator));

	return 0;
}