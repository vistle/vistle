#include "dataAdaptor.h"
#include <cassert>
#include <algorithm>
using namespace vistle::insitu;

Array::~Array() {
	if (owner == Owner::Vistle)
	{
		free(data);
	}
}

size_t Mesh::numVerts() const {
	if (interleaved)
	{
		size_t s = 1;
		for (size_t i = 0; i < 3; i++)
		{
			s *= std::max(sizes[0], (size_t)1);
		}
		assert(data[0].size * static_cast<int>(dim) == s);
		return s;
	}
	else
	{
		assert(data[0].size == data[1].size && data[0].size == data[2].size);
		return data[0].size;
	}
}