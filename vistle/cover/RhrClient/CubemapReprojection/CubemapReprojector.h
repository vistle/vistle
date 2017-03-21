// CubemapReprojector.h

#ifndef _CUBEMAPREPROJECTOR_H_INCLUDED_
#define _CUBEMAPREPROJECTOR_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/DataStructures/Properties.h>
#include <Core/System/ThreadPool.h>

#include <EngineBuildingBlocks/Graphics/Camera/Camera.h>
#include <EngineBuildingBlocks/Graphics/Camera/CubemapHelper.hpp>
#include <EngineBuildingBlocks/SystemTime.h>
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
{
	EngineBuildingBlocks::PathHandler m_PathHandler;

	unsigned m_ClientWidth, m_ClientHeight, m_ServerWidth, m_ServerHeight,
		m_CountSamples;

	Core::ThreadPool m_ThreadPool;
	std::atomic<bool> m_IsShuttingDown;

	EngineBuildingBlocks::SceneNodeHandler m_SceneNodeHandler;

	void InitializeGL(unsigned openGLContextID);

private:

	EngineBuildingBlocks::Graphics::Camera m_ServerCamera, m_NewServerCamera,
		m_CurrentServerCamera, m_ServerCameraForAdjustment;
	EngineBuildingBlocks::Graphics::Camera m_ClientCamera, m_ClientCameraCopy;
	EngineBuildingBlocks::Graphics::CubemapCameraGroup m_ServerCubemapCameraGroup, m_ServerCubemapCameraGroupForAdjustment;

	GridReprojector m_GridReprojector;
	
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
	
	std::mutex m_BufferMutex;
	unsigned m_WriteBufferIndex;
	unsigned m_WrittenBufferIndex;
	unsigned m_ReadBufferIndex;

	void InitializeBuffers();
	unsigned GetBufferIndex(BufferType type, unsigned sideIndex, unsigned dbIndex);
	void IncrementWriteBufferIndex();

private: // Textures.

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

	void UpdateTextures();

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

	void SaveCameraLocation();
	void LoadCameraLocation();

private: // VR.

	std::unique_ptr<OculusVR::VR_Helper> m_VRHelper;
	bool m_IsVRGraphicsInitialized;
	OpenGLRender::FrameBufferObject m_FBOs[2];
	EngineBuildingBlocks::Graphics::Camera* m_VRCameras[2];

	void InitializeVR();
	void InitializeVRGraphics(int prevDrawFBO);
	void ReleaseVR();

public:

	CubemapReprojector();

	void Destroy();
	void Render(unsigned openGLContextID);

	void AdjustDimensionsAndMatrices(unsigned sideIndex, 
		unsigned short clientWidth, unsigned short clientHeight,
		unsigned short* serverWidth, unsigned short* serverHeight,
		const double* leftViewMatrix, const double* rightViewMatrix,
		const double* leftProjMatrix, const double* rightProjMatrix,
		double* viewMatrix, double* projMatrix);

	void SetNewServerCameraTransformations(int sideIndex, const double* modelMatrix, const double* viewMatrix, const double* projMatrix);
	void SetClientCameraTransformations(const double* modelMatrix, const double* viewMatrix, const double* projMatrix);

	void ResizeView(int idx, int w, int h, GLenum depthFormat);

	unsigned char* GetColorBuffer(unsigned index);
	unsigned char* GetDepthBuffer(unsigned index);
	void SwapBuffers();

private: // Temporary.

	Core::ByteVectorU m_SwapVector;
	void SwapX(void* pBuffer, unsigned width, unsigned height, unsigned elementSize);
	void SwapY(void* pBuffer, unsigned width, unsigned height, unsigned elementSize);
};

#endif