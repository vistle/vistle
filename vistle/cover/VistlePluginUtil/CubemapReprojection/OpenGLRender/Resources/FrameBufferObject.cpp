// OpenGLRender/Resources/FrameBufferObject.cpp

#include <OpenGLRender/Resources/FrameBufferObject.h>

#include <EngineBuildingBlocks/ErrorHandling.h>

using namespace OpenGLRender;

FrameBufferObject::FrameBufferObject()
{
}

void FrameBufferObject::Initialize()
{
	glGenFramebuffers(1, m_Handle.GetPointer());
}

void FrameBufferObject::Delete()
{
	m_Handle.Delete();
	m_OwnedTextures.clear();
}

const std::vector<Texture2D>& FrameBufferObject::GetOwnedTextures()
{
	return m_OwnedTextures;
}

void FrameBufferObject::Reset()
{
	Delete();
	Initialize();
}

void FrameBufferObject::DeleteResource(GLuint handle)
{
	glDeleteFramebuffers(1, &handle);
}

GLuint FrameBufferObject::GetHandle() const
{
	return m_Handle;
}

void FrameBufferObject::Bind(FrameBufferBindingTarget target)
{
	glBindFramebuffer((GLenum)target, m_Handle);
}

void FrameBufferObject::Unbind(FrameBufferBindingTarget target)
{
	glBindFramebuffer((GLenum)target, 0);
}

void FrameBufferObject::Attach(const Texture2D& texture, FrameBufferAttachment attachmentType,
	FrameBufferBindingTarget bindingTarget,
	unsigned attachmentIndex, unsigned arrayIndex, unsigned sideIndex, unsigned mipmapIndex)
{
	auto textureHandle = texture.GetHandle();
	auto textureTarget = texture.GetTextureDescription().Target;
	auto attachmentPoint = (GLenum)attachmentType + attachmentIndex;

	switch (texture.GetTextureDescription().Target)
	{
	case TextureTarget::Texture2D:
	case TextureTarget::Texture2DMS:
		glFramebufferTexture2D((GLenum)bindingTarget, attachmentPoint, (GLenum)textureTarget, textureHandle,
			mipmapIndex); break;
	case TextureTarget::TextureCubemap:
		glFramebufferTexture2D((GLenum)bindingTarget, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + sideIndex,
			textureHandle, mipmapIndex); break;
	case TextureTarget::Texture2DArray:
	case TextureTarget::Texture2DMSArray:
		glFramebufferTextureLayer((GLenum)bindingTarget, attachmentPoint, textureHandle, mipmapIndex, arrayIndex);
		break;
	case TextureTarget::TextureCubemapArray:
		assert(false && "Cannot attach a cubemap array to a frame buffer object."); break;
	default: break;
	}
}

void FrameBufferObject::Attach(Texture2D&& texture, FrameBufferAttachment attachmentType,
	FrameBufferBindingTarget bindingTarget,
	unsigned attachmentIndex, unsigned arrayIndex, unsigned sideIndex, unsigned mipmapIndex)
{
	Attach(texture, attachmentType, bindingTarget, attachmentIndex, arrayIndex, sideIndex, mipmapIndex);
	m_OwnedTextures.push_back(std::move(texture));
}

void FrameBufferObject::CheckStatus(FrameBufferBindingTarget bindingTarget)
{
	auto status = glCheckFramebufferStatus((GLenum)bindingTarget);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::stringstream ss;
		ss << "Error when checking frame buffer object status: ";
		switch (status)
		{
		case GL_FRAMEBUFFER_UNDEFINED: ss << "undefined"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: ss << "incomplete attachment"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: ss << "incomplete missing attachment"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: ss << "incomplete draw buffer"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: ss << "incomplete read buffer"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED: ss << "unsupported"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: ss << "incomplete multisample"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: ss << "incomplete layer targets"; break;
		}
		EngineBuildingBlocks::RaiseException(ss);
	}
}