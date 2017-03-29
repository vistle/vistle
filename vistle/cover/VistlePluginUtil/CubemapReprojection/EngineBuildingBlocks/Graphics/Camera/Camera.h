// EngineBuildingBlocks/Graphics/Camera/Camera.h

#ifndef _ENGINEBUILDINGBLOCKS_CAMERA_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_CAMERA_H_INCLUDED_

#include <Core/SimpleBinarySerialization.hpp>

#include <EngineBuildingBlocks/SceneNode.h>
#include <EngineBuildingBlocks/Math/BoundingFrustum.h>
#include <EngineBuildingBlocks/Graphics/Camera/CameraProjection.h>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		struct CameraData
		{
			CameraProjection Projection;
			EngineBuildingBlocks::RigidTransformation LocalTransformation;
		};

		class Camera
		{
			WrappedSceneNode m_SceneNode;

			CameraProjection m_Projection;

			glm::mat4 m_ViewMatrix;
			glm::mat4 m_ProjectionMatrix;
			glm::mat4 m_ViewProjectionMatrix;
			
			EngineBuildingBlocks::Math::BoundingFrustum m_Frustum;

			bool m_IsViewProjectionMatrixRecomputationNeeded;

		public:

			Camera(EngineBuildingBlocks::SceneNodeHandler* sceneNodeHandler);

			const EngineBuildingBlocks::WrappedSceneNode& GetWrappedSceneNode() const;
			EngineBuildingBlocks::WrappedSceneNode& GetWrappedSceneNode();

			EngineBuildingBlocks::SceneNodeHandler* GetSceneNodeHandler();

			// Sets world location and projection to the values obtained
			// from the parameter.
			void Set(Camera& camera);

		public: // Serialization.

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);

		private: // Matrix computing and getting.

			void UpdateViewMatrix();
			void UpdateViewProjectionMatrix();

		public:

			const glm::mat4& GetViewMatrix();
			const glm::mat4& GetProjectionMatrix();
			const glm::mat4& GetViewProjectionMatrix();
			glm::mat4 GetViewMatrixInverse();
			glm::mat4 GetProjectionMatrixInverse();
			glm::mat4 GetViewProjectionMatrixInverse();

			const EngineBuildingBlocks::Math::BoundingFrustum& GetViewFrustum();

			void SetFromViewMatrix(glm::mat4& viewMatrix);

			static void SetViewMatrix(
				const EngineBuildingBlocks::ScaledTransformation& worldTransformation,
				glm::mat4& viewMatrix);

		private: // Projection related functions.

			void SetPerspectiveProjectionMatrix(float left, float right, float bottom, float top,
				float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);

		public:

			CameraData GetData() const;
			void SetData(const CameraData& data);

			const CameraProjection& GetProjection() const;
			void SetProjection(const CameraProjection& projection);

			ProjectionType GetProjectionType() const;

			bool IsProjectingTo_0_1_Interval() const;

			Math::BoundingFrustum GetProjectionOnlyFrustum() const;

			void SetPerspectiveProjection(float fovY, float aspectRatio,
				float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);
			void SetPerspectiveProjection(float left, float right, float bottom, float top,
				float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);
			void SetOrthographicProjection(float left, float right, float bottom, float top,
				float nearPlaneDistance, float farPlaneDistance, bool isProjectingTo_0_1_Interval);

			void SetDefaultPerspectiveProjection(bool isProjectingTo_0_1_Interval);

		public:

			bool HasSameLocationAndProjection(Camera& other, float epsilon = 0.0f);

			// Sets the same local transformation for *this as the parameter object's.
			void SetLocation(Camera& other);

			// Sets the same local transformation and projection for *this
			// as the parameter object's.
			void SetLocationAndProjection(Camera& other);

		public: // Location related functions.

			const glm::vec3& GetLocalPosition() const;
			void SetLocalPosition(const glm::vec3& localPosition);

			const EngineBuildingBlocks::RigidTransformation& GetLocalTransformation() const;
			const glm::mat3& GetLocalOrientation() const;
			void SetLocalOrientation(const glm::mat3& localOrientation);

			const glm::vec3& GetScaler() const;
			void SetScaler(const glm::vec3& scaler);

			EngineBuildingBlocks::ScaledTransformation GetWorldTransformation();
			glm::mat3 GetWorldOrientation();

			EngineBuildingBlocks::ScaledTransformation GetInverseWorldTransformation();

			const EngineBuildingBlocks::ScaledTransformation& GetScaledWorldTransformation();
			const EngineBuildingBlocks::ScaledTransformation& GetInverseScaledWorldTransformation();

			const glm::vec3& GetPosition();
			void SetPosition(const glm::vec3& position);

			glm::vec3 GetDirection();
			glm::vec3 GetUp();
			glm::vec3 GetRight();

			glm::mat3 GetOrientation() const;
			glm::quat GetOrientationQuaternion() const;
			glm::mat3 GetViewOrientation() const;

			void SetOrientation(const glm::mat3& orientation);
			void SetOrientation(const glm::quat& orientation);
			void SetViewOrientation(const glm::mat3& orientation);
			void SetDirectionUp(const glm::vec3& direction, const glm::vec3& up);
			void SetDirection(const glm::vec3& direction);

			void SetLocalTransformation(const EngineBuildingBlocks::RigidTransformation& transformation);
			void SetLocalTransformation(const EngineBuildingBlocks::ScaledTransformation& transformation);
			void SetLocation(const EngineBuildingBlocks::ScaledTransformation& transformation);
			void SetLocation(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up);
			void SetLocation(const glm::vec3& position, const glm::vec3& direction);

			glm::vec3 GetLookAt();
			void LookAt(const glm::vec3& lookAtPosition);
		};
	}
}

#endif