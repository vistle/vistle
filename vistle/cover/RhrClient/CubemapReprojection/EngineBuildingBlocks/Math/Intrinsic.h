// EngineBuildingBlocks/Math/Intrinsic.h

#ifndef _ENGINEBUILDINGBLOCKS_INTRINSIC_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_INTRINSIC_H_INCLUDED_

#include <EngineBuildingBlocks/Settings.h>

#if(IS_USING_AVX2)

#include <intrin.h>
#include <cstring>

#include <External/vectorclass/vectorclass.h>

// Unary minus. Note that this operator requires 2 AVX operations.
inline __forceinline __m128 operator-(const __m128& x)
{
	return _mm_sub_ps(_mm_setzero_ps(), x);
}

inline __forceinline __m128 operator+(const __m128& x, const __m128& y)
{
	return _mm_add_ps(x, y);
}

inline __forceinline __m128 operator-(const __m128& x, const __m128& y)
{
	return _mm_sub_ps(x, y);
}

inline __forceinline __m128 operator*(const __m128& x, const __m128& y)
{
	return _mm_mul_ps(x, y);
}

inline __forceinline __m128 operator/(const __m128& x, const __m128& y)
{
	return _mm_div_ps(x, y);
}

inline __forceinline bool operator==(const __m128& x, const __m128& y)
{
	return (memcmp(&x, &y, sizeof(__m128)) == 0);
}

inline __forceinline bool operator!=(const __m128& x, const __m128& y)
{
	return (memcmp(&x, &y, sizeof(__m128)) != 0);
}

inline __forceinline __m128 Minimum(const __m128& x, const __m128& y)
{
	return _mm_min_ps(x, y);
}

inline __forceinline __m128 Maximum(const __m128& x, const __m128& y)
{
	return _mm_max_ps(x, y);
}

///////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// 256-BIT FLOAT //////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Unary minus. Note that this operator requires 2 AVX operations.
inline __forceinline __m256 operator-(const __m256& x)
{
	return _mm256_sub_ps(_mm256_setzero_ps(), x);
}

inline __forceinline __m256 operator+(const __m256& x, const __m256& y)
{
	return _mm256_add_ps(x, y);
}

inline __forceinline __m256& operator+=(__m256& x, const __m256& y)
{
	x = _mm256_add_ps(x, y);
	return x;
}

inline __forceinline __m256 operator-(const __m256& x, const __m256& y)
{
	return _mm256_sub_ps(x, y);
}

inline __forceinline __m256& operator-=(__m256& x, const __m256& y)
{
	x = _mm256_sub_ps(x, y);
	return x;
}

inline __forceinline __m256 operator*(const __m256& x, const __m256& y)
{
	return _mm256_mul_ps(x, y);
}

inline __forceinline __m256& operator*=(__m256& x, const __m256& y)
{
	x = _mm256_mul_ps(x, y);
	return x;
}

inline __forceinline __m256 operator/(const __m256& x, const __m256& y)
{
	return _mm256_div_ps(x, y);
}

inline __forceinline __m256& operator/=(__m256& x, const __m256& y)
{
	x = _mm256_div_ps(x, y);
	return x;
}

inline __forceinline bool operator==(const __m256& x, const __m256& y)
{
	return (memcmp(&x, &y, sizeof(__m256)) == 0);
}

inline __forceinline bool operator!=(const __m256& x, const __m256& y)
{
	return (memcmp(&x, &y, sizeof(__m256)) != 0);
}

inline __forceinline __m256 Minimum(const __m256& x, const __m256& y)
{
	return _mm256_min_ps(x, y);
}

inline __forceinline __m256 Maximum(const __m256& x, const __m256& y)
{
	return _mm256_max_ps(x, y);
}

inline __forceinline void ToDouble(const __m256& floats, __m256d& doubles1, __m256d& doubles2)
{
	doubles1 = _mm256_cvtps_pd(reinterpret_cast<const __m128&>(floats));
	doubles2 = _mm256_cvtps_pd(*reinterpret_cast<const __m128*>
		(reinterpret_cast<const unsigned char*>(&floats) + sizeof(__m128)));
}

inline float HorizontalSum(const __m256& floats)
{
	__m256 sum0 = _mm256_hadd_ps(floats, floats);
	__m256 sum1 = _mm256_hadd_ps(sum0, sum0);
	return sum1.m256_f32[0] + sum1.m256_f32[4];
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Unary minus. Note that this operator requires 2 AVX operations.
inline __forceinline __m256i operator-(const __m256i& x)
{
	return _mm256_sub_epi32(_mm256_setzero_si256(), x);
}

inline __forceinline __m256i operator+(const __m256i& x, const __m256i& y)
{
	return _mm256_add_epi32(x, y);
}

inline __forceinline __m256i operator-(const __m256i& x, const __m256i& y)
{
	return _mm256_sub_epi32(x, y);
}

inline __forceinline __m256i operator*(const __m256i& x, const __m256i& y)
{
	return _mm256_mullo_epi32(x, y);
}

inline __forceinline __m256i operator/(__m256i& a, int b)
{
	return reinterpret_cast<__m256i&>(reinterpret_cast<Vec8i&>(a) / Divisor_i(b));
}

inline __forceinline __m256i operator/(__m256i& a, Divisor_i& divisor)
{
	return reinterpret_cast<__m256i&>(reinterpret_cast<Vec8i&>(a) / divisor);
}

inline __forceinline bool operator==(const __m256i& x, const __m256i& y)
{
	return (memcmp(&x, &y, sizeof(__m256i)) == 0);
}

inline __forceinline bool operator!=(const __m256i& x, const __m256i& y)
{
	return (memcmp(&x, &y, sizeof(__m256i)) != 0);
}

inline __forceinline __m256i Minimum(const __m256i& x, const __m256i& y)
{
	return _mm256_min_epi32(x, y);
}

inline __forceinline __m256i Maximum(const __m256i& x, const __m256i& y)
{
	return _mm256_max_epi32(x, y);
}

inline __forceinline __m256 ToFloat(const __m256i& x)
{
	return _mm256_cvtepi32_ps(x);
}

inline __forceinline void ToFloat(const __m256i& integers, __m256& floats)
{
	floats = _mm256_cvtepi32_ps(integers);
}

inline __forceinline void ToDouble(const __m256i& integers, __m256d& doubles1, __m256d& doubles2)
{
	doubles1 = _mm256_cvtepi32_pd(reinterpret_cast<const __m128i&>(integers));
	doubles2 = _mm256_cvtepi32_pd(*reinterpret_cast<const __m128i*>
		(reinterpret_cast<const unsigned char*>(&integers) + sizeof(__m128i)));
}

// Unary minus. Note that this operator requires 2 AVX operations.
inline __forceinline __m256d operator-(const __m256d& x)
{
	return _mm256_sub_pd(_mm256_setzero_pd(), x);
}

inline __forceinline __m256d operator+(const __m256d& x, const __m256d& y)
{
	return _mm256_add_pd(x, y);
}

inline __forceinline __m256d operator-(const __m256d& x, const __m256d& y)
{
	return _mm256_sub_pd(x, y);
}

inline __forceinline __m256d operator*(const __m256d& x, const __m256d& y)
{
	return _mm256_mul_pd(x, y);
}

inline __forceinline __m256d operator/(const __m256d& x, const __m256d& y)
{
	return _mm256_div_pd(x, y);
}

inline __forceinline bool operator==(const __m256d& x, const __m256d& y)
{
	return (memcmp(&x, &y, sizeof(__m256d)) == 0);
}

inline __forceinline bool operator!=(const __m256d& x, const __m256d& y)
{
	return (memcmp(&x, &y, sizeof(__m256d)) != 0);
}

inline __forceinline __m256d Minimum(const __m256d& x, const __m256d& y)
{
	return _mm256_min_pd(x, y);
}

inline __forceinline __m256d Maximum(const __m256d& x, const __m256d& y)
{
	return _mm256_max_pd(x, y);
}

inline __forceinline void ToFloat(const __m256d& doubles1, const __m256d& doubles2, __m256& floats)
{
	reinterpret_cast<__m128&>(floats) = _mm256_cvtpd_ps(doubles1);
	*reinterpret_cast<__m128*>(reinterpret_cast<unsigned char*>(&floats) + sizeof(__m128))
		= _mm256_cvtpd_ps(doubles2);
}

inline __forceinline void ToInt(const __m256d& doubles1, const __m256d& doubles2, __m256i& integers)
{
	reinterpret_cast<__m128i&>(integers) = _mm256_cvtpd_epi32(doubles1);
	*reinterpret_cast<__m128i*>(reinterpret_cast<unsigned char*>(&integers) + sizeof(__m128i))
		= _mm256_cvtpd_epi32(doubles2);
}

#endif // if(IS_USING_AVX2)

#endif