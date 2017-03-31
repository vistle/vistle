// EngineBuildingBlocks/Math/Vector256.h

#ifndef _ENGINEBUILDINGBLOCKS_VECTOR256_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_VECTOR256_H_INCLUDED_

#include <Settings.h>

#if(IS_USING_AVX2)

#include <EngineBuildingBlocks/Math/Intrinsic.h>

namespace EngineBuildingBlocks
{
	namespace Math
	{
		/////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////// 256-BIT VECTOR2 //////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		struct Vector2_256
		{
			union
			{
				struct
				{
					__m256 X, Y;
				};
				__m256 Vector[2];
			};

			// Default constructor. It doesn't do any initialization.
			Vector2_256() {}

			Vector2_256(const Vector2_256& other)
				: X(other.X)
				, Y(other.Y)
			{}

			Vector2_256(const __m256& value)
				: X(value)
				, Y(value)
			{}

			Vector2_256(const __m256& x, const __m256& y)
				: X(x)
				, Y(y)
			{}

			inline __forceinline Vector2_256 operator +(const Vector2_256& other) const
			{
				return Vector2_256(X + other.X, Y + other.Y);
			}

			inline __forceinline Vector2_256& operator +=(const Vector2_256& other)
			{
				X += other.X;
				Y += other.Y;
				return *this;
			}

			// Unary minus.
			inline __forceinline Vector2_256 operator -() const
			{
				return Vector2_256(-X, -Y);
			}

			inline __forceinline Vector2_256 operator -(const Vector2_256& other) const
			{
				return Vector2_256(X - other.X, Y - other.Y);
			}

			inline __forceinline Vector2_256& operator -=(const Vector2_256& other)
			{
				X -= other.X;
				Y -= other.Y;
				return *this;
			}

			inline __forceinline Vector2_256 operator *(const __m256& m) const
			{
				return Vector2_256(X * m, Y * m);
			}

			inline __forceinline Vector2_256 operator *(const Vector2_256& other) const
			{
				return Vector2_256(X * other.X, Y * other.Y);
			}

			inline __forceinline Vector2_256& operator *=(const __m256& m)
			{
				X *= m;
				Y *= m;
				return *this;
			}

			inline __forceinline Vector2_256& operator *=(const Vector2_256& other)
			{
				X *= other.X;
				Y *= other.Y;
				return *this;
			}

			inline __forceinline Vector2_256 operator /(const __m256& m) const
			{
				return Vector2_256(X / m, Y / m);
			}

			inline __forceinline Vector2_256 operator /(const Vector2_256& other) const
			{
				return Vector2_256(X / other.X, Y / other.Y);
			}

			inline __forceinline Vector2_256& operator /=(const __m256& m)
			{
				X /= m;
				Y /= m;
				return *this;
			}

			inline __forceinline Vector2_256& operator /=(const Vector2_256& other)
			{
				X /= other.X;
				Y /= other.Y;
				return *this;
			}

			void ReadScalar(unsigned index, float* values)
			{
				values[0] = X.m256_f32[index];
				values[1] = Y.m256_f32[index];
			}

			void WriteScalar(unsigned index, const float* values)
			{
				X.m256_f32[index] = values[0];
				Y.m256_f32[index] = values[1];
			}
		};

		static inline __forceinline __m256 Dot(const Vector2_256& a, const Vector2_256& b)
		{
			_mm256_fmadd_ps(a.X, b.X, a.Y * b.Y);
		}

		/////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////// 256-BIT VECTOR3 //////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		struct Vector3_256
		{
			union
			{
				struct
				{
					__m256 X, Y, Z;
				};
				__m256 Vector[3];
			};

			// Default constructor. It doesn't do any initialization.
			Vector3_256() {}

			Vector3_256(const Vector3_256& other)
				: X(other.X)
				, Y(other.Y)
				, Z(other.Z)
			{}

			Vector3_256(const __m256& value)
				: X(value)
				, Y(value)
				, Z(value)
			{}

			Vector3_256(const __m256& x, const __m256& y, const __m256& z)
				: X(x)
				, Y(y)
				, Z(z)
			{}

			Vector3_256(const Vector2_256& xy, const __m256& z)
				: X(xy.X)
				, Y(xy.Y)
				, Z(z)
			{}

			inline __forceinline Vector3_256 operator +(const Vector3_256& other) const
			{
				return Vector3_256(X + other.X, Y + other.Y, Z + other.Z);
			}

			inline __forceinline Vector3_256& operator +=(const Vector3_256& other)
			{
				X += other.X;
				Y += other.Y;
				Z += other.Z;
				return *this;
			}

			// Unary minus.
			inline __forceinline Vector3_256 operator -() const
			{
				return Vector3_256(-X, -Y, -Z);
			}

			inline __forceinline Vector3_256 operator -(const Vector3_256& other) const
			{
				return Vector3_256(X - other.X, Y - other.Y, Z - other.Z);
			}

			inline __forceinline Vector3_256& operator -=(const Vector3_256& other)
			{
				X -= other.X;
				Y -= other.Y;
				Z -= other.Z;
				return *this;
			}

			inline __forceinline Vector3_256 operator *(const __m256& m) const
			{
				return Vector3_256(X * m, Y * m, Z * m);
			}

			inline __forceinline Vector3_256 operator *(const Vector3_256& other) const
			{
				return Vector3_256(X * other.X, Y * other.Y, Z * other.Z);
			}

			inline __forceinline Vector3_256& operator *=(const __m256& m)
			{
				X *= m;
				Y *= m;
				Z *= m;
				return *this;
			}

			inline __forceinline Vector3_256& operator *=(const Vector3_256& other)
			{
				X *= other.X;
				Y *= other.Y;
				Z *= other.Z;
				return *this;
			}

			inline __forceinline Vector3_256 operator /(const __m256& m) const
			{
				return Vector3_256(X / m, Y / m, Z / m);
			}

			inline __forceinline Vector3_256 operator /(const Vector3_256& other) const
			{
				return Vector3_256(X / other.X, Y / other.Y, Z / other.Z);
			}

			inline __forceinline Vector3_256& operator /=(const __m256& m)
			{
				X /= m;
				Y /= m;
				Z /= m;
				return *this;
			}

			inline __forceinline Vector3_256& operator /=(const Vector3_256& other)
			{
				X /= other.X;
				Y /= other.Y;
				Z /= other.Z;
				return *this;
			}

			void ReadScalar(unsigned index, float* values)
			{
				values[0] = X.m256_f32[index];
				values[1] = Y.m256_f32[index];
				values[2] = Z.m256_f32[index];
			}

			void WriteScalar(unsigned index, const float* values)
			{
				X.m256_f32[index] = values[0];
				Y.m256_f32[index] = values[1];
				Z.m256_f32[index] = values[2];
			}
		};

		static inline __forceinline __m256 Dot(const Vector3_256& a, const Vector3_256& b)
		{
			_mm256_fmadd_ps(a.X, b.X, _mm256_fmadd_ps(a.Y, b.Y, a.Z * b.Z));
		}

		static inline __forceinline Vector3_256 Cross(const Vector3_256& a, const Vector3_256& b)
		{
			return Vector3_256(
				_mm256_fmsub_ps(a.Y, b.Z, b.Y * a.Z),
				_mm256_fmsub_ps(a.Z, b.X, b.Z * a.X),
				_mm256_fmsub_ps(a.X, b.Y, b.X * a.Y));
		}

		/////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////// 256-BIT VECTOR4 //////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		struct Vector4_256
		{
			union
			{
				struct
				{
					__m256 X, Y, Z, W;
				};
				__m256 Vector[4];
			};

			// Default constructor. It doesn't do any initialization.
			Vector4_256() {}

			Vector4_256(const Vector4_256& other)
				: X(other.X)
				, Y(other.Y)
				, Z(other.Z)
				, W(other.W)
			{}

			Vector4_256(const __m256& value)
				: X(value)
				, Y(value)
				, Z(value)
				, W(value)
			{}

			Vector4_256(const __m256& x, const __m256& y, const __m256& z, const __m256& w)
				: X(x)
				, Y(y)
				, Z(z)
				, W(w)
			{}

			Vector4_256(const Vector2_256& xy, const __m256& z, const __m256& w)
				: X(xy.X)
				, Y(xy.Y)
				, Z(z)
				, W(w)
			{}

			Vector4_256(const Vector3_256& xyz, const __m256& w)
				: X(xyz.X)
				, Y(xyz.Y)
				, Z(xyz.Z)
				, W(w)
			{}

			inline __forceinline Vector4_256 operator +(const Vector4_256& other) const
			{
				return Vector4_256(X + other.X, Y + other.Y, Z + other.Z, W + other.W);
			}

			inline __forceinline Vector4_256& operator +=(const Vector4_256& other)
			{
				X += other.X;
				Y += other.Y;
				Z += other.Z;
				W += other.W;
				return *this;
			}

			// Unary minus.
			inline __forceinline Vector4_256 operator -() const
			{
				return Vector4_256(-X, -Y, -Z, -W);
			}

			inline __forceinline Vector4_256 operator -(const Vector4_256& other) const
			{
				return Vector4_256(X - other.X, Y - other.Y, Z - other.Z, W - other.W);
			}

			inline __forceinline Vector4_256& operator -=(const Vector4_256& other)
			{
				X -= other.X;
				Y -= other.Y;
				Z -= other.Z;
				W -= other.W;
				return *this;
			}

			inline __forceinline Vector4_256 operator *(const __m256& m) const
			{
				return Vector4_256(X * m, Y * m, Z * m, W * m);
			}

			inline __forceinline Vector4_256 operator *(const Vector4_256& other) const
			{
				return Vector4_256(X * other.X, Y * other.Y, Z * other.Z, W * other.W);
			}

			inline __forceinline Vector4_256& operator *=(const __m256& m)
			{
				X *= m;
				Y *= m;
				Z *= m;
				W *= m;
				return *this;
			}

			inline __forceinline Vector4_256& operator *=(const Vector4_256& other)
			{
				X *= other.X;
				Y *= other.Y;
				Z *= other.Z;
				W *= other.W;
				return *this;
			}

			inline __forceinline Vector4_256 operator /(const __m256& m) const
			{
				return Vector4_256(X / m, Y / m, Z / m, W / m);
			}

			inline __forceinline Vector4_256 operator /(const Vector4_256& other) const
			{
				return Vector4_256(X / other.X, Y / other.Y, Z / other.Z, W / other.W);
			}

			inline __forceinline Vector4_256& operator /=(const __m256& m)
			{
				X /= m;
				Y /= m;
				Z /= m;
				W /= m;
				return *this;
			}

			inline __forceinline Vector4_256& operator /=(const Vector4_256& other)
			{
				X /= other.X;
				Y /= other.Y;
				Z /= other.Z;
				W /= other.W;
				return *this;
			}

			void ReadScalar(unsigned index, float* values)
			{
				values[0] = X.m256_f32[index];
				values[1] = Y.m256_f32[index];
				values[2] = Z.m256_f32[index];
				values[3] = W.m256_f32[index];
			}

			void WriteScalar(unsigned index, const float* values)
			{
				X.m256_f32[index] = values[0];
				Y.m256_f32[index] = values[1];
				Z.m256_f32[index] = values[2];
				W.m256_f32[index] = values[3];
			}
		};

		static inline __forceinline __m256 Dot(const Vector4_256& a, const Vector4_256& b)
		{
			_mm256_fmadd_ps(a.X, b.X, _mm256_fmadd_ps(a.Y, b.Y,
				_mm256_fmadd_ps(a.Z, b.Z, a.W * b.W)));
		}

		/////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// 256-BIT MATRIX2X2 /////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		// MUADDIB_TODO: implement 256-bit matrix2x2.

		/////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// 256-BIT MATRIX3X3 /////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		struct Matrix3x3_256
		{
			union
			{
				struct
				{
					__m256 M00, M01, M02, M10, M11, M12, M20, M21, M22;
				};
				Vector3_256 Columns[3];
			};

			Matrix3x3_256() {}

			// Elementwise-contructor. m{i}{j} is the element in the i. column and j. row.
			Matrix3x3_256(
				const __m256& m00, const __m256& m01, const __m256& m02,
				const __m256& m10, const __m256& m11, const __m256& m12,
				const __m256& m20, const __m256& m21, const __m256& m22)
				: M00(m00)
				, M01(m01)
				, M02(m02)
				, M10(m10)
				, M11(m11)
				, M12(m12)
				, M20(m20)
				, M21(m21)
				, M22(m22)
			{
			}

			Matrix3x3_256(const Vector3_256& column0, const Vector3_256& column1,
				const Vector3_256& column2)
			{
				Columns[0] = column0;
				Columns[1] = column1;
				Columns[2] = column2;
			}

			Matrix3x3_256 operator+(const Matrix3x3_256& other) const
			{
				return
				{
					Columns[0] + other.Columns[0],
					Columns[1] + other.Columns[1],
					Columns[2] + other.Columns[2]
				};
			}

			Matrix3x3_256& operator+=(const Matrix3x3_256& other)
			{
				Columns[0] += other.Columns[0];
				Columns[1] += other.Columns[1];
				Columns[2] += other.Columns[2];
				return *this;
			}

			// Unary minus.
			inline __forceinline Matrix3x3_256 operator -() const
			{
				return
				{
					-Columns[0],
					-Columns[1],
					-Columns[2]
				};
			}

			Matrix3x3_256 operator-(const Matrix3x3_256& other) const
			{
				return
				{
					Columns[0] - other.Columns[0],
					Columns[1] - other.Columns[1],
					Columns[2] - other.Columns[2]
				};
			}

			Matrix3x3_256& operator-=(const Matrix3x3_256& other)
			{
				Columns[0] -= other.Columns[0];
				Columns[1] -= other.Columns[1];
				Columns[2] -= other.Columns[2];
				return *this;
			}

			Vector3_256 operator*(const Vector3_256& v) const
			{
				return
				{
					_mm256_fmadd_ps(M00, v.X, _mm256_fmadd_ps(M10, v.Y, M20 * v.Z)),
					_mm256_fmadd_ps(M01, v.X, _mm256_fmadd_ps(M11, v.Y, M21 * v.Z)),
					_mm256_fmadd_ps(M02, v.X, _mm256_fmadd_ps(M12, v.Y, M22 * v.Z))
				};
			}

			Matrix3x3_256 operator*(const Matrix3x3_256& other) const
			{
				return
				{
					_mm256_fmadd_ps(M00, other.M00, _mm256_fmadd_ps(M10, other.M01, M20 * other.M02)),
					_mm256_fmadd_ps(M01, other.M00, _mm256_fmadd_ps(M11, other.M01, M21 * other.M02)),
					_mm256_fmadd_ps(M02, other.M00, _mm256_fmadd_ps(M12, other.M01, M22 * other.M02)),
					_mm256_fmadd_ps(M00, other.M10, _mm256_fmadd_ps(M10, other.M11, M20 * other.M12)),
					_mm256_fmadd_ps(M01, other.M10, _mm256_fmadd_ps(M11, other.M11, M21 * other.M12)),
					_mm256_fmadd_ps(M02, other.M10, _mm256_fmadd_ps(M12, other.M11, M22 * other.M12)),
					_mm256_fmadd_ps(M00, other.M20, _mm256_fmadd_ps(M10, other.M21, M20 * other.M22)),
					_mm256_fmadd_ps(M01, other.M20, _mm256_fmadd_ps(M11, other.M21, M21 * other.M22)),
					_mm256_fmadd_ps(M02, other.M20, _mm256_fmadd_ps(M12, other.M21, M22 * other.M22))
				};
			}

			Matrix3x3_256& operator*=(const Matrix3x3_256& other)
			{
				*this = *this * other;
				return *this;
			}

			void ReadScalar(unsigned index, float* values)
			{
				values[0] = M00.m256_f32[index];
				values[1] = M01.m256_f32[index];
				values[2] = M02.m256_f32[index];
				values[3] = M10.m256_f32[index];
				values[4] = M11.m256_f32[index];
				values[5] = M12.m256_f32[index];
				values[6] = M20.m256_f32[index];
				values[7] = M21.m256_f32[index];
				values[8] = M22.m256_f32[index];
			}

			void WriteScalar(unsigned index, const float* values)
			{
				M00.m256_f32[index] = values[0];
				M01.m256_f32[index] = values[1];
				M02.m256_f32[index] = values[2];
				M10.m256_f32[index] = values[3];
				M11.m256_f32[index] = values[4];
				M12.m256_f32[index] = values[5];
				M20.m256_f32[index] = values[6];
				M21.m256_f32[index] = values[7];
				M22.m256_f32[index] = values[8];
			}

			void WriteScalarTransposed(unsigned index, const float* values)
			{
				M00.m256_f32[index] = values[0];
				M01.m256_f32[index] = values[3];
				M02.m256_f32[index] = values[6];
				M10.m256_f32[index] = values[1];
				M11.m256_f32[index] = values[4];
				M12.m256_f32[index] = values[7];
				M20.m256_f32[index] = values[2];
				M21.m256_f32[index] = values[5];
				M22.m256_f32[index] = values[8];
			}
		};

		/////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// 256-BIT MATRIX4X4 /////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

		// MUADDIB_TODO: implement 256-bit matrix4x4.
	}
}

#endif // #if(IS_USING_AVX2)
#endif // #ifndef _ENGINEBUILDINGBLOCKS_VECTOR256_H_INCLUDED_