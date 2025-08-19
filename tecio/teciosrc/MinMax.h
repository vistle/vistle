 #pragma once
#include "ThirdPartyHeadersBegin.h"
#include <algorithm>
#include "ThirdPartyHeadersEnd.h"
#include "CodeContract.h"
class ___2477 : public std::pair<double, double> {
 #define INVALID_MINMAX_MIN_VALUE (10.*___2177) 
 #define INVALID_MINMAX_MAX_VALUE (-10.*___2177)
public: ___2477() { invalidate(); } ___2477(double newMin, double newMax) { first  = newMin; second = newMax; } explicit ___2477(double ___4296) { first  = ___4296; second = first; } inline void swap(___2477& ___2886) { using std::swap; swap(first, ___2886.first); swap(second, ___2886.second); } inline double minValue(void) const { return first; } inline double maxValue(void) const { return second; } inline void ___3497(double newMin, double newMax) { REQUIRE(newMin <= newMax); first  = newMin; second = newMax; } inline void ___3497(___2477 const& ___2886) { first  = ___2886.first; second = ___2886.second; } inline void include(double ___4296) { first  = std::min(first, ___4296); second = std::max(second, ___4296); } inline void include(___2477 const& minMax) { REQUIRE(minMax.___2065()); first  = std::min(first, minMax.first); second = std::max(second, minMax.second); } inline bool containsValue(double ___4296) const { return ( first <= ___4296 && ___4296 <= second ); } inline void invalidate(void) { first  = INVALID_MINMAX_MIN_VALUE; second = INVALID_MINMAX_MAX_VALUE; } inline bool ___2065(void) const { REQUIRE(*(uint64_t *)&first != 0xcdcdcdcdcdcd || *(uint64_t *)&second != 0xcdcdcdcdcdcd); bool const valid = (first <= second); ___476(IMPLICATION(!valid, first==INVALID_MINMAX_MIN_VALUE && second==INVALID_MINMAX_MAX_VALUE)); return valid; } };
