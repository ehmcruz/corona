#include <corona.h>

template <typename T, typename DISTRIB, unsigned BUFFER_SIZE>
class parallel_random_number_generator_t
{
	OO_ENCAPSULATE_RO(uint32_t, nt)

private:
	boost::lockfree::queue<T, boost::lockfree::fixed_sized<true>, boost::lockfree::capacity<BUFFER_SIZE>> queue;
	std::vector<std::thread*> workers;
	DISTRIB& distrib_;

public:
	parallel_random_number_generator_t (DISTRIB& distrib, uint32_t nt)
		: distrib_(distrib)
	{
		std::thread *t;

		exit(1);

		this->nt = nt;
		dprintf("number of threads: %u\n", this->nt);

		if (this->nt <= 1)
			this->nt = 0;

		for (uint32_t i; i<this->nt; i++) {
			//C_ASSERT( distrib == this->distrib_worker.back() )
			//std::cout << this->distrib_worker.back().mean() << " " << this->distrib_worker.back().stddev() << std::endl;
		#if 0
			t = new std::thread( [this, &distrib] {
				std::mt19937_64 rgenerator_worker;
				DISTRIB distrib_worker(distrib.param());
				T v;
				bool ok = true;
				std::random_device rd;

				rgenerator_worker.seed( rd() ); // generate entropy

				while (true) {
					if (ok)
						v = distrib_worker(rgenerator_worker);					

					//dprintf("%.2f\n", v);
					ok = this->queue.push(v);
				}
			});
		#endif
		}
	}

	parallel_random_number_generator_t (DISTRIB& distrib)
		: parallel_random_number_generator_t(distrib, std::thread::hardware_concurrency() - 1)
	{

	}


	T pop ()
	{
		T r;

		if (this->nt > 1) {
			bool ok;

			do {
				ok = this->queue.pop(r);
			} while (ok == false);
		}
		else
			r = this->distrib_(rgenerator);

		return r;
	}
};

void benchmark_random_engine ()
{
	const uint32_t max = 10000000;
	double r = 0.0; // avoid compiler opt
	std::chrono::steady_clock::time_point tbegin, tend;
	std::normal_distribution<double> test_dist(5, 1.5);

	cprintf("generating %u random numbers in serial...", max);
	
	parallel_random_number_generator_t<double, std::normal_distribution<double>, 20> test_serial(test_dist, 1);

	tbegin = std::chrono::steady_clock::now();

	for (uint32_t i=0; i<max; i++)
		r += test_serial.pop();

	tend = std::chrono::steady_clock::now();

	std::chrono::duration<double> time_serial = std::chrono::duration_cast<std::chrono::duration<double>>(tend - tbegin);

	cprintf("generating %u random numbers in parallel...", max);

	parallel_random_number_generator_t<double, std::normal_distribution<double>, 20> test_parallel(test_dist);

	tbegin = std::chrono::steady_clock::now();

	for (uint32_t i=0; i<max; i++)
		r += test_parallel.pop();

	tend = std::chrono::steady_clock::now();

	std::chrono::duration<double> time_parallel = std::chrono::duration_cast<std::chrono::duration<double>>(tend - tbegin);

	cprintf("r=%.2f\n", r);
	cprintf("time in serial: %.2fs\n", time_serial.count());
	cprintf("time in parallel: %.2fs\n", time_parallel.count());

	exit(1);
}