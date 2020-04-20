#include <corona.h>

neighbor_list_t::neighbor_list_t (person_t *p)
{
	this->p = p;
}

neighbor_list_t::iterator_t::iterator_t ()
{
	this->ptr_get_probability = &neighbor_list_t::iterator_t::get_probability_;
	this->ptr_get_person = &neighbor_list_t::iterator_t::get_person_;
	this->ptr_next = &neighbor_list_t::iterator_t::next_;
}

double neighbor_list_t::iterator_t::get_probability_ ()
{
	C_ASSERT(0)
	return 0.0;
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
		it.current = population[0];
		it.pos = 0;
	}
	else
		it.current = nullptr;

	return it;
}

neighbor_list_fully_connected_t::iterator_fully_connected_t::iterator_fully_connected_t ()
{
	static_assert(sizeof(neighbor_list_fully_connected_t::iterator_fully_connected_t) == sizeof(neighbor_list_t::iterator_t));

	this->ptr_get_probability = (prob_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::get_probability_;
	this->ptr_get_person = (person_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::get_person_;
	this->ptr_next = (next_func_t) &neighbor_list_fully_connected_t::iterator_fully_connected_t::next_;

	this->current = nullptr;
}

double neighbor_list_fully_connected_t::iterator_fully_connected_t::get_probability_ ()
{
	return 0.0;
}

person_t* neighbor_list_fully_connected_t::iterator_fully_connected_t::get_person_ ()
{
	return this->current;
}

neighbor_list_t::iterator_t& neighbor_list_fully_connected_t::iterator_fully_connected_t::next_ ()
{
	if (likely(this->current != nullptr)) {
		this->pos++;
		if (likely(this->pos < population.size()))
			this->current = population[ this->pos ];
		else
			this->current = nullptr;
	}

	return *this;
}