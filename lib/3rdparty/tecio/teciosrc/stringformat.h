 #pragma once
#include "ThirdPartyHeadersBegin.h"
 #if __cplusplus >= 201703L
#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
 #endif
#include <iomanip>
#include <stdio.h>
#include <sstream>
#include <string>
#include "ThirdPartyHeadersEnd.h"
namespace tecplot { template <typename T> std::string ___4185( T   value, int precision = 6) { std::stringstream ss; ss << std::setprecision(precision) << value; return ss.str(); } } namespace tecplot { template <typename T> std::string toString(T value) { return tecplot::___4185(value); } } template <typename T> std::string ___4185(T value) { return tecplot::___4185(value); } namespace tecplot { inline std::string doubleToFullPrecisionFormattedStr(double value) { return tecplot::___4185(value, 15); } }
 #if __cplusplus >= 201703L
template<typename T> [[nodiscard]] std::string joinElements(std::vector<T> const& elements, std::function<std::string(size_t elemID)> const& delimiterProvider) { std::ostringstream joinedElementString; size_t elemCount{0}; std::transform(elements.begin(), elements.end(), std::ostream_iterator<std::string>(joinedElementString), [&](T const& ___2083) { std::string const itemStr{[&___2083] { if constexpr (std::is_same<T, std::string>::value) return ___2083; else return tecplot::___4185(___2083); }() }; std::string const delimeter{elemCount ? delimiterProvider(elemCount) : ""}; ++elemCount; return delimeter + itemStr; }); return joinedElementString.str(); }
 #endif
