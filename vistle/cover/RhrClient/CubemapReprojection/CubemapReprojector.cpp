// CubemapReprojector.h

// Must be included before OpenGL includes.
#include <External/glew-1.12.0/include/GL/glew.h>

#include <CubemapReprojector.h>

#include <Core/Constants.h>
#include <Core/Percentage.hpp>
#include <Core/Platform.h>
#include <Core/String.hpp>
#include <Core/System/SimpleIO.h>
#include <Core/System/Filesystem.h>
#include <EngineBuildingBlocks/ErrorHandling.h>
#include <OculusVR/SimpleVR_GL.h>

// For debugging:
#include <EngineBuildingBlocks/Graphics/Primitives/PrimitiveCreation.h>
#include <EngineBuildingBlocks/Graphics/Resources/ImageHelper.h>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;
using namespace EngineBuildingBlocks::Input;
using namespace OpenGLRender;
using namespace CubemapStreaming;

inline void PrintException(const std::exception& ex)
{
	printf("An application error has been occured: %s\n", ex.what());
	std::cin.get();
}

///////////////////////////////////// CUBEMAP TEXTURES /////////////////////////////////////

void CubemapReprojector::InitializeColorCubemap(unsigned resourceIndex)
{
	auto& texture = m_ColorTextures[resourceIndex];
	texture.Delete();
	OpenGLRender::Texture2DDescription textureDesc;
	textureDesc.Width = m_ServerWidth;
	textureDesc.Height = m_ServerHeight;
	textureDesc.ArraySize = 1;
	textureDesc.Target = TextureTarget::TextureCubemap;
	textureDesc.Format = TextureFormat::RGBA8;
	textureDesc.HasMipmaps = true;
	TextureSamplingDescription samplingDesc;
	samplingDesc.MinifyingFilter = TextureMinifyingFilter::Linear_Mipmap_Linear;
	samplingDesc.MagnificationFilter = TextureMagnificationFilter::Linear;
	samplingDesc.WrapMode = TextureWrapMode::ClampToEdge;
	texture.Initialize(textureDesc, samplingDesc);

	// Setting 0 initially.
	Core::ByteVectorU initData(c_CountCubemapSides * m_ServerWidth * m_ServerHeight * 4, 0);

	texture.Bind();
	texture.SetData(initData.GetArray(), PixelDataFormat::RGBA, PixelDataType::Uint8);
	texture.GenerateMipmaps();
}

void CubemapReprojector::SetColorCubemapData(unsigned resourceIndex, unsigned char** ppData,
	bool isContainigAlpha, glm::uvec2* start, glm::uvec2* end)
{
	auto& texture = m_ColorTextures[resourceIndex];
	texture.Bind();
	for (unsigned i = 0; i < 6; i++)
	{
		unsigned offset[] = { start[i].x, start[i].y, i };
		unsigned size[] = { end[i].x - start[i].x, end[i].y - start[i].y, 1 };
		texture.SetData(ppData[i], (isContainigAlpha ? PixelDataFormat::RGBA : PixelDataFormat::RGB),
			PixelDataType::Uint8, offset, size, 0);
	}
	texture.GenerateMipmaps();
}

void CubemapReprojector::InitializeDepthTextureArrays(unsigned resourceIndex)
{
	auto& texture = m_DepthTextures[resourceIndex];
	texture.Delete();
	OpenGLRender::Texture2DDescription textureDesc;
	textureDesc.Width = m_ServerWidth;
	textureDesc.Height = m_ServerHeight;
	textureDesc.ArraySize = 6;
	textureDesc.Target = TextureTarget::Texture2DArray;
	textureDesc.Format = TextureFormat::R32F;
	textureDesc.HasMipmaps = true;
	TextureSamplingDescription samplingDesc;
	samplingDesc.MinifyingFilter = TextureMinifyingFilter::Linear_Mipmap_Nearest;
	samplingDesc.MagnificationFilter = TextureMagnificationFilter::Linear;
	samplingDesc.WrapMode = TextureWrapMode::ClampToEdge;
	texture.Initialize(textureDesc, samplingDesc);

	// Setting 1.0f initially.
	Core::SimpleTypeVector<float> initData(c_CountCubemapSides * m_ServerWidth * m_ServerHeight, 1.0f);
	unsigned char* ppData[c_CountCubemapSides];
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		ppData[i] = reinterpret_cast<unsigned char*>(
			initData.GetArray() + i * m_ServerWidth * m_ServerHeight);
	}

	texture.Bind();
	texture.SetData(ppData[0], PixelDataFormat::Red, PixelDataType::Float);
}

void CubemapReprojector::SetDepthData(unsigned resourceIndex, unsigned char** ppData,
	glm::uvec2* start, glm::uvec2* end)
{
	auto& texture = m_DepthTextures[resourceIndex];
	texture.Bind();
	for (unsigned i = 0; i < 6; i++)
	{
		unsigned offset[] = { start[i].x, start[i].y, i };
		unsigned size[] = { end[i].x - start[i].x, end[i].y - start[i].y, 1 };
		texture.SetData(ppData[i], PixelDataFormat::Red, PixelDataType::Float, offset, size, 0);
	}
}

inline void InitializeShaderProgram_VS_PS(const PathHandler& pathHandler,
	const char* vsPath, const char* psPath, OpenGLRender::ShaderProgram& program)
{
	OpenGLRender::ShaderProgramDescription spd;
	spd.Shaders =
	{
		ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(vsPath), ShaderType::Vertex),
		ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(psPath), ShaderType::Fragment)
	};
	program.Initialize(spd);
}

void DebugSourceTexture(const PathHandler& pathHandler, BufferType bufferType)
{
	const bool isClearingBuffers = false;
	static bool isInitializing = true;
	static OpenGLRender::ShaderProgram colorShowProgram, depthShowProgram;
	static Primitive quad;
	if (isInitializing)
	{
		isInitializing = false;

		auto vsPath = "Shaders/ShowBuffer_vs.glsl";
		auto colorPSPath = "Shaders/ShowColorBuffer_ps.glsl";
		auto depthPSPath = "Shaders/ShowDepthBuffer_ps.glsl";

		InitializeShaderProgram_VS_PS(pathHandler, vsPath, colorPSPath, colorShowProgram);
		InitializeShaderProgram_VS_PS(pathHandler, vsPath, depthPSPath, depthShowProgram);

		Vertex_SOA_Data vertexData;
		IndexData indexData;
		CreateQuadGeometry(vertexData, indexData, PrimitiveRange::_Minus1_To_Plus1);
		vertexData.RemoveVertexElement(c_PositionVertexElement);
		vertexData.RemoveVertexElement(c_NormalVertexElement);
		quad.Initialize(OpenGLRender::BufferUsage::StaticDraw, vertexData, indexData);
		
		// Both shader program uses the same input layout.
		colorShowProgram.SetInputLayout(vertexData.InputLayout);
	}

	if (isClearingBuffers)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	GLint prevDepthFunc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
	glDepthFunc(GL_LEQUAL);

	if (bufferType == BufferType::Color)
	{
		colorShowProgram.Bind();
		colorShowProgram.SetUniformValue("ColorTexture", 0);
	}
	else
	{
		depthShowProgram.Bind();
		depthShowProgram.SetUniformValue("DepthTexture", 1);
	}

	quad.Bind();
	glDrawElements(GL_TRIANGLES, quad.CountIndices, GL_UNSIGNED_INT, 0);

	glDepthFunc(prevDepthFunc);
}

void RenderProgressBar(PathHandler& pathHandler)
{
	static bool isInitializing = true;
	static OpenGLRender::ShaderProgram pbProgram;
	static Primitive quad;
	static unsigned countFrames = 0;
	if (isInitializing)
	{
		isInitializing = false;

		auto vsPath = "Shaders/ProgressBar_vs.glsl";
		auto psPath = "Shaders/ProgressBar_ps.glsl";

		OpenGLRender::ShaderProgramDescription spd;
		spd.Shaders =
		{
			ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(vsPath), ShaderType::Vertex),
			ShaderDescription::FromFile(pathHandler.GetPathFromRootDirectory(psPath), ShaderType::Fragment)
		};
		pbProgram.Initialize(spd);

		Vertex_SOA_Data vertexData;
		IndexData indexData;
		CreateQuadGeometry(vertexData, indexData, PrimitiveRange::_Minus1_To_Plus1);
		vertexData.RemoveVertexElement(c_PositionVertexElement);
		vertexData.RemoveVertexElement(c_NormalVertexElement);
		quad.Initialize(OpenGLRender::BufferUsage::StaticDraw, vertexData, indexData);
		pbProgram.SetInputLayout(vertexData.InputLayout);
	}

	pbProgram.Bind();

	pbProgram.SetUniformValue("CountFrames", ++countFrames);

	quad.Bind();
	glDrawElements(GL_TRIANGLES, quad.CountIndices, GL_UNSIGNED_INT, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CubemapReprojector::CubemapReprojector()
	: m_ClientWidth(0)
	, m_ClientHeight(0)
	, m_ServerWidth(2)
	, m_ServerHeight(2)
	, m_ThreadPool(1)
	, m_IsShuttingDown(false)
	, m_ServerCamera(&m_SceneNodeHandler)
	, m_ServerCameraCopy(&m_SceneNodeHandler)
	, m_ServerCameraNew(&m_SceneNodeHandler)
	, m_ServerCameraForAdjustment(&m_SceneNodeHandler)
	, m_ClientCamera(&m_SceneNodeHandler)
	, m_ClientCameraCopy(&m_SceneNodeHandler, &m_KeyHandler, &m_MouseHandler)
	, m_ServerCubemapCameraGroup(&m_ServerCamera)
	, m_ServerCubemapCameraGroupForAdjustment(&m_ServerCameraForAdjustment)
	, m_GridReprojector(m_SceneNodeHandler, &m_IsShuttingDown)
	, m_InitializedOpenGLContextId(Core::c_InvalidIndexU)
	, m_IsVRGraphicsInitialized(false)
	, m_KeyHandler(&m_EventManager)
	, m_MouseHandler(&m_EventManager)
	, m_ConstantViewInverse(0.0f)
{
	LoadConfiguration();

	if (!m_IsRenderingInVR && m_GridReprojector.IsMultisamplingEnabled()) m_CountSamples = 8;
	else m_CountSamples = 1;

	m_GridReprojector.SetClearingBuffers(false);
	m_GridReprojector.SetCullingEnabled(true);

	if (m_IsRenderingInVR)
	{
		InitializeVR();
	}

	InitializeBuffers();

	InitializeInput();
}

void CubemapReprojector::InitializeGL(unsigned openGLContextID)
{
	if (m_InitializedOpenGLContextId != Core::c_InvalidIndexU
		&& m_InitializedOpenGLContextId != openGLContextID)
	{
		printf("Multiple OpenGL contexts are not yet supported.\n");
	}

	if (m_InitializedOpenGLContextId == Core::c_InvalidIndexU)
	{
		InitializeGLEW();
		InitializeGLDebugging();

		auto shadersPath = m_PathHandler.GetPathFromRootDirectory("Shaders/");

		m_IsContainingAlpha = true;

		for (unsigned i = 0; i < c_CountTextures; i++)
		{
			InitializeColorCubemap(i);
			InitializeDepthTextureArrays(i);
		}
		m_CurrentTextureIndex = 0;

		m_GridReprojector.Initialize(shadersPath);

		CheckGLError();

		// Starting shader rebuilding thread.
		m_ThreadPool.GetThread(0).Execute(&ShaderRebuilder::RebuildShaders,
			&m_GridReprojector.GetShaderRebuilder(), &m_PathHandler);

		m_InitializedOpenGLContextId = openGLContextID;

		if (!m_IsRenderingInVR)
		{
			CreateFBO();
		}
	}
}

void CubemapReprojector::CreateFBO()
{
	auto target = (m_CountSamples == 1 ? TextureTarget::Texture2D : TextureTarget::Texture2DMS);
	Texture2D colorTexture(OpenGLRender::Texture2DDescription(m_ClientWidth, m_ClientHeight, TextureFormat::RGBA8, 1, m_CountSamples, target));
	Texture2D depthTexture(OpenGLRender::Texture2DDescription(m_ClientWidth, m_ClientHeight, TextureFormat::DepthComponent32, 1, m_CountSamples, target));
	m_FBO.Initialize();
	m_FBO.Bind();
	m_FBO.Attach(std::move(colorTexture), FrameBufferAttachment::Color);
	m_FBO.Attach(std::move(depthTexture), FrameBufferAttachment::Depth);
}

void CubemapReprojector::Destroy()
{
	m_IsShuttingDown = true;
	m_ThreadPool.Join();

	if (m_IsRenderingInVR)
	{
		ReleaseVR();
	}
}

// Calling this function twice per update for precise results.
void CubemapReprojector::UpdateVRData()
{
	glm::vec3 outputPosition;
	glm::mat3 outputViewOrientation;
	OculusVR::PerspectiveProjectionParameters projParams;

	m_VRHelper->UpdateLocationTracking();

	for (unsigned i = 0; i < 2; i++)
	{
		auto& inputCamera = m_ClientCamera;
		auto& outputCamera = *m_VRCameras[i];

		m_VRHelper->UpdateBeforeRendering(i, inputCamera.GetPosition(), inputCamera.GetViewOrientation(),
			outputPosition, outputViewOrientation, projParams);

		outputCamera.SetPosition(outputPosition);
		outputCamera.SetViewOrientation(outputViewOrientation);
		outputCamera.SetPerspectiveProjection(projParams.Left, projParams.Right, projParams.Bottom, projParams.Top,
			projParams.NearPlaneDistance, projParams.FarPlaneDistance, projParams.IsProjectingFrom_0_To_1);
	}
}

void CubemapReprojector::Render(unsigned openGLContextID)
{
	UpdateInput();

	// Synchronising with updating thread.
	bool isTextureDataAvailable;
	SynchronizeWithUpdating(&isTextureDataAvailable);

	// Getting previous state.
	GLint prevActiveTexture, prevProgram, prevReadFBO, prevDrawFBO;
	{
		glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
		glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
	}

	InitializeGL(openGLContextID);

	m_GridReprojector.Update();

	// We have to postpone VR graphics initialization until this point, since the output FBO is not created
	// at initialization time.
	if (m_IsRenderingInVR)
	{
		InitializeVRGraphics(prevDrawFBO);
	}

	UpdateTextures(isTextureDataAvailable);

	m_ColorTextures[m_CurrentTextureIndex].Bind(0);
	m_DepthTextures[m_CurrentTextureIndex].Bind(1);

	if (m_GridReprojector.IsMultisamplingEnabled()) glEnable(GL_MULTISAMPLE);
	else                                            glDisable(GL_MULTISAMPLE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	if (m_IsRenderingInVR)
	{
		UpdateVRData();

		for (unsigned i = 0; i < 2; i++)
		{
			m_FBOs[i].Bind();
			RenderReprojection(*m_VRCameras[i]);
		}
		m_VRHelper->OnRenderingFinished();
	}
	else
	{
		m_FBO.Bind(FrameBufferBindingTarget::Draw);

		// Reading current content from the target FBO.
		glBindFramebuffer(GL_READ_FRAMEBUFFER, prevDrawFBO);
		glBlitFramebuffer(0, 0, m_ClientWidth, m_ClientHeight, 0, 0, m_ClientWidth, m_ClientHeight,
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		RenderReprojection(m_ClientCamera);

		// Writing rendered content back to the target FBO.
		m_FBO.Bind(FrameBufferBindingTarget::Read);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
		glBlitFramebuffer(0, 0, m_ClientWidth, m_ClientHeight, 0, 0, m_ClientWidth, m_ClientHeight,
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
	}

	// Setting back previous state.
	{
		glActiveTexture(prevActiveTexture);
		glUseProgram(prevProgram);
	}

	CheckGLError();
}

void CubemapReprojector::RenderReprojection(Camera& clientCamera)
{
	switch (m_RenderMode)
	{
	case RenderMode::Default:
		m_GridReprojector.Render(m_ServerCubemapCameraGroup, clientCamera, m_IsUsingWireframe); break;
	case RenderMode::DebugColors:
		DebugSourceTexture(m_PathHandler, BufferType::Color); break;
	case RenderMode::DebugDepths:
		DebugSourceTexture(m_PathHandler, BufferType::Depth); break;
	}
	//RenderProgressBar(m_PathHandler);
}

/////////////////////////////////////////// VR ///////////////////////////////////////////

void CubemapReprojector::InitializeVR()
{
	m_VRHelper.reset(new OculusVR::VR_GL_Helper());
	m_VRHelper->InitializeSDK();

	for (unsigned i = 0; i < 2; i++)
	{
		m_VRCameras[i] = new EngineBuildingBlocks::Graphics::Camera(&m_SceneNodeHandler);
	}
}

void CubemapReprojector::ReleaseVR()
{
	m_VRHelper->Release();

	for (unsigned i = 0; i < 2; i++)
	{
		delete m_VRCameras[i];
	}
}

void CubemapReprojector::InitializeVRGraphics(int prevDrawFBO)
{
	if (!m_IsVRGraphicsInitialized)
	{
		bool isMultisamplingEnabled = m_GridReprojector.IsMultisamplingEnabled();
		unsigned sampleCount = (isMultisamplingEnabled ? 8 : 1);

		OculusVR::VR_GL_InitData vrInitData;
		vrInitData.WindowSize = glm::uvec2(m_ClientWidth, m_ClientHeight);
		vrInitData.SampleCount = sampleCount;
		vrInitData.Flags = OculusVR::VR_Flags::SetUpViewportForRendering;
		vrInitData.OutputFBO = prevDrawFBO;

		for (unsigned i = 0; i < 2; i++)
		{
			auto& fbo = m_FBOs[i];
			auto size = m_VRHelper->GetEyeTextureSize(i);
			Texture2D colorTexture(OpenGLRender::Texture2DDescription(size.x, size.y, TextureFormat::RGBA8, 1, sampleCount));
			Texture2D depthTexture(OpenGLRender::Texture2DDescription(size.x, size.y, TextureFormat::DepthComponent32F, 1, sampleCount));
			fbo.Initialize();
			fbo.Bind();
			fbo.Attach(std::move(colorTexture), FrameBufferAttachment::Color);
			fbo.Attach(std::move(depthTexture), FrameBufferAttachment::Depth);
			vrInitData.InputEyeFBOs[i] = m_FBOs[i].GetHandle();
		}

		m_VRHelper->InitializeGraphics(vrInitData);

		m_IsVRGraphicsInitialized = true;
	}
}

///////////////////////////////////// CONFIGURATION, SERIALIZATION /////////////////////////////////////

template <typename T>
inline void InitializeRootProperty(const Core::Properties& configuration, const char* name, T& property)
{
	configuration.TryGetPropertyValue((std::string("Configuration.") + name).c_str(), property);
}

void CubemapReprojector::LoadConfiguration()
{
	m_Configuration.LoadFromXml(
		m_PathHandler.GetPathFromRootDirectory("Configurations/CubemapStreaming_Client_Configuration.xml").c_str());
	InitializeRootProperty(m_Configuration, "IsRenderingInVR", m_IsRenderingInVR);
	InitializeRootProperty(m_Configuration, "IsLoadingFakeCubemap", m_IsLoadingFakeCubemap);
	InitializeRootProperty(m_Configuration, "IsContainingAlpha", m_IsContainingAlpha);
	InitializeRootProperty(m_Configuration, "SideVisiblePortion", m_SideVisiblePortion);
	InitializeRootProperty(m_Configuration, "PosZVisiblePortion", m_PosZVisiblePortion);
	InitializeRootProperty(m_Configuration, "MinPosDiffForServer", m_MinPosDiffForServer);
	InitializeRootProperty(m_Configuration, "MinDirCrossForServer", m_MinDirCrossForServer);

	m_GridReprojector.LoadConfiguration(m_Configuration);
}

void CubemapReprojector::SaveClientCameraLocation()
{
	auto cameraLocationPath = m_PathHandler.GetPathFromRootDirectory("Temp/CameraLocation.bin");

	Core::ByteVector bytes;
	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		Core::SerializeSB(bytes, Core::ToPlaceHolder(m_ClientCameraCopy.GetLocalOrientation()));
		Core::SerializeSB(bytes, Core::ToPlaceHolder(m_ClientCameraCopy.GetLocalPosition()));
	}
	Core::WriteAllBytes(cameraLocationPath, bytes.GetArray(), bytes.GetSize());
}

void CubemapReprojector::LoadClientCameraLocation()
{
	auto cameraLocationPath = m_PathHandler.GetPathFromRootDirectory("Temp/CameraLocation.bin");

	if (Core::FileExists(cameraLocationPath))
	{
		Core::ByteVector bytes;
		Core::ReadAllBytes(cameraLocationPath, bytes);

		glm::mat3 clientOrientation;
		glm::vec3 clientPosition;
		const unsigned char* pBytes = bytes.GetArray();
		Core::DeserializeSB(pBytes, Core::ToPlaceHolder(clientOrientation));
		Core::DeserializeSB(pBytes, Core::ToPlaceHolder(clientPosition));

		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		m_ClientCameraCopy.SetLocalOrientation(clientOrientation);
		m_ClientCameraCopy.SetLocalPosition(clientPosition);
	}
}

///////////////////////////////////// DATA TRANSFER /////////////////////////////////////

void CubemapReprojector::InitializeBuffers()
{
	m_Buffers.resize(c_CountBuffers);
	m_WriteBufferIndex = 0;
	m_WrittenBufferIndex = Core::c_InvalidIndexU;
	m_ReadBufferIndex = Core::c_InvalidIndexU;
}

unsigned CubemapReprojector::GetBufferIndex(BufferType type, unsigned sideIndex, unsigned tbIndex)
{
	return ((unsigned)type * c_CountCubemapSides + sideIndex) * c_TripleBuffering + tbIndex;
}

unsigned char* CubemapReprojector::GetColorBuffer(unsigned viewIndex)
{
	return m_Buffers[GetBufferIndex(BufferType::Color, ViewToSideIndex(viewIndex), m_WriteBufferIndex)].GetArray();
}

unsigned char* CubemapReprojector::GetDepthBuffer(unsigned viewIndex)
{
	return m_Buffers[GetBufferIndex(BufferType::Depth, ViewToSideIndex(viewIndex), m_WriteBufferIndex)].GetArray();
}

void CubemapReprojector::IncrementWriteBufferIndex()
{
	if (++m_WriteBufferIndex == c_TripleBuffering) m_WriteBufferIndex = 0;
}

inline void SwapX(void* pBuffer, unsigned width, unsigned height, unsigned elementSize)
{
	uint32_t temp;
	assert(elementSize <= sizeof(uint32_t));

	auto leftPtr = reinterpret_cast<uint8_t*>(pBuffer);
	auto rightPtr = leftPtr + (width - 1) * elementSize;
	auto leftPtrLimit = leftPtr + width * height * elementSize;
	auto rowSize = width * elementSize;
	for (; leftPtr < leftPtrLimit; leftPtr += rowSize, rightPtr += rowSize)
	{
		auto cLeftPtr = leftPtr;
		auto cRightPtr = rightPtr;
		for (; cLeftPtr < cRightPtr; cLeftPtr += elementSize, cRightPtr -= elementSize)
		{
			memcpy(&temp, cLeftPtr, elementSize);
			memcpy(cLeftPtr, cRightPtr, elementSize);
			memcpy(cRightPtr, &temp, elementSize);
		}
	}
}

inline void PrepareDepth(void* pBuffer, unsigned width, unsigned height)
{
	// Swapping Y and transforming from [0, 1] to [-1, 1].

	auto ptr1 = reinterpret_cast<float*>(pBuffer);
	auto ptr2 = ptr1 + (height - 1) * width;
	for (; ptr1 < ptr2; ptr1 += width, ptr2 -= width)
	{
		auto cPtr1 = ptr1;
		auto cPtr2 = ptr2;
		auto cPtr1Limit = ptr1 + width;
		for (; cPtr1 < cPtr1Limit; ++cPtr1, ++cPtr2)
		{
			float temp = *cPtr1 * 2.0f - 1.0f;
			*cPtr1 = *cPtr2 * 2.0f - 1.0f;
			*cPtr2 = temp;
		}
	}
}

const bool c_IsDebuggingBuffers = false;

void CubemapReprojector::SwapFrame()
{
	if (!m_IsUpdatingTextures)
		return;

	// TODO: implement swappings in shader!
	glm::uvec2 sideSizes[c_CountCubemapSides];
	GetImageDataSideSize(m_ServerWidth, m_ServerHeight, m_SideVisiblePortion, m_PosZVisiblePortion, sideSizes);
	for (unsigned i = 0; i < c_CountCubemapSides; i++)
	{
		auto& colorBuffer = m_Buffers[GetBufferIndex(BufferType::Color, i, m_WriteBufferIndex)];
		auto& depthBuffer = m_Buffers[GetBufferIndex(BufferType::Depth, i, m_WriteBufferIndex)];
		auto& size = sideSizes[i];
		SwapX(colorBuffer.GetArray(), size.x, size.y, m_IsContainingAlpha ? 4 : 3);
		PrepareDepth(depthBuffer.GetArray(), size.x, size.y);
	}

	if (c_IsDebuggingBuffers)
	{
		for (unsigned sideIndex = 0; sideIndex < 6; sideIndex++)
		{
			auto colorBuffer
				= m_Buffers[GetBufferIndex(BufferType::Color, sideIndex, m_WriteBufferIndex)].GetArray();
			char fileRelPath[32];
			auto& size = sideSizes[sideIndex];
			snprintf(fileRelPath, 32, "Temp/ColorBuffer_%d.png", sideIndex);
			SaveImageToFile(colorBuffer,
				Image2DDescription::ColorImage(size.x, size.y, m_IsContainingAlpha ? 4 : 3),
				m_PathHandler.GetPathFromRootDirectory(fileRelPath),
				ImageSaveFlags::IsFlippingY);
		}
	}

	std::lock_guard<std::mutex> lock(m_RenderSyncMutex);

	m_WrittenBufferIndex = m_WriteBufferIndex;
	IncrementWriteBufferIndex();
	if (m_WriteBufferIndex == m_ReadBufferIndex)
		IncrementWriteBufferIndex();

	m_ServerCameraCopy.Set(m_ServerCameraNew);
}

void CubemapReprojector::SynchronizeWithUpdating(bool* pIsTextureDataAvailable)
{
	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		*pIsTextureDataAvailable = (m_ReadBufferIndex != m_WrittenBufferIndex);
		m_ReadBufferIndex = m_WrittenBufferIndex;

		m_ClientCamera.Set(m_ClientCameraCopy);
		m_ServerCamera.Set(m_ServerCameraCopy);
	}
	m_ServerCubemapCameraGroup.Update();
}

inline bool NeedsResizing(unsigned bufferSize, unsigned width, unsigned height, bool isContainingAlpha)
{
	unsigned pixelSize = (isContainingAlpha ? 4 : 3);
	return (bufferSize != width * height * pixelSize);
}

void CubemapReprojector::UpdateTextures(bool isTextureDataAvailable)
{
	if (isTextureDataAvailable)
	{
		m_CurrentTextureIndex = 1 - m_CurrentTextureIndex;

		auto& checkBuffer = m_Buffers[GetBufferIndex(BufferType::Color, 0, m_ReadBufferIndex)];
		auto& checkTextureDesc = m_ColorTextures[m_CurrentTextureIndex].GetTextureDescription();
		if (NeedsResizing(checkBuffer.GetSizeInBytes(), checkTextureDesc.Width, checkTextureDesc.Height,
			m_IsContainingAlpha))
		{
			InitializeColorCubemap(m_CurrentTextureIndex);
			InitializeDepthTextureArrays(m_CurrentTextureIndex);
		}

		unsigned char* colorDataPointers[c_CountCubemapSides];
		unsigned char* depthDataPointers[c_CountCubemapSides];

		glm::uvec2 totalSize(m_ServerWidth, m_ServerHeight);
		glm::uvec2 sideStarts[c_CountCubemapSides], sideEnds[c_CountCubemapSides];
		GetImageDataSideBounds(m_ServerWidth, m_ServerHeight, m_SideVisiblePortion, m_PosZVisiblePortion,
			sideStarts, sideEnds);

		// Modifying starts and ends for flipped ranges.
		glm::uvec2 colorSideStarts[c_CountCubemapSides], colorSideEnds[c_CountCubemapSides];
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			unsigned yIndex;
			if (i == 2 || i == 3) yIndex = 5 - i;
			else yIndex = i;
			colorSideStarts[i] = glm::uvec2(totalSize.x - sideEnds[i].x, sideStarts[yIndex].y);
			colorSideEnds[i] = glm::uvec2(totalSize.x - sideStarts[i].x, sideEnds[yIndex].y);
		}

		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			colorDataPointers[i] = m_Buffers[GetBufferIndex(BufferType::Color, i, m_ReadBufferIndex)].GetArray();
			depthDataPointers[i] = m_Buffers[GetBufferIndex(BufferType::Depth, i, m_ReadBufferIndex)].GetArray();
		}

		SetColorCubemapData(m_CurrentTextureIndex, colorDataPointers, m_IsContainingAlpha,
			colorSideStarts, colorSideEnds);
		SetDepthData(m_CurrentTextureIndex, depthDataPointers, sideStarts, sideEnds);

		m_GridReprojector.SetTextureData(depthDataPointers, m_ServerWidth, m_ServerHeight,
			m_IsContainingAlpha, m_SideVisiblePortion, m_PosZVisiblePortion,
			m_ServerCubemapCameraGroup);
	}
}

///////////////////////////////////////// OTHER PUBLIC FUNCTIONS /////////////////////////////////////////

glm::mat4 ToMatrix(const double* values)
{
	glm::mat4 m(glm::uninitialize);
	for (int c = 0; c < 16; c++) glm::value_ptr(m)[c] = (float)values[c];
	return m;
}

void CreateFromMatrix(const glm::mat4& m, double* values)
{
	for (int c = 0; c < 16; c++) values[c] = glm::value_ptr(m)[c];
}

RigidTransformation ToRigidTransformation(const double* values)
{
	return RigidTransformation(ToMatrix(values));
}

CameraProjection ToProjection(const double* values)
{
	CameraProjection projection;
	if (!projection.CreateFromMatrix(ToMatrix(values))) printf("Camera projection error!\n");
	return projection;
}

unsigned CubemapReprojector::GetCountSourceViews() const
{
	unsigned countSides = 1;
	if (m_SideVisiblePortion > 0.0f) countSides += 4;
	if (m_PosZVisiblePortion > 0.0f) ++countSides;
	return countSides;
}

unsigned CubemapReprojector::ViewToSideIndex(unsigned viewIndex) const
{
	assert(m_PosZVisiblePortion == 0.0f || m_SideVisiblePortion > 0.0f);
	if (m_SideVisiblePortion == 0.0f)
	{
		assert(viewIndex == 0);
		return (unsigned)CubemapSide::Front;
	}
	else if (m_PosZVisiblePortion == 0.0f)
	{
		assert(viewIndex < c_CountCubemapSides - 1);
		if (viewIndex < (unsigned)CubemapSide::Back) return viewIndex;
		return (unsigned)CubemapSide::Front;
	}
	return viewIndex;
}

unsigned CubemapReprojector::SideToViewIndex(unsigned sideIndex) const
{
	assert(m_PosZVisiblePortion == 0.0f || m_SideVisiblePortion > 0.0f);
	if (m_SideVisiblePortion == 0.0f)
	{
		assert(sideIndex == (unsigned)CubemapSide::Front);
		return 0;
	}
	else if (m_PosZVisiblePortion == 0.0f)
	{
		assert(sideIndex != (unsigned)CubemapSide::Back);
		return std::min(sideIndex, (unsigned)CubemapSide::Back);
	}
	return sideIndex;
}

void CubemapReprojector::AdjustDimensionsAndMatrices(unsigned viewIndex,
	unsigned short clientWidth, unsigned short clientHeight,
	unsigned short* serverWidth, unsigned short* serverHeight,
	const double* leftViewMatrix, const double* rightViewMatrix,
	const double* leftProjMatrix, const double* rightProjMatrix,
	double* viewMatrix, double* projMatrix, double time)
{
	// TODO: we are currently not synchronising dimesions.

	// We assume, that the function is first called with the index = 0.
	if (viewIndex == 0)
	{
		m_ClientWidth = clientWidth;
		m_ClientHeight = clientHeight;

		unsigned serverSize = std::max(*serverWidth, *serverHeight);
		bool isResizingBuffers = (m_ServerWidth != serverSize);
		m_ServerWidth = serverSize;
		m_ServerHeight = serverSize;

		if (isResizingBuffers)
		{
			unsigned countPixels = m_ServerWidth * m_ServerHeight;
			unsigned colorBufferSize = countPixels * (m_IsContainingAlpha ? 4 : 3);
			unsigned depthBufferSize = countPixels * sizeof(float);
			for (unsigned i = 0; i < c_CountCubemapSides; i++)
			{
				for (unsigned j = 0; j < c_TripleBuffering; j++)
				{
					auto& cb = m_Buffers[GetBufferIndex(BufferType::Color, i, j)];
					auto& db = m_Buffers[GetBufferIndex(BufferType::Depth, i, j)];
					cb.Resize(colorBufferSize);
					db.Resize(depthBufferSize);
					cb.SetByte(0);
					db.SetByte(0); // Expecting results in the [0, 1] range.
				}
			}
		}

		auto leftViewTr = ToRigidTransformation(leftViewMatrix);
		auto rightViewTr = ToRigidTransformation(rightViewMatrix);
		assert(leftViewTr.Orientation == rightViewTr.Orientation);
		auto pos = (leftViewTr.Position + rightViewTr.Position) * 0.5f;
		auto viewTr = RigidTransformation(leftViewTr.Orientation, pos);
		m_ServerCameraForAdjustment.SetFromViewMatrix(viewTr.AsMatrix4());

		auto leftProj = ToProjection(leftProjMatrix);
		auto rightProj = ToProjection(rightProjMatrix);
		auto& lp = leftProj.Projection.Perspective;
		auto& rp = rightProj.Projection.Perspective;
		assert(leftProj.Type == ProjectionType::Perspective && rightProj.Type == ProjectionType::Perspective);
		assert(lp.NearPlaneDistance == rp.NearPlaneDistance && lp.FarPlaneDistance == rp.FarPlaneDistance);
		assert(lp.Bottom == rp.Bottom && lp.Top == rp.Top && lp.Right - lp.Left == rp.Right - rp.Left);
		CameraProjection proj;
		proj.CreatePerspecive(1.0f, glm::half_pi<float>(), lp.NearPlaneDistance, lp.FarPlaneDistance, false);
		m_ServerCameraForAdjustment.SetProjection(proj);
		m_ServerCubemapCameraGroupForAdjustment.Update();
	}

	auto sideIndex = ViewToSideIndex(viewIndex);
	auto camera = m_ServerCubemapCameraGroupForAdjustment.Cameras[sideIndex];

	glm::uvec2 sideSizes[c_CountCubemapSides];
	GetImageDataSideSize(m_ServerWidth, m_ServerHeight, m_SideVisiblePortion, m_PosZVisiblePortion, sideSizes);
	*serverWidth = sideSizes[sideIndex].x;
	*serverHeight = sideSizes[sideIndex].y;

	assert(*serverWidth > 0 && *serverHeight > 0);

	if (sideIndex < (unsigned)CubemapSide::Back && m_SideVisiblePortion < 1.0f)
	{
		assert(m_SideVisiblePortion > 0.0f);

		auto sideRatio = std::min(*serverWidth, *serverHeight) / (float)m_ServerWidth;

		auto projection = camera->GetProjection();
		auto& pp = projection.Projection.Perspective;
		switch ((CubemapSide)sideIndex)
		{
		case CubemapSide::Left: pp.Left = pp.Right - (pp.Right - pp.Left) * sideRatio; break;
		case CubemapSide::Right: pp.Right = pp.Left + (pp.Right - pp.Left) * sideRatio; break;
		case CubemapSide::Bottom: pp.Top = pp.Bottom + (pp.Top - pp.Bottom) * sideRatio; break;
		case CubemapSide::Top: pp.Bottom = pp.Top - (pp.Top - pp.Bottom) * sideRatio; break;
		}
		camera->SetProjection(projection);
	}

	CreateFromMatrix(camera->GetViewMatrix(), viewMatrix);
	CreateFromMatrix(camera->GetProjectionMatrix(), projMatrix);
}

inline void SetCameraFromMatrices(Camera& camera, const double* model, const double* view, const double* proj)
{
	auto modelView = ToMatrix(view) * ToMatrix(model);
	camera.SetFromViewMatrix(modelView);
	camera.SetProjection(ToProjection(proj));
}

void CubemapReprojector::SetNewServerCamera(int viewIndex,
	const double* model, const double* view, const double* proj)
{
	if (ViewToSideIndex(viewIndex) == (int)CubemapSide::Front)
	{
		SetCameraFromMatrices(m_ServerCameraNew, model, view, proj);
	}
}

void CubemapReprojector::SetClientCamera(const double* model, const double* view, const double* proj)
{
	bool isFirstFrame = (m_ConstantViewInverse == glm::mat4(0.0f));

	// This condition is incorrect, we should update the client camera copy,
	// but this way we can prevent control feedback. This is only used for debugging.
	if (isFirstFrame || !m_IsClientCameraMovementAllowed)
	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		SetCameraFromMatrices(m_ClientCameraCopy, model, view, proj);
	}
	if (isFirstFrame)
	{
		m_ConstantViewInverse = glm::inverse(ToMatrix(view));
	}
}

void CubemapReprojector::ResizeView(int viewIndex, int w, int h, GLenum depthFormat)
{
	assert(viewIndex == 0);
	
	// Handling view resizing in the AdjustDimensionsAndMatrices(...) function.
}

// Improvements:
//
// - use only some portion of the sides and don't use the +Z face.

// TODO:
//
// - check whether multisampling is working properly
// - check stiching geometry (there seem to be some holes...)

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// INPUT HANDLING //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Note: this is ONLY for debugging the reprojection's behaviour.

#include <EngineBuildingBlocks/Input/DefaultInputBinder.h>

#ifdef IS_WINDOWS

#include <Core/Windows.h>

KeyState CubemapReprojector::GetKeyState(Keys key) const
{
	auto specificKey = m_WindowsKeys.GetSpecificKey(key);
	if (specificKey != 0)
	{
		return (::GetKeyState(specificKey) & 0x8000
			? KeyState::Pressed
			: KeyState::Released);
	}
	return KeyState::Unhandled;
}

MouseButtonState CubemapReprojector::GetMouseButtonState(MouseButton button) const
{
	int specificButton = -1;
	switch (button)
	{
	case MouseButton::Left: specificButton = VK_LBUTTON; break;
	case MouseButton::Middle: specificButton = VK_MBUTTON; break;
	case MouseButton::Right: specificButton = VK_RBUTTON; break;
	}
	if (specificButton == -1) return MouseButtonState::Released;

	return (::GetAsyncKeyState(specificButton) & 0x8000
		? MouseButtonState::Pressed
		: MouseButtonState::Released);
}

void CubemapReprojector::GetCursorPosition(float& cursorPositionX, float& cursorPositionY) const
{
	POINT point;
	::GetCursorPos(&point);
	cursorPositionX = (float)point.x;
	cursorPositionY = (float)point.y;
}

#else

// Dummy implementation.

KeyState CubemapReprojector::GetKeyState(Keys key) const
{
	return KeyState::Unhandled;
}

MouseButtonState CubemapReprojector::GetMouseButtonState(MouseButton button) const
{
	return MouseButtonState::Released;
}

void CubemapReprojector::GetCursorPosition(float& cursorPositionX, float& cursorPositionY) const
{
	cursorPositionX = 0.0f;
	cursorPositionY = 0.0f;
}

#endif

void CubemapReprojector::InitializeInput()
{
	m_KeyHandler.SetKeyStateProvider(this);
	m_MouseHandler.SetMouseStateProvider(this);

	m_SystemTime.Initialize();
	m_KeyHandler.Initialize();
	m_MouseHandler.Initialize();

	DefaultInputBinder::Bind(&m_KeyHandler, &m_MouseHandler, &m_ClientCameraCopy);

	m_LoadClientCameraECI = m_KeyHandler.RegisterStateKeyEventListener("LoadClientCamera", this);
	m_SaveClientCameraECI = m_KeyHandler.RegisterStateKeyEventListener("SaveClientCamera", this);
	m_FlipIsUpdatingTextureECI = m_KeyHandler.RegisterStateKeyEventListener("FlipIsUpdatingTexture", this);
	m_FlipWireFrameECI = m_KeyHandler.RegisterStateKeyEventListener("FlipWireFrame", this);
	m_SaveImageDataECI = m_KeyHandler.RegisterStateKeyEventListener("SaveImageData", this);
	m_StepRenderModeECI = m_KeyHandler.RegisterStateKeyEventListener("StepRenderMode", this);
	m_KeyHandler.BindEventToKey(m_LoadClientCameraECI, Keys::L);
	m_KeyHandler.BindEventToKey(m_SaveClientCameraECI, Keys::K);
	m_KeyHandler.BindEventToKey(m_FlipIsUpdatingTextureECI, Keys::J);
	m_KeyHandler.BindEventToKey(m_FlipWireFrameECI, Keys::U);
	m_KeyHandler.BindEventToKey(m_SaveImageDataECI, Keys::H);
	m_KeyHandler.BindEventToKey(m_StepRenderModeECI, Keys::G);

	m_ClientCameraCopy.SetMaxSpeed(300.0f);
}

bool CubemapReprojector::HandleEvent(const Event* _event)
{
	auto eci = _event->ClassId;
	if (eci == m_LoadClientCameraECI) LoadClientCameraLocation();
	else if (eci == m_SaveClientCameraECI) SaveClientCameraLocation();
	else if (eci == m_FlipIsUpdatingTextureECI) m_IsUpdatingTextures = !m_IsUpdatingTextures;
	else if (eci == m_FlipWireFrameECI) m_IsUsingWireframe = !m_IsUsingWireframe;
	else if (eci == m_SaveImageDataECI) SaveImageData();
	else if (eci == m_StepRenderModeECI)
		m_RenderMode = (RenderMode)(((int)m_RenderMode + 1) % (int)RenderMode::COUNT);
	else return false;
	return true;
}

void CubemapReprojector::UpdateInput()
{
	for (int generalKey = 0; generalKey < (int)Keys::COUNT; generalKey++)
	{
		auto keyState = GetKeyState((Keys)generalKey);
		if (keyState != KeyState::Unhandled)
		{
			m_KeyHandler.OnKeyAction((Keys)generalKey, keyState);
		}
	}
	static MouseButtonState previousButtonState[] = {
		MouseButtonState::Released, MouseButtonState::Released, MouseButtonState::Released };
	static glm::vec2 previousMousePosition(-1.0f, -1.0f);
	for (int button = 0; button < (int)MouseButton::COUNT_HANDLED; button++)
	{
		auto buttonState = GetMouseButtonState((MouseButton)button);
		if (buttonState != previousButtonState[button])
		{
			previousButtonState[button] = buttonState;
			m_MouseHandler.OnMouseButtonAction((MouseButton)button, buttonState);
		}
	}
	glm::vec2 mousePosition;
	GetCursorPosition(mousePosition.x, mousePosition.y);
	if (previousMousePosition != glm::vec2(-1.0f, -1.0f) && mousePosition != previousMousePosition)
	{
		m_MouseHandler.OnCursorPositionChanged(mousePosition.x, mousePosition.y);
	}
	previousMousePosition = mousePosition;

	m_SystemTime.Update();

	m_KeyHandler.Update(m_SystemTime);
	m_MouseHandler.Update(m_SystemTime);

	m_EventManager.HandleEvents();

	if (m_IsClientCameraMovementAllowed)
	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
		m_ClientCameraCopy.Update(m_SystemTime);
	}
}

const double c_FirstModelMatrix[] = {
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	1.0, 0.0, 0.0, 0.0,
	50.0, -1250.0, -200.0, 1.0 };

void CubemapReprojector::GetFirstModelMatrix(double* model)
{
	memcpy(model, c_FirstModelMatrix, sizeof(double) * 16);
}

void CubemapReprojector::GetModelMatrix(double* model, bool* pIsValid)
{
	std::lock_guard<std::mutex> lock(m_RenderSyncMutex);
	auto modelView = m_ClientCameraCopy.GetViewMatrix();
	auto modelMatrix = m_ConstantViewInverse * modelView;
	CreateFromMatrix(modelMatrix, model);
	*pIsValid = (m_ConstantViewInverse != glm::mat4(0.0f));
}

void CubemapReprojector::SetClientCameraMovementAllowed(bool isAllowed)
{
	m_IsClientCameraMovementAllowed = isAllowed;
	m_ClientCameraCopy.SetFrozen(!isAllowed);
}

struct MetaData
{
	unsigned TextureWidth, TextureHeight;
	bool IsContainingAlpha;
	float SideVisiblePortion; // Side 0, 1, 2, 3.
	float PosZVisiblePortion; // Side 4.
	glm::mat3 CameraOrientation;
	glm::vec3 CameraPosition;
	EngineBuildingBlocks::Graphics::CameraProjection CameraProjection;
};

void CubemapReprojector::SaveImageData()
{
	Core::ByteVector imageData;
	Core::ByteVector metaData;
	{
		std::lock_guard<std::mutex> lock(m_RenderSyncMutex);

		metaData.Resize(sizeof(MetaData));
		auto pMetaData = (MetaData*)metaData.GetArray();
		pMetaData->TextureWidth = m_ServerWidth;
		pMetaData->TextureHeight = m_ServerHeight;
		pMetaData->IsContainingAlpha = m_IsContainingAlpha;
		pMetaData->SideVisiblePortion = m_SideVisiblePortion;
		pMetaData->PosZVisiblePortion = m_PosZVisiblePortion;
		pMetaData->CameraOrientation = m_ClientCameraCopy.GetLocalOrientation();
		pMetaData->CameraPosition = m_ClientCameraCopy.GetLocalPosition();
		pMetaData->CameraProjection = m_ClientCameraCopy.GetProjection();

		auto colorCopySize = m_ServerWidth * m_ServerHeight * (m_IsContainingAlpha ? 4 : 3);
		auto depthCopySize = m_ServerWidth * m_ServerHeight * (unsigned)sizeof(float);
		imageData.Resize(c_CountCubemapSides * (colorCopySize + depthCopySize));
		auto pImage = imageData.GetArray();	
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			memcpy(pImage,
				m_Buffers[GetBufferIndex(BufferType::Color, i, m_WrittenBufferIndex)].GetArray(),
				colorCopySize);
			pImage += colorCopySize;
		}
		for (unsigned i = 0; i < c_CountCubemapSides; i++)
		{
			memcpy(pImage,
				m_Buffers[GetBufferIndex(BufferType::Depth, i, m_WrittenBufferIndex)].GetArray(),
				depthCopySize);
			pImage += depthCopySize;
		}
	}

	auto cubemapImageDataPath = m_PathHandler.GetPathFromRootDirectory("Temp/FakeCubemapImage.bin");
	auto cubemapMetadataPath = m_PathHandler.GetPathFromRootDirectory("Temp/FakeCubemapMetadata.bin");

	Core::WriteAllBytes(cubemapImageDataPath, imageData);
	Core::WriteAllBytes(cubemapMetadataPath, metaData);
}