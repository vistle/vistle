// OpenGLRender/Resources/Texture.h

#ifndef OPENGLRENDER_TEXTURE_H_INCLUDED_
#define OPENGLRENDER_TEXTURE_H_INCLUDED_

#include <OpenGLRender/Resources/Resource.h>
#include <OpenGLRender/Resources/TextureEnums.h>

namespace OpenGLRender
{
	struct TextureSamplingDescription
	{
		TextureMinifyingFilter MinifyingFilter;
		TextureMagnificationFilter MagnificationFilter;
		TextureWrapMode WrapMode;
		TextureComparisonMode ComparisonMode;
		TextureComparisonFunction ComparisonFunction;
		float MaximumAnisotropy;

		TextureSamplingDescription();
	};

	void SetTextureSampler(TextureTarget tTarget,
		const TextureSamplingDescription& description);

	class Texture
	{
	public:

		static void DeleteResource(GLuint handle);

	private:

		ResourceHandle<Texture> m_Handle;

	public:

		void Initialize();
		void Delete();

		GLuint GetHandle() const;
	};
}

#endif