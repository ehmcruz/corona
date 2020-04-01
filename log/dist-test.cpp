#include <random>
#include <stdio.h>

std::mt19937 rgenerator;

int main ()
{
	int32_t i, n=50;

	std::normal_distribution<double> ndistribution (3.5, 1.5);

	for (i=0; i<n; i++)
		printf("normal_distribution: %.2f\n", ndistribution(rgenerator));

	return 0;
}