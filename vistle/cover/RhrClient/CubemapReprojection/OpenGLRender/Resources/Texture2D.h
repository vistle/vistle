// OpenGLRender/Resources/Texture2D.h

#ifndef OPENGLRENDER_TEXTURE2D_H_INCLUDED_
#define OPENGLRENDER_TEXTURE2D_H_INCLUDED_

#include <OpenGLRender/Resources/Texture.h>

namespace OpenGLRender
{
	struct Texture2DDescription
	{
		unsigned Width;
		unsigned Height;
		unsigned ArraySize;
		unsigned SampleCount;
		TextureTarget Target;
		TextureFormat Format;
		bool HasMipmaps;

		Texture2DDescription();
		Texture2DDescription(unsigned width, unsigned height,
			TextureFormat format,
			unsigned arraySize = 1, unsigned sampleCount = 1,
			TextureTarget target = TextureTarget::Texture2D,
			bool hasMipmaps = false);
		bool IsCubemap() const;
		bool IsArray() const;
		bool IsMultisampled() const;
		unsigned GetCountMipmaps() const;
	};

	class Texture2D
	{
		Texture m_Texture;
		Texture2DDescription m_TextureDescription;
		TextureSamplingDescription m_SamplingDescription;

	public:

		Texture2D();
		Texture2D(const Texture2DDescription& textureDesc,
			const TextureSamplingDescription& samplingDesc = TextureSamplingDescription());

		void Initialize(const Texture2DDescription& textureDesc,
			const TextureSamplingDescription& samplingDesc = TextureSamplingDescription());
		void Delete();

		GLuint GetHandle() const;

		const Texture2DDescription& GetTextureDescription() const;
		const TextureSamplingDescription& GetSamplingDescription() const;

		void Bind();
		void Unbind();

		void Bind(unsigned unitIndex);
		void Unbind(unsigned unitIndex);

		// Sets the pixel data for the texture. Bind() must be called first.
		void SetData(const void* pData, PixelDataFormat pixelDataFormat,
			PixelDataType pixelDataType);

		// Sets the pixel data for the texture. Bind() must be called first.
		// Offsets and sizes are defined as:
		// Texture2D:       x, y
		// Texture2DArray:  x, y, arrayIndex
		// Cubemap:         x, y, faceIndex
		// CubemapArray:    x, y, arrayIndex, faceIndex
		void SetData(const void* pData, PixelDataFormat pixelDataFormat,
			PixelDataType pixelDataType,
			const unsigned* pOffset, const unsigned* pSize,
			unsigned mipmapLevel);
		
		// Generates mipmap for the texture. Bind() must be called first.
		void GenerateMipmaps();
	};
}

#endif