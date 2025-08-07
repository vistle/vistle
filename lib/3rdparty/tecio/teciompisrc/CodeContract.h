 #pragma once
 #ifndef CODE_CONTRACT_H
 #define CODE_CONTRACT_H
#include "ThirdPartyHeadersBegin.h"
 #if !defined NO_ASSERTS && defined CHECKED_BUILD
#include <stdio.h>
#include <stdlib.h>
 #endif
#include <cstdlib>
#include <iostream>
#include <limits>
#include "ThirdPartyHeadersEnd.h"
 #define VALID_REF(p)      ((p)  != 0)
 #define VALID_FN_REF(___3000)  ((___3000) != 0)
 #if defined NO_ASSERTS
 #define ASSERT(___1243)
 #define ASSERT_ONLY(___1243)
 #else   
 #define ASSERT_ONLY(___1243) ___1243
 #if defined CHECKED_BUILD
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4505)
 #endif
#include <assert.h>
static bool tpReportAssertion( char const* ___2430, char const* srcFileName, int         srcLineNum) { char const* const ___2316 = ::getenv("TP_ASSERT_LOG_FILENAME"); if (___2316 != 0) { FILE* logFile = ::fopen(___2316, "a"); if (logFile != 0) { ::fprintf(logFile, "Assertion failure:\n" "    File:   %s\n" "    Line:   %d\n" "    Reason: %s\n", srcFileName, srcLineNum, ___2430); ::fclose(logFile); } else { assert(!"Failed to log checked build assertion. Check permissions"); } return true; } else { return false; } }
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #endif
 #define CHECKED_ASSERT(___1243) ((___1243) ? (void)(0) : (tpReportAssertion(#___1243,__FILE__,__LINE__) ? (void)(0) : assert(___1243)))
 #define ASSERT(___1243) CHECKED_ASSERT(___1243)
 #elif !defined NDEBUG 
#include <assert.h>
 #define ASSERT(___1243) assert(___1243)
 #endif
 #endif
 #define INVARIANT(___1243) ASSERT(___1243)
 #define REQUIRE(___1243)   ASSERT(___1243)
 #define ENSURE(___1243)    ASSERT(___1243)
 #define ___476(___1243)     ASSERT(___1243)
 #if defined NO_ASSERTS
 #define VERIFY(___1243) ((void)(___1243))
 #elif defined CHECKED_BUILD
 #define VERIFY(___1243) CHECKED_ASSERT(___1243)
 #elif !defined NDEBUG 
 #define VERIFY(___1243) assert(___1243)
 #endif
 #define IMPLICATION(___2892,___3256) (!(___2892) || (___3256))
 #define EQUIVALENCE(___2892,___3256) ((!!(___2892)) == (!!(___3256)))
 #define VALID_MAP_KEY(key,map) (map.find(key) != map.end())
 #define VALID_REF_OR_NULL(p)       (VALID_REF(p) || p == 0)
 #define VALID_BOOLEAN(b)           ((b) == 1 || (b) == 0)
 #define VALID_ENUM(value, type)    (0 <= (value) && (value) < END_##type)
 #define VALID_NON_ZERO_LEN_STR(str) (VALID_REF(str) && str[0] != '\0')
 #define VALID_CLASS_ENUM(e) (static_cast<std::decay<decltype((e))>::type>(0) <= (e) && (e) < std::decay<decltype((e))>::type::END_ENUM)
 #ifdef __cplusplus
 #if __cplusplus > 199711L 
namespace tecplot { template<typename Dst, typename Src> inline Dst numeric_cast(Src value) {
 #if defined(_MSC_VER)
 #pragma warning (push, 0)
 #pragma warning (disable:4018)
 #elif defined LINUX && defined __GNUC__ && __cplusplus >= 201103L
 #define ___1543 (__GNUC__ * 10000 \
 + __GNUC_MINOR__ * 100 \
 + __GNUC_PATCHLEVEL__)
 #if ___1543 >= 40600
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wsign-compare"
 #pragma GCC diagnostic ignored "-Wbool-compare"
 #endif
 #endif
const bool positive_overflow_possible = std::numeric_limits<Dst>::max() < std::numeric_limits<Src>::max(); const bool negative_overflow_possible = std::numeric_limits<Src>::is_signed || (std::numeric_limits<Dst>::lowest() > std::numeric_limits<Src>::lowest()); if ((!std::numeric_limits<Dst>::is_signed) && (!std::numeric_limits<Src>::is_signed)) { if (positive_overflow_possible && (value > std::numeric_limits<Dst>::max())) { REQUIRE(!"positive overflow"); } } else if ((!std::numeric_limits<Dst>::is_signed) && std::numeric_limits<Src>::is_signed) { if (positive_overflow_possible && (value > std::numeric_limits<Dst>::max())) { REQUIRE(!"positive overflow"); } else if (negative_overflow_possible && (value < 0)) { REQUIRE(!"negative overflow"); } } else if (std::numeric_limits<Dst>::is_signed && (!std::numeric_limits<Src>::is_signed)) { if (positive_overflow_possible && (value > std::numeric_limits<Dst>::max())) { REQUIRE(!"positive overflow"); } } else if (std::numeric_limits<Dst>::is_signed && std::numeric_limits<Src>::is_signed) { if (positive_overflow_possible && (value > std::numeric_limits<Dst>::max())) { REQUIRE(!"positive overflow"); } else if (negative_overflow_possible && (value < std::numeric_limits<Dst>::lowest())) { REQUIRE(!"negative overflow"); } }
 #if defined(_MSC_VER)
 #pragma warning (pop)
 #elif defined LINUX && defined __GNUC__ && __cplusplus >= 201103L && ___1543 >= 40600
 #pragma GCC diagnostic pop
 #endif
return static_cast<Dst>(value); } }
 #endif
template <typename TargetType, typename SourceType> inline TargetType checked_numeric_cast(SourceType const& value) {
 #if __cplusplus > 199711L 
ASSERT_ONLY( tecplot::numeric_cast<TargetType>(value); );
 #endif
return static_cast<TargetType>(value); } namespace { template<typename TargetType, typename SourceType> struct alias_cast_t { union { SourceType source; TargetType ___3356; }; }; } template<typename TargetType, typename SourceType> TargetType alias_cast(SourceType value) {
 #if __cplusplus > 199711L 
static_assert(sizeof(TargetType) == sizeof(SourceType), "Cannot cast types of different sizes"); alias_cast_t<TargetType, SourceType> ac {{value}};
 #else
REQUIRE(sizeof(TargetType) == sizeof(SourceType)); alias_cast_t<TargetType, SourceType> ac; ac.source = value;
 #endif
return ac.___3356; }
 #endif
 #endif 
