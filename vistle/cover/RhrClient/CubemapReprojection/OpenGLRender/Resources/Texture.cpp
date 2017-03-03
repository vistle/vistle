// OpenGLRender/Resources/Texture.cpp

#include <OpenGLRender/Resources/Texture.h>

using namespace OpenGLRender;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

TextureSamplingDescription::TextureSamplingDescription()
	: MinifyingFilter(TextureMinifyingFilter::Linear_Mipmap_Linear)
	, MagnificationFilter(TextureMagnificationFilter::Linear)
	, WrapMode(TextureWrapMode::Repeat)
	, ComparisonMode(TextureComparisonMode::None)
	, ComparisonFunction(TextureComparisonFunction::Always)
	, MaximumAnisotropy(1.0f)
{
}

namespace OpenGLRender
{
	void SetTextureSampler(TextureTarget tTarget, const TextureSamplingDescription& description)
	{
		auto target = (GLenum)tTarget;

		// Setting border color.
		float borderColor[]{ 0.0f, 0.0f, 0.0f, 0.0f };
		glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, borderColor);

		// Setting filters and wrapping.
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (GLenum)description.MinifyingFilter);
		glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, description.MaximumAnisotropy);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, (GLenum)description.MagnificationFilter);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLenum)description.WrapMode);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLenum)description.WrapMode);
		glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, (GLenum)description.ComparisonMode);
		glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, (GLenum)description.ComparisonFunction);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void Texture::Initialize()
{
	glGenTextures(1, m_Handle.GetPointer());
}

void Texture::DeleteResource(GLuint handle)
{
	glDeleteTextures(1, &handle);
}

void Texture::Delete()
{
	m_Handle.Delete();
}

GLuint Texture::GetHandle() const
{
	return m_Handle;
}