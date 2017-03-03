// Core/Percentage.hpp

#ifndef _CORE_PERCENTAGE_HPP_INCLUDED_
#define _CORE_PERCENTAGE_HPP_INCLUDED_

#include <cmath>
#include <algorithm>

namespace Core
{
	inline double RoundWithPrecision(double value, unsigned precision)
	{
		double multiplier = std::pow(10.0, static_cast<double>(precision));
		return (round(value * multiplier) / multiplier);
	}

	inline double Percent(double value, unsigned precision)
	{
		return RoundWithPrecision(value * 100.0, precision);
	}

	inline double Percent(double value)
	{
		unsigned precision = std::max(0, static_cast<int>(ceil(-log10(value) - 1)));
		return Percent(value, precision);
	}
}

#endif