// OpenGLRender/Resources/FrameBufferObject.h

#ifndef OPENGLRENDER_FRAMEBUFFEROBJECT_H_INCLUDED_
#define OPENGLRENDER_FRAMEBUFFEROBJECT_H_INCLUDED_

#include <OpenGLRender/Resources/Texture2D.h>

#include <vector>

namespace OpenGLRender
{
	enum class FrameBufferBindingTarget
	{
		Draw = GL_DRAW_FRAMEBUFFER,
		Read = GL_READ_FRAMEBUFFER,
		DrawAndRead = GL_FRAMEBUFFER
	};

	enum class FrameBufferAttachment
	{
		Color = GL_COLOR_ATTACHMENT0,
		Depth = GL_DEPTH_ATTACHMENT,
		Stencil = GL_STENCIL_ATTACHMENT,
		DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT
	};

	class FrameBufferObject
	{
	public:

		static void DeleteResource(GLuint handle);

	private:

		ResourceHandle<FrameBufferObject> m_Handle;
		std::vector<Texture2D> m_OwnedTextures;

	public:

		FrameBufferObject();

		void Initialize();
		void Delete();
		
		// Calls Delete() and Initialize(). Useful for detaching
		// the attached textures.
		void Reset();

		GLuint GetHandle() const;

		void Bind(FrameBufferBindingTarget target = FrameBufferBindingTarget::DrawAndRead);
		void Unbind(FrameBufferBindingTarget target = FrameBufferBindingTarget::DrawAndRead);

		// Attaches a texture 2D to the frame buffer object. Bind() must be called first.
		void Attach(const Texture2D& texture, FrameBufferAttachment attachmentType,
			FrameBufferBindingTarget bindingTarget = FrameBufferBindingTarget::Draw,
			unsigned attachmentIndex = 0, unsigned arrayIndex = 0,
			unsigned sideIndex = 0, unsigned mipmapIndex = 0);

		// Attaches a AND TAKES OWNERSHIP of texture 2D to the frame buffer object. Bind() must be called first.
		void Attach(Texture2D&& texture, FrameBufferAttachment attachmentType,
			FrameBufferBindingTarget bindingTarget = FrameBufferBindingTarget::Draw,
			unsigned attachmentIndex = 0, unsigned arrayIndex = 0,
			unsigned sideIndex = 0, unsigned mipmapIndex = 0);

		void CheckStatus(FrameBufferBindingTarget bindingTarget = FrameBufferBindingTarget::Draw);
	};
}

#endif