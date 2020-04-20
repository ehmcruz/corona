#include <corona.h>

neighbor_list_t::neighbor_list_t (person_t *p)
{
	this->person = p;
}

neighbor_list_t::iterator_t::iterator_t ()
{
	this->ptr_check_probability = &neighbor_list_t::iterator_t::check_probability_;
	this->ptr_get_person = &neighbor_list_t::iterator_t::get_person_;
	this->ptr_next = &neighbor_list_t::iterator_t::next_;
}

bool neighbor_list_t::iterator_t::check_probability_ ()
{
	C_ASSERT(0)
	return false;
}

person_t* neighbor_list_t::iterator_t::get_person_ ()
{
	C_ASSERT(0)
	return nullptr;
}

neighbor_list_t::iterator_t& neighbor_list_t::iterator_t::next_ ()
{
	C_ASSERT(0)
	return *this;
}

// -----------------------------------------------------------------------

neighbor_list_t::iterator_t neighbor_list_fully_connected_t::begin ()
{
	iterator_fully_connected_t it;

	if (likely(population.size() > 0)) {
		it.prob = (cfg.probability_infect_per_cycle * cfg.global_r0_factor
		        * r0_factor_per_group[ this->get_person()->get_infected_state() ]);
		
		it.calc();
	}
	else
		it.current = nullptr;

	return it;
}

neighbor_list_fully_connected_t::iterator_fully_connected_t::iterator_fully_connected_t ()
{
	static_assert(sizeof(neighbor_list_fully_connected_t::iterator_fully_connected_t) == sizeof(neighbor_list_t::iterator_t));

	this->ptr_check_probability = (prob_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::check_probability_;
	this->ptr_get_person = (person_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::get_person_;
	this->ptr_next = (next_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::next_;

	this->current = nullptr;
}

void neighbor_list_fully_connected_t::iterator_fully_connected_t::calc ()
{
	if (this->prob > 0.0) {
		double p;
		bool end = false;

		do {
			if (this->prob <= 1.0) {
				p = this->prob;
				this->prob = 0.0;
			}
			else {
				p = 1.0;
				this->prob -= 1.0;
			}

			if (roll_dice(p)) { // ok, I will infect someone, let's find someone
				person_t *p = pick_random_person();

				if (p->get_state() == ST_HEALTHY) {
					this->current = p;
					end = true;
				}
				else if (this->prob == 0.0) { // The person is not suscetible, so I won't infect, and I'm out of probability to try again
					this->current = nullptr;
					end = true;
				}
			}
			else if (this->prob == 0.0) { // I won't infect and I'm out of probability to try again
				this->current = nullptr;
				end = true;
			}
		} while (!end);
	}
	else
		this->current = nullptr;
}

bool neighbor_list_fully_connected_t::iterator_fully_connected_t::check_probability_ ()
{
	return (this->current != nullptr);
}

person_t* neighbor_list_fully_connected_t::iterator_fully_connected_t::get_person_ ()
{
	return this->current;
}

neighbor_list_t::iterator_t& neighbor_list_fully_connected_t::iterator_fully_connected_t::next_ ()
{
	this->calc();

	return *this;
}