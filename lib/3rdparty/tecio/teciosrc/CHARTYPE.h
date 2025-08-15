 #ifndef TECPLOT_CHARTYPE
 #define TECPLOT_CHARTYPE
#include <cctype>
#include <locale>
 #if defined EXTERN
 #undef EXTERN
 #endif
 #if defined ___475
 #define EXTERN
 #else
 #define EXTERN extern
 #endif
#include "CodeContract.h"
namespace tecplot { template <typename CHAR_TYPE> inline bool isspace(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::isspace(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::space, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2048(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2048(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::print, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2007(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2007(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::cntrl, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool isupper(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::isupper(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::upper, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2030(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2030(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::lower, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___1996(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___1996(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::alpha, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2010(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2010(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::digit, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2050(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2050(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::punct, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2082(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2082(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::xdigit, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___1995(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___1995(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::alnum, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline bool ___2022(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return ::___2022(static_cast<int>(ch));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return std::use_facet<std::ctype<wchar_t> >(___2311).is(std::ctype_base::graph, static_cast<wchar_t>(ch));
 #endif
} template <typename CHAR_TYPE> inline CHAR_TYPE toupper(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return static_cast<CHAR_TYPE>(::toupper(static_cast<int>(ch)));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return static_cast<CHAR_TYPE>( std::use_facet<std::ctype<wchar_t> >(___2311).toupper(static_cast<wchar_t>(ch)));
 #endif
} template <typename CHAR_TYPE> inline CHAR_TYPE tolower(CHAR_TYPE          ch, std::locale const& ___2311 = std::locale::classic()) {
 #if defined ___3891
REQUIRE(___2311 == std::locale::classic()); return static_cast<CHAR_TYPE>(::tolower(static_cast<int>(ch)));
 #else
REQUIRE(std::has_facet<std::ctype<wchar_t> >(___2311)); return static_cast<CHAR_TYPE>( std::use_facet<std::ctype<wchar_t> >(___2311).tolower(static_cast<wchar_t>(ch)));
 #endif
} }
 #endif
