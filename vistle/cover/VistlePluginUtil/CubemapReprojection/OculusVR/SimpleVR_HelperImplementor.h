// OculusVR/SimpleVR_HelperImplementor.h

#ifndef _OCULUSVR_SIMPLEVR_HELPERIMPLEMENTOR_H_INCLUDED_
#define _OCULUSVR_SIMPLEVR_HELPERIMPLEMENTOR_H_INCLUDED_

#include <OculusVR/SimpleVR.h>
#include <Core/Platform.h>

#include <OculusSDK/LibOVR/Include/OVR_CAPI.h>

#ifdef IS_WINDOWS

#include <Core/Windows.h>
#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "SimpleVR", MB_ICONERROR | MB_OK); exit(-1); }
#endif

#else

#include <stdexcept>

#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { throw std::runtime_error(msg); }
#endif

#endif

namespace OculusVR
{
	class VR_HelperImplementor
	{
	protected:

		ovrSession m_Session;
		ovrHmdDesc m_HMDInfo;
		ovrVector3f m_HMDToEyeOffset[2];
		ovrPosef m_EyeRenderPose[2];
		ovrRecti m_EyeRenderViewport[2];

		ovrEyeRenderDesc m_EyeRenderDesc[2];

		ovrMirrorTexture m_MirrorTexture;

		glm::uvec2 m_WindowSize;
		unsigned m_SampleCount;
		bool m_IsProjectingFrom_0_To_1;

	public:

		unsigned Flags;

		VR_HelperImplementor()
			: m_SampleCount(1)
		{
		}

		virtual ~VR_HelperImplementor()
		{
		}

		void InitializeSDK()
		{
			ovrResult result = ovr_Initialize(nullptr);
			VALIDATE(result == ovrSuccess, "Failed to initialize libOVR.");
			ovrGraphicsLuid luid;
			result = ovr_Create(&m_Session, &luid);
			VALIDATE(result == ovrSuccess, "Oculus Rift not detected.");
			m_HMDInfo = ovr_GetHmdDesc(m_Session);

			for (int eye = 0; eye < 2; eye++)
			{
				ovrSizei idealSize = ovr_GetFovTextureSize(m_Session, (ovrEyeType)eye, m_HMDInfo.DefaultEyeFov[eye], 1.0f);
				m_EyeRenderViewport[eye].Pos.x = 0;
				m_EyeRenderViewport[eye].Pos.y = 0;
				m_EyeRenderViewport[eye].Size = idealSize;
			}

			m_EyeRenderDesc[0] = ovr_GetRenderDesc(m_Session, ovrEye_Left, m_HMDInfo.DefaultEyeFov[0]);
			m_EyeRenderDesc[1] = ovr_GetRenderDesc(m_Session, ovrEye_Right, m_HMDInfo.DefaultEyeFov[1]);
		}

		void ReleaseSDK()
		{
			ovr_Destroy(m_Session);
			ovr_Shutdown();
		}

		void ReleaseMirrorTexture()
		{
			ovr_DestroyMirrorTexture(m_Session, m_MirrorTexture);
		}

		glm::uvec2 GetEyeTextureSize(unsigned eyeIndex) const
		{
			auto& size = m_EyeRenderViewport[eyeIndex].Size;
			return glm::uvec2(size.w, size.h);
		}

		void UpdateLocationTracking()
		{
			m_HMDToEyeOffset[0] = m_EyeRenderDesc[0].HmdToEyeOffset;
			m_HMDToEyeOffset[1] = m_EyeRenderDesc[1].HmdToEyeOffset;
			double           ftiming = ovr_GetPredictedDisplayTime(m_Session, 0);
			ovrTrackingState hmdState = ovr_GetTrackingState(m_Session, ftiming, ovrTrue);
			ovr_CalcEyePoses(hmdState.HeadPose.ThePose, m_HMDToEyeOffset, m_EyeRenderPose);
		}

		void SetCamera(unsigned eyeIndex,
			const glm::vec3& inputPosition,
			const glm::mat3& inputViewOrientation,
			glm::vec3& outputPosition,
			glm::mat3& outputViewOrientation,
			float cameraNear, float cameraFar,
			PerspectiveProjectionParameters& projectionParameters)
		{
			// Setting camera view.
			auto eyePosition = glm::vec3(m_EyeRenderPose[eyeIndex].Position.x, m_EyeRenderPose[eyeIndex].Position.y,
				m_EyeRenderPose[eyeIndex].Position.z);
			auto eyeOrientation = glm::mat3_cast(glm::quat(m_EyeRenderPose[eyeIndex].Orientation.w, m_EyeRenderPose[eyeIndex].Orientation.x,
				m_EyeRenderPose[eyeIndex].Orientation.y, m_EyeRenderPose[eyeIndex].Orientation.z));

			outputPosition = inputPosition + inputViewOrientation * eyePosition;
			outputViewOrientation = inputViewOrientation * eyeOrientation;

			// Setting camera projection.
			float n = cameraNear;
			projectionParameters.NearPlaneDistance = n;
			projectionParameters.FarPlaneDistance = cameraFar;
			projectionParameters.Left = -m_EyeRenderDesc[eyeIndex].Fov.LeftTan * n;
			projectionParameters.Right = m_EyeRenderDesc[eyeIndex].Fov.RightTan * n;
			projectionParameters.Bottom = -m_EyeRenderDesc[eyeIndex].Fov.DownTan * n;
			projectionParameters.Top = m_EyeRenderDesc[eyeIndex].Fov.UpTan * n;
			projectionParameters.IsProjectingFrom_0_To_1 = m_IsProjectingFrom_0_To_1;
		}

		virtual void SetHostEnvironment(const VR_InitData& initData) = 0;
		virtual void CreateEyeBuffers() = 0;
		virtual void CreateMirrorForMonitor() = 0;
		virtual void ReleaseResources() = 0;
		virtual void SetUpViewportForRendering(unsigned eyeIndex) = 0;
		virtual void DistortAndPresent(bool& isVisible) = 0;
		virtual void RenderMirror() = 0;
	};
}

#endif