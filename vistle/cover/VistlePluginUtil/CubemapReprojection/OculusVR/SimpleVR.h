// OculusVR/SimpleVR.h

#ifndef _OCULUSVR_SIMPLEVR_H_INCLUDED_
#define _OCULUSVR_SIMPLEVR_H_INCLUDED_

#include <EngineBuildingBlocks/Math/GLM.h>

#include <memory>

namespace OculusVR
{
	class VR_HelperImplementor;

	struct PerspectiveProjectionParameters
	{
		float Left;
		float Right;
		float Bottom;
		float Top;
		float NearPlaneDistance;
		float FarPlaneDistance;
		bool IsProjectingFrom_0_To_1;
	};

	enum VR_Flags
	{
		Empty = 0,

		// Set this flag for automatically setting the viewport size when calling
		// UpdateBeforeRendering(...) to the recommended eye texture size returned
		// by GetEyeTextureSize(...).
		SetUpViewportForRendering = 1
	};

	struct VR_InitData
	{
		glm::uvec2 WindowSize;
		unsigned SampleCount;
		unsigned Flags;

		VR_InitData();
	};

	class VR_Helper
	{
	protected:

		std::unique_ptr<VR_HelperImplementor> m_Implementor;

		bool m_IsVisible;

	public:

		VR_Helper();
		~VR_Helper();

		// Initializing the SDK. Call this method first, then call GetEyeTextureSize(...) to obtain
		// the ideal eye render target size, with which create the these render targets.
		// Then call InitializeGraphics(...) with the render target handles.
		void InitializeSDK();

		// Initializes graphics related resources. Call InitializeSDK first, then GetEyeTextureSize(...)
		// to obtain the ideal eye render target size, with which create the these render targets.
		// Then call this function with the render target handles.
		void InitializeGraphics(const VR_InitData& initData);

		void Release();

		glm::uvec2 GetEyeTextureSize(unsigned eyeIndex) const;

		void UpdateLocationTracking();
		void UpdateBeforeRendering(unsigned eyeIndex,
			const glm::vec3& inputPosition,
			const glm::mat3& inputViewOrientation,
			glm::vec3& outputPosition,
			glm::mat3& outputViewOrientation,
			float cameraNear, float cameraFar,
			PerspectiveProjectionParameters& projectionParameters);
		void OnRenderingFinished();
	};
}

#endif