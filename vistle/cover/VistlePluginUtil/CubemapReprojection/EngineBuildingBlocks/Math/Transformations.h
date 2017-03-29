// EngineBuildingBlocks/Transformations.h

#ifndef _ENGINEBUILDINGBLOCKS_TRANSFORMATIONS_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_TRANSFORMATIONS_H_INCLUDED_

#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

#include <cassert>

namespace EngineBuildingBlocks
{
	struct UninitializedType {};
	const UninitializedType Uninitialized;

	const glm::dualquat c_ZeroDualQuaternion
		= glm::dualquat(glm::quat(0.0f, 0.0f, 0.0f, 0.0f), glm::quat(0.0f, 0.0f, 0.0f, 0.0f));

	inline glm::dualquat ToDualQuaternion(const glm::mat4x3& m)
	{
		return glm::dualquat_cast(glm::transpose(m));
	}

	inline glm::dualquat ToDualQuaternion(const glm::mat4& m)
	{
		return ToDualQuaternion(glm::mat4x3(m));
	}

	inline glm::dualquat AddSignAware(const glm::dualquat& dq0, const glm::dualquat& dq1)
	{
		float d = glm::dot(dq0.real, dq1.real);
		if (d < 0.0f) return dq0 - dq1;
		return dq0 + dq1;
	}

	inline glm::mat4 ToMatrix4x3(const glm::dualquat& dQuat)
	{
		return glm::transpose(glm::mat3x4_cast(dQuat));
	}

	inline glm::mat4 ToMatrix4(const glm::dualquat& dQuat)
	{
		return glm::mat4(ToMatrix4x3(dQuat));
	}

	inline bool HasLength(const glm::vec3& v, float length, const float errorEpsilon)
	{
		return (glm::abs(glm::length(v) - length) <= errorEpsilon);
	}

	inline bool IsOrthonorm(const glm::mat3& m, const float errorEpsilon)
	{
		return HasLength(m[0], 1.0f, errorEpsilon)
			&& HasLength(m[1], 1.0f, errorEpsilon)
			&& HasLength(m[2], 1.0f, errorEpsilon)
			&& HasLength(glm::cross(m[0], m[1]), 1.0f, errorEpsilon)
			&& HasLength(glm::cross(m[0], m[2]), 1.0f, errorEpsilon)
			&& HasLength(glm::cross(m[1], m[2]), 1.0f, errorEpsilon);
	}

	// This structure represents a rigid transformation, i.e. a transformation consisting only rotatations and translations,
	// which might written as a rotation (3x3 matrix part) and a translation.
	struct RigidTransformation
	{
		glm::mat3 Orientation;
		glm::vec3 Position;

		RigidTransformation() {}
		RigidTransformation(UninitializedType) : Orientation(glm::uninitialize), Position(glm::uninitialize) {}
		RigidTransformation(const glm::mat3& orientation, const glm::vec3& position) : Orientation(orientation), Position(position) {}
		RigidTransformation(const glm::mat4x3& m) { *this = m; }
		RigidTransformation(const glm::mat4& m) { *this = m; }
		RigidTransformation(const glm::dualquat& dQuat) { *this = dQuat; }

		inline const glm::mat4x3& AsMatrix4x3() const
		{
			return reinterpret_cast<const glm::mat4x3&>(*this);
		}

		inline explicit operator const glm::mat4x3&() const
		{
			return AsMatrix4x3();
		}

		inline RigidTransformation& operator=(const glm::mat4x3& m)
		{
			reinterpret_cast<glm::mat4x3&>(*this) = m;
			return *this;
		}

		inline glm::mat4 AsMatrix4() const
		{
			glm::mat4 m(glm::uninitialize);
			m[0][3] = m[1][3] = m[2][3] = 0.0f; m[3][3] = 1.0f;
			reinterpret_cast<glm::vec3&>(m[0]) = Orientation[0];
			reinterpret_cast<glm::vec3&>(m[1]) = Orientation[1];
			reinterpret_cast<glm::vec3&>(m[2]) = Orientation[2];
			reinterpret_cast<glm::vec3&>(m[3]) = Position;
			return m;
		}

		inline explicit operator const glm::mat4() const
		{
			return AsMatrix4();
		}

		inline RigidTransformation& operator=(const glm::mat4& m)
		{
			Orientation = m;
			Position = m[3];
			return *this;
		}

		inline glm::dualquat ToDualQuaternion() const
		{
			return EngineBuildingBlocks::ToDualQuaternion(AsMatrix4x3());
		}

		inline explicit operator const glm::dualquat() const
		{
			return ToDualQuaternion();
		}

		inline RigidTransformation& operator=(const glm::dualquat& dQuat)
		{
			*this = EngineBuildingBlocks::ToMatrix4x3(dQuat);
			return *this;
		}

		inline RigidTransformation operator*(const glm::mat4& right) const
		{
			return{ AsMatrix4x3() * right };
		}

		// Interpolates two rigid transformation by linear matrix interpolation.
		// Note that the resulting transformation is in general not rigid, sometimes
		// even becomes a projection (e.g. candy wrapper effect).
		inline static RigidTransformation MatrixInterpolate(
			const RigidTransformation& r0, const RigidTransformation& r1, float weight)
		{
			return glm::mix(r0.AsMatrix4x3(), r1.AsMatrix4x3(), weight);
		}

		inline static RigidTransformation DualQuaternionInterpolate(
			const RigidTransformation& r0, const RigidTransformation& r1, float weight)
		{
			return glm::lerp(r0.ToDualQuaternion(), r1.ToDualQuaternion(), weight);
		}

		void SerializeSB(Core::ByteVector& bytes) const
		{
			Core::SerializeSB(bytes, Core::ToPlaceHolder(*this));
		}

		void DeserializeSB(const unsigned char*& bytes)
		{
			Core::DeserializeSB(bytes, Core::ToPlaceHolder(*this));
		}
	};

	// This structure represents a represents a transformation which may include multiple scalings, rotations and translations,
	// which might be written as a rotation, scaling and a rotation (SVD decomposition of the 3x3 matrix part) and a translation.
	struct ScaledTransformation
	{
		glm::mat3 A;
		glm::vec3 Position;

		ScaledTransformation() {}
		ScaledTransformation(UninitializedType) : A(glm::uninitialize), Position(glm::uninitialize) {}
		ScaledTransformation(const glm::mat3& a, const glm::vec3& position) : A(a), Position(position) {}
		ScaledTransformation(const glm::mat4x3& m) { *this = m; }
		ScaledTransformation(const glm::mat4& m) { *this = m; }
		ScaledTransformation(const RigidTransformation& t) : A(t.Orientation), Position(t.Position) {}

		inline const glm::mat4x3& AsMatrix4x3() const
		{
			return reinterpret_cast<const glm::mat4x3&>(*this);
		}

		inline explicit operator const glm::mat4x3&() const
		{
			return AsMatrix4x3();
		}

		inline ScaledTransformation& operator=(const glm::mat4x3& m)
		{
			reinterpret_cast<glm::mat4x3&>(*this) = m;
			return *this;
		}

		inline glm::mat4 AsMatrix4() const
		{
			glm::mat4 m(glm::uninitialize);
			m[0][3] = m[1][3] = m[2][3] = 0.0f; m[3][3] = 1.0f;
			reinterpret_cast<glm::vec3&>(m[0]) = A[0];
			reinterpret_cast<glm::vec3&>(m[1]) = A[1];
			reinterpret_cast<glm::vec3&>(m[2]) = A[2];
			reinterpret_cast<glm::vec3&>(m[3]) = Position;
			return m;
		}

		inline explicit operator const glm::mat4() const
		{
			return AsMatrix4();
		}

		inline ScaledTransformation& operator=(const glm::mat4& m)
		{
			A = m;
			Position = m[3];
			return *this;
		}

		inline ScaledTransformation operator*(const glm::mat4& right) const
		{
			return{ AsMatrix4x3() * right };
		}

		void SerializeSB(Core::ByteVector& bytes) const
		{
			Core::SerializeSB(bytes, Core::ToPlaceHolder(*this));
		}

		void DeserializeSB(const unsigned char*& bytes)
		{
			Core::DeserializeSB(bytes, Core::ToPlaceHolder(*this));
		}
	};

	struct QuaternionLocalTransformation
	{
		glm::quat Rotation;
		glm::vec4 LocalTr;	// 3 floats would be enough, but having a size divisible with 16 bytes makes
							// copying arrays to the GPU easier.

		QuaternionLocalTransformation() {}
		QuaternionLocalTransformation(const glm::quat& rotation, const glm::vec3& localTr)
			: Rotation(rotation), LocalTr(localTr, 0.0f) {}
		QuaternionLocalTransformation(const glm::mat4x3& m)
			: Rotation(glm::uninitialize), LocalTr(glm::uninitialize) { *this = m; }
		QuaternionLocalTransformation(const glm::mat4& m)
			: Rotation(glm::uninitialize), LocalTr(glm::uninitialize) { *this = m; }
		QuaternionLocalTransformation(const glm::mat3& rotation, const glm::vec3& translation)
			: Rotation(glm::uninitialize), LocalTr(glm::uninitialize) { _Construct(rotation, translation); }

		inline void _Construct(const glm::mat3& rotation, const glm::vec3& translation)
		{
			assert(IsOrthonorm(rotation, 1e-4f));
			Rotation = glm::quat_cast(rotation);
			LocalTr = glm::vec4(glm::transpose(rotation) * translation, 0.0f);
		}

		inline QuaternionLocalTransformation& operator=(const glm::mat4x3& m)
		{
			_Construct(reinterpret_cast<const glm::mat3&>(m), m[3]);
			return *this;
		}

		inline QuaternionLocalTransformation& operator=(const glm::mat4& m)
		{
			_Construct(glm::mat3(m), reinterpret_cast<const glm::vec3&>(m[3]));
			return *this;
		}
	};
}

#endif