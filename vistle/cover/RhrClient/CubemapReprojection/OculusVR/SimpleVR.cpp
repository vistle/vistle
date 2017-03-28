// OculusVR/SimpleVR.cpp

#include <OculusVR/SimpleVR.h>

#include <OculusVR/SimpleVR_HelperImplementor.h>

using namespace OculusVR;

VR_InitData::VR_InitData()
	: SampleCount(1)
	, Flags(0)
{
}

VR_Helper::VR_Helper()
	: m_IsVisible(true)
{
}

VR_Helper::~VR_Helper()
{
}

void VR_Helper::InitializeSDK()
{
	m_Implementor->InitializeSDK();
}

void VR_Helper::InitializeGraphics(const VR_InitData& initData)
{
	m_Implementor->SetHostEnvironment(initData);
	m_Implementor->CreateEyeBuffers();
	m_Implementor->CreateMirrorForMonitor();
}

void VR_Helper::Release()
{
	m_Implementor->ReleaseMirrorTexture();
	m_Implementor->ReleaseResources();
	m_Implementor->ReleaseSDK();
}

glm::uvec2 VR_Helper::GetEyeTextureSize(unsigned eyeIndex) const
{
	return m_Implementor->GetEyeTextureSize(eyeIndex);
}

void VR_Helper::UpdateBeforeRendering(unsigned eyeIndex,
	const glm::vec3& inputPosition,
	const glm::mat3& inputViewOrientation,
	glm::vec3& outputPosition,
	glm::mat3& outputViewOrientation,
	float cameraNear, float cameraFar,
	PerspectiveProjectionParameters& projectionParameters)
{
	m_Implementor->SetCamera(eyeIndex, inputPosition, inputViewOrientation, outputPosition, outputViewOrientation,
		cameraNear, cameraFar, projectionParameters);

	if ((m_Implementor->Flags & VR_Flags::SetUpViewportForRendering) != 0)
	{
		m_Implementor->SetUpViewportForRendering(eyeIndex);
	}
}

void VR_Helper::OnRenderingFinished()
{
	m_Implementor->DistortAndPresent(m_IsVisible);
	m_Implementor->RenderMirror();
}

void VR_Helper::UpdateLocationTracking()
{
	m_Implementor->UpdateLocationTracking();
}