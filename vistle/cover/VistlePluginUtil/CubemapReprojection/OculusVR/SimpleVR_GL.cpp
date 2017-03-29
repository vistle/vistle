// OculusVR/SimpleVR_GL.cpp

#include <OculusVR/SimpleVR_GL.h>

#include <Core/Platform.h>
#include <OculusVR/SimpleVR_HelperImplementor.h>

using namespace OculusVR;

// Including the Oculus SDK.
#include <OculusSDK/LibOVR/Include/OVR_CAPI_GL.h>

#include <glew/include/GL/glew.h>

#ifndef IS_WINDOWS
#include <GL/glx.h>
#endif

struct OculusRenderTarget
{
	ovrSession Session;
	ovrTextureSwapChain TextureChain;
	GLuint FBOId;
	ovrSizei TextureSize;

	OculusRenderTarget(ovrSession session, const ovrSizei& size)
		: Session(session)
		, TextureChain(nullptr)
		, FBOId(0)
	{
		TextureSize = size;

		assert(session);

		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = size.w;
		desc.Height = size.h;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;

		ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

		int length = 0;
		ovr_GetTextureSwapChainLength(session, TextureChain, &length);

		if (OVR_SUCCESS(result))
		{
			for (int i = 0; i < length; ++i)
			{
				GLuint chainTexId;
				ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
				glBindTexture(GL_TEXTURE_2D, chainTexId);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}

		glGenFramebuffers(1, &FBOId);
	}

	~OculusRenderTarget()
	{
		if (TextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, TextureChain);
			TextureChain = nullptr;
		}
		if (FBOId)
		{
			glDeleteFramebuffers(1, &FBOId);
			FBOId = 0;
		}
	}

	const ovrSizei& GetSize() const
	{
		return TextureSize;
	}

	void SetAsCopyTarget()
	{
		assert(TextureChain);

		GLuint curTexId;
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOId);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
	}

	void UnsetCopySurface()
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOId);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	}

	void Commit()
	{
		if (TextureChain)
		{
			ovr_CommitTextureSwapChain(Session, TextureChain);
		}
	}
};

namespace OculusVR
{
	class VR_GL_HelperImplementor : public VR_HelperImplementor
	{
		OculusRenderTarget* m_OculusRenderTargets[2];
		unsigned m_MirrorTextureFBO;

	private: // Host API information.

		unsigned m_InputEyeFBOs[2];
		unsigned m_OutputFBO;

		void SetVSync(int interval)
		{
#ifdef IS_WINDOWS
			auto wglSwapIntervalEXT = reinterpret_cast<BOOL(APIENTRY*)(int)>(wglGetProcAddress("wglSwapIntervalEXT"));
			if (wglSwapIntervalEXT != nullptr)
			{
				wglSwapIntervalEXT(interval);
			}
#else
			if (strstr((const char*)glGetString(GL_EXTENSIONS), "glXSwapIntervalEXT") != nullptr)
			{
				auto glXSwapIntervalEXT = reinterpret_cast<void(*)(Display*, GLXDrawable, int)>(
					glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT"));
				auto display = glXGetCurrentDisplay();
				auto drawable = glXGetCurrentDrawable();
				if (drawable) glXSwapIntervalEXT(display, drawable, interval);
			}
#endif
		}

	public:

		void SetHostEnvironment(const VR_InitData& initData)
		{
			auto& glInitData = static_cast<const VR_GL_InitData&>(initData);

			for (unsigned i = 0; i < 2; i++)
			{
				m_InputEyeFBOs[i] = glInitData.InputEyeFBOs[i];
			}
			m_OutputFBO = glInitData.OutputFBO;

			m_WindowSize = initData.WindowSize;
			m_SampleCount = initData.SampleCount;
			m_IsProjectingFrom_0_To_1 = false;
			Flags = initData.Flags;

			// Turn off vsync to let the compositor do its magic.
			SetVSync(0);
		}

		void ReleaseResources()
		{
			for (int eye = 0; eye < 2; eye++)
			{
				delete m_OculusRenderTargets[eye];
			}
		}

		void CreateEyeBuffers()
		{
			for (int eye = 0; eye < 2; eye++)
			{
				auto& size = m_EyeRenderViewport[eye].Size;
				m_OculusRenderTargets[eye] = new OculusRenderTarget(m_Session, size);
			}
		}

		void CreateMirrorForMonitor()
		{
			m_MirrorTexture = nullptr;
			ovrMirrorTextureDesc td = {};
			td.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
			td.Width = m_WindowSize.x;
			td.Height = m_WindowSize.y;
			auto result = ovr_CreateMirrorTextureGL(m_Session, &td, &m_MirrorTexture);
			VALIDATE(result == ovrSuccess, "ovr_CreateMirrorTextureGL failed.");

			GLuint mirrorTextureId;
			ovr_GetMirrorTextureBufferGL(m_Session, m_MirrorTexture, &mirrorTextureId);

			glGenFramebuffers(1, &m_MirrorTextureFBO);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorTextureFBO);
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
			glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		void SetUpViewportForRendering(unsigned eyeIndex)
		{
			auto& size = m_EyeRenderViewport[eyeIndex].Size;
			glViewport(0, 0, size.w, size.h);
		}

		void RenderMirror()
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorTextureFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_OutputFBO);
			GLint w = m_WindowSize.x;
			GLint h = m_WindowSize.y;
			glBlitFramebuffer(0, h, w, 0,
				0, 0, w, h,
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		void DistortAndPresent(bool& isVisible)
		{
			ovrViewScaleDesc viewScaleDesc;
			viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
			viewScaleDesc.HmdToEyeOffset[0] = m_HMDToEyeOffset[0];
			viewScaleDesc.HmdToEyeOffset[1] = m_HMDToEyeOffset[1];
			ovrLayerEyeFov ld;
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = 0;
			for (int eye = 0; eye < 2; eye++)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_InputEyeFBOs[eye]);
				m_OculusRenderTargets[eye]->SetAsCopyTarget();

				auto& size = m_OculusRenderTargets[eye]->GetSize();
				GLint w = size.w;
				GLint h = size.h;
				glViewport(0, 0, w, h);
				glBlitFramebuffer(0, 0, w, h,
					0, h, w, 0,
					GL_COLOR_BUFFER_BIT, GL_NEAREST);

				m_OculusRenderTargets[eye]->UnsetCopySurface();
				m_OculusRenderTargets[eye]->Commit();
				ld.ColorTexture[eye] = m_OculusRenderTargets[eye]->TextureChain;
				ld.Viewport[eye] = m_EyeRenderViewport[eye];
				ld.Fov[eye] = m_HMDInfo.DefaultEyeFov[eye];
				ld.RenderPose[eye] = m_EyeRenderPose[eye];
			}
			ovrLayerHeader* layers = &ld.Header;
			isVisible = (ovr_SubmitFrame(m_Session, 0, &viewScaleDesc, &layers, 1) == ovrSuccess);
		}
	};
}

VR_GL_InitData::VR_GL_InitData()
	: OutputFBO(0)
{
	InputEyeFBOs[0] = 0;
	InputEyeFBOs[1] = 0;
}

VR_GL_Helper::VR_GL_Helper()
{
	m_Implementor.reset(new VR_GL_HelperImplementor());
}

VR_GL_Helper::~VR_GL_Helper()
{
}