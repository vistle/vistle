// CubemapReprojector.h

#ifndef _CUBEMAPREPROJECTOR_H_INCLUDED_
#define _CUBEMAPREPROJECTOR_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/Properties.h>
#include <Core/System/ThreadPool.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Graphics/Camera/FreeCamera.h>
#include <EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp>
#include <EngineBuildingBlocks/SystemTime.h>
#include <EngineBuildingBlocks/EventHandling.h>
#include <EngineBuildingBlocks/Input/KeyHandler.h>
#include <EngineBuildingBlocks/Input/MouseHandler.h>
#include <EngineBuildingBlocks/Input/WindowsKeys.h>
#include <OpenGLRender/Resources/Texture2D.h>
#include <OpenGLRender/Resources/FrameBufferObject.h>

#include <OculusVR/SimpleVR.h>

#include <GridReprojector.h>
#include <Message.h>
#include <Settings.h>

#include <atomic>

enum class BufferType
{
	Color = 0, Depth, COUNT
};

class CubemapReprojector
	: public EngineBuildingBlocks::Input::IKeyStateProvider
	, public EngineBuildingBlocks::Input::IMouseStateProvider
	, public EngineBuildingBlocks::IEventListener
{
	EngineBuildingBlocks::PathHandler m_PathHandler;
	EngineBuildingBlocks::EventManager m_EventManager;

	EngineBuildingBlocks::Input::KeyHandler m_KeyHandler;
	EngineBuildingBlocks::Input::MouseHandler m_MouseHandler;
	EngineBuildingBlocks::Input::WindowsKeyMap m_WindowsKeys;

	EngineBuildingBlocks::SystemTime m_SystemTime;

	unsigned m_ClientWidth, m_ClientHeight, m_ServerWidth, m_ServerHeight,
		m_CountSamples;

	Core::ThreadPool m_ThreadPool;
	std::atomic<bool> m_IsShuttingDown;

	EngineBuildingBlocks::SceneNodeHandler m_SceneNodeHandler;

	void InitializeGL(unsigned openGLContextID);

public: // Keys and mouse.

	EngineBuildingBlocks::Input::KeyState GetKeyState(EngineBuildingBlocks::Input::Keys key) const;

	unsigned m_LoadClientCameraECI;
	unsigned m_SaveClientCameraECI;
	unsigned m_FlipIsUpdatingTextureECI;
	unsigned m_FlipWireFrameECI;
	unsigned m_SaveImageDataECI;
	unsigned m_StepRenderModeECI;

	bool HandleEvent(const EngineBuildingBlocks::Event* _event);

	EngineBuildingBlocks::Input::MouseButtonState GetMouseButtonState(EngineBuildingBlocks::Input::MouseButton button) const;
	void GetCursorPosition(float& cursorPositionX, float& cursorPositionY) const;

	void InitializeInput();
	void UpdateInput();

private:

	EngineBuildingBlocks::Graphics::Camera m_ServerCamera, m_ServerCameraCopy, 
		m_ServerCameraNew, m_ServerCameraForAdjustment;
	EngineBuildingBlocks::Graphics::Camera m_ClientCamera;
	EngineBuildingBlocks::Graphics::FreeCamera m_ClientCameraCopy;
	EngineBuildingBlocks::Graphics::CubemapCameraGroup m_ServerCubemapCameraGroup, m_ServerCubemapCameraGroupForAdjustment;

	glm::mat4 m_ConstantViewInverse;
	bool m_IsClientCameraMovementAllowed = false;

	GridReprojector m_GridReprojector;
	bool m_IsUsingWireframe = false;
	
	std::mutex m_RenderSyncMutex;

	OpenGLRender::FrameBufferObject m_FBO;

	void CreateFBO();

	void UpdateVRData();
	void RenderReprojection(
		EngineBuildingBlocks::Graphics::Camera& clientCamera);

private: // Buffers.

	static const unsigned c_CountCubemapSides = 6;
	static const unsigned c_TripleBuffering = 3;
	static const unsigned c_CountBuffers
		= ((unsigned)BufferType::COUNT) * c_CountCubemapSides * c_TripleBuffering;

	std::vector<Core::ByteVectorU> m_Buffers;
	
	unsigned m_WriteBufferIndex;
	unsigned m_WrittenBufferIndex;
	unsigned m_ReadBufferIndex;

	void InitializeBuffers();
	unsigned GetBufferIndex(BufferType type, unsigned sideIndex, unsigned dbIndex);
	void IncrementWriteBufferIndex();

	void SynchronizeWithUpdating(bool* pIsTextureDataAvailable);

	void SaveImageData();

private: // Textures.

	bool m_IsUpdatingTextures = true;

	unsigned m_InitializedOpenGLContextId;
	OpenGLRender::Texture2D m_ColorTextures[c_CountTextures];
	OpenGLRender::Texture2D m_DepthTextures[c_CountTextures];

	unsigned m_CurrentTextureIndex;

	void InitializeColorCubemap(unsigned resourceIndex);
	void InitializeDepthTextureArrays(unsigned resourceIndex);
	void SetColorCubemapData(unsigned resourceIndex, unsigned char** ppData,
		bool isContainigAlpha, glm::uvec2* start, glm::uvec2* end);
	void SetDepthData(unsigned resourceIndex, unsigned char** ppData,
		glm::uvec2* start, glm::uvec2* end);

	void UpdateTextures(bool isTextureDataAvailable);

private: // Configuration.

	Core::Properties m_Configuration;

	void LoadConfiguration();

	bool m_IsRenderingInVR = false;
	bool m_IsLoadingFakeCubemap = false;
	bool m_IsContainingAlpha = false;
	float m_SideVisiblePortion = 0.0f;
	float m_PosZVisiblePortion = 0.0f;
	float m_MinPosDiffForServer = 0.0f;
	float m_MinDirCrossForServer = 0.0f;

private: // Serialization.

	void SaveClientCameraLocation();
	void LoadClientCameraLocation();

private: // VR.

	std::unique_ptr<OculusVR::VR_Helper> m_VRHelper;
	bool m_IsVRGraphicsInitialized;
	OpenGLRender::FrameBufferObject m_FBOs[2];
	EngineBuildingBlocks::Graphics::Camera* m_VRCameras[2];

	void InitializeVR();
	void InitializeVRGraphics();
	void ReleaseVR();

private: // Misc.

	enum class RenderMode
	{
		Default = 0, DebugColors, DebugDepths, COUNT
	};

	RenderMode m_RenderMode = RenderMode::Default;

	unsigned ViewToSideIndex(unsigned viewIndex) const;
	unsigned SideToViewIndex(unsigned sideIndex) const;

public:

	CubemapReprojector();

	void Destroy();
	void Render(unsigned openGLContextID);

	void AdjustDimensionsAndMatrices(unsigned viewIndex,
		unsigned short clientWidth, unsigned short clientHeight,
		unsigned short* serverWidth, unsigned short* serverHeight,
		const double* leftViewMatrix, const double* rightViewMatrix,
		const double* leftProjMatrix, const double* rightProjMatrix,
		double* viewMatrix, double* projMatrix, double time);

	void SetNewServerCamera(int viewIndex,
		const double* model, const double* view, const double* proj);

	void SetClientCamera(const double* model, const double* view, const double* proj);

	void ResizeView(int viewIndex, int w, int h, GLenum depthFormat);

	unsigned char* GetColorBuffer(unsigned viewIndex);
	unsigned char* GetDepthBuffer(unsigned viewIndex);
	void SwapFrame();

	void GetFirstModelMatrix(double* model);
	void GetModelMatrix(double* model, bool* pIsValid);
	
	void SetClientCameraMovementAllowed(bool isAllowed);

	unsigned GetCountSourceViews() const;
};

#endif