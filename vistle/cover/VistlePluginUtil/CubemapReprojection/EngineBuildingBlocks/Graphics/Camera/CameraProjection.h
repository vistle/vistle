// EngineBuildingBlocks/Graphics/Camera/CameraProjection.h

#ifndef _ENGINEBUILDINGBLOCKS_CAMERAPROJECTION_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_CAMERAPROJECTION_H_INCLUDED_

#include <Core/SimpleBinarySerialization.hpp>
#include <Core/SimpleXMLSerialization.hpp>

#include <EngineBuildingBlocks/Math/GLM.h>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		enum class ProjectionType : unsigned char
		{
			Perspective, Orthographic
		};

		struct PerspectiveOrthographicProjection
		{
			float NearPlaneDistance;
			float FarPlaneDistance;
			bool IsProjectingTo_0_1_Interval;

			float Left;
			float Right;
			float Bottom;
			float Top;

			void Set(float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval,
				float fovY, float aspectRatio);
			void Set(float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval,
				float left, float right, float bottom, float top);

			float GetAspectRatio() const;

			float GetNearPlaneWidth() const;
			float GetNearPlaneHeight() const;

			float GetFarPlaneWidth() const;
			float GetFarPlaneHeight() const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);

			void SerializeSXML(Core::SerializationSXMLData& data) const;
			void DeserializeSXML(Core::DeserializationSXMLData& data);
		};

		struct CameraProjection
		{
			union Projection
			{
				struct Perspective : public PerspectiveOrthographicProjection
				{
					float GetFovX() const;
					float GetFovY() const;

					// Returns the linear depth (z in view space) for a perspective depth (d = z_p / w_p).
					// The input value must be in the interval: d E [0, 1] if IsProjectingTo_0_1_Interval == true
					//                                          d E [-1, 1] otherwise.
					// The return value is a negative number.
					float GetLinearDepth(float perspectiveDepth) const;

					// Returns the perspective depth (d = z_p / w_p) for a linear depth (z in view space).
					// The input must be a negative value.
					// The return value is in the interval: d E [0, 1] if IsProjectingTo_0_1_Interval == true
					//                                      d E [-1, 1] otherwise.
					float GetPerspectiveDepth(float linearDepth) const;
				} Perspective;

				struct Orthographic : public PerspectiveOrthographicProjection
				{
				} Orthographic;
			} Projection;

			ProjectionType Type;

			float GetAspectRatio() const;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);

			void SerializeSXML(Core::SerializationSXMLData& data) const;
			void DeserializeSXML(Core::DeserializationSXMLData& data);
		
			bool CreateFromMatrix(const glm::mat4& m);
			void CreatePerspecive(float aspectRatio, float fovY, float near, float far,
				bool isProjectingTo_0_1);
		};
	}
}

#endif