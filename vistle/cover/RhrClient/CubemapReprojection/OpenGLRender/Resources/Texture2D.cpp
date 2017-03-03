// OpenGLRender/Resources/Texture2D.cpp

#include <OpenGLRender/Resources/Texture2D.h>

#include <EngineBuildingBlocks/Graphics/Resources/ResourceUtility.h>

#include <cassert>

using namespace OpenGLRender;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

Texture2DDescription::Texture2DDescription()
	: Width(0)
	, Height(0)
	, ArraySize(1)
	, SampleCount(1)
	, Target(TextureTarget::Texture2D)
	, Format(TextureFormat::Unknown)
	, HasMipmaps(false)
{
}

Texture2DDescription::Texture2DDescription(unsigned width, unsigned height, TextureFormat format,
	unsigned arraySize, unsigned sampleCount, TextureTarget target, bool hasMipmaps)
	: Width(width)
	, Height(height)
	, ArraySize(arraySize)
	, SampleCount(sampleCount)
	, Target(target)
	, Format(format)
	, HasMipmaps(hasMipmaps)
{
}

bool Texture2DDescription::IsCubemap() const
{
	return (Target == TextureTarget::TextureCubemap || Target == TextureTarget::TextureCubemapArray);
}

bool Texture2DDescription::IsArray() const
{
	return (Target == TextureTarget::Texture2DArray || Target == TextureTarget::Texture2DMSArray
		|| Target == TextureTarget::TextureCubemapArray);
}

bool Texture2DDescription::IsMultisampled() const
{
	return (Target == TextureTarget::Texture2DMS || Target == TextureTarget::Texture2DMSArray);
}

unsigned Texture2DDescription::GetCountMipmaps() const
{
	return (HasMipmaps ? EngineBuildingBlocks::Graphics::GetMipmapLevelCount(Width, Height) : 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

Texture2D::Texture2D()
{
}

Texture2D::Texture2D(const Texture2DDescription& textureDesc, const TextureSamplingDescription& samplingDesc)
{
	Initialize(textureDesc, samplingDesc);
}

void Texture2D::Initialize(const Texture2DDescription& textureDesc, const TextureSamplingDescription& samplingDesc)
{
	assert(textureDesc.SampleCount == 1 || !textureDesc.HasMipmaps);

	m_TextureDescription = textureDesc;
	m_SamplingDescription = samplingDesc;
	m_Texture.Initialize();

	Bind();
	SetTextureSampler(textureDesc.Target, samplingDesc);

	auto target = (GLenum)textureDesc.Target;
	auto format = (GLint)textureDesc.Format;
	auto width = textureDesc.Width;
	auto height = textureDesc.Height;
	auto arraySize = textureDesc.ArraySize;
	auto countLevels = textureDesc.GetCountMipmaps();
	auto countSamples = textureDesc.SampleCount;
	switch (textureDesc.Target)
	{
	case TextureTarget::Texture2D:
	case TextureTarget::TextureCubemap: glTexStorage2D(target, countLevels, format, width, height); break;
	case TextureTarget::Texture2DArray: glTexStorage3D(target, countLevels, format, width, height, arraySize); break;
	case TextureTarget::TextureCubemapArray: glTexStorage3D(target, countLevels, format, width, height, arraySize * 6); break;
	case TextureTarget::Texture2DMS: glTexStorage2DMultisample(target, countSamples, format, width, height, GL_TRUE); break;
	case TextureTarget::Texture2DMSArray: glTexStorage3DMultisample(target, countSamples, format, width, height, arraySize, GL_TRUE); break;
	default: break;
	}

	Unbind();
}

void Texture2D::Delete()
{
	m_Texture.Delete();
}

GLuint Texture2D::GetHandle() const
{
	return m_Texture.GetHandle();
}

const Texture2DDescription& Texture2D::GetTextureDescription() const
{
	return m_TextureDescription;
}

const TextureSamplingDescription& Texture2D::GetSamplingDescription() const
{
	return m_SamplingDescription;
}

void Texture2D::Bind()
{
	glBindTexture((GLenum)m_TextureDescription.Target, m_Texture.GetHandle());
}

void Texture2D::Unbind()
{
	glBindTexture((GLenum)m_TextureDescription.Target, 0);
}

void Texture2D::Bind(unsigned unitIndex)
{
	glActiveTexture(GL_TEXTURE0 + unitIndex);
	Bind();
}

void Texture2D::Unbind(unsigned unitIndex)
{
	glActiveTexture(GL_TEXTURE0 + unitIndex);
	Unbind();
}

void Texture2D::SetData(const void* pData, PixelDataFormat pixelDataFormat, PixelDataType pixelDataType)
{
	unsigned start[] = { 0, 0, 0, 0 };
	unsigned end[] = { m_TextureDescription.Width, m_TextureDescription.Height,
		(m_TextureDescription.Target == TextureTarget::TextureCubemap ? 6 : m_TextureDescription.ArraySize), 6 };
	SetData(pData, pixelDataFormat, pixelDataType, start, end, 0);
}

void Texture2D::SetData(const void* pData, PixelDataFormat pixelDataFormat, PixelDataType pixelDataType,
	const unsigned* pOffset, const unsigned* pSize, unsigned mipmapLevel)
{
	assert(pData != nullptr);
	assert(m_TextureDescription.Target != TextureTarget::Texture2DMS
		&& m_TextureDescription.Target != TextureTarget::Texture2DMSArray);
	auto dataFormat = (GLenum)pixelDataFormat;
	auto dataType = (GLenum)pixelDataType;
	switch (m_TextureDescription.Target)
	{
	case TextureTarget::Texture2D:
		glTexSubImage2D(GL_TEXTURE_2D, mipmapLevel, pOffset[0], pOffset[1], pSize[0], pSize[1], dataFormat, dataType, pData); break;
	case TextureTarget::Texture2DArray:
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mipmapLevel, pOffset[0], pOffset[1], pOffset[2], pSize[0], pSize[1], pSize[2],
			dataFormat, dataType, pData); break;
	case TextureTarget::TextureCubemap:
	{
		unsigned writeSize = pSize[0] * pSize[1] * GetPixelDataSizeInBytes(pixelDataFormat, pixelDataType);
		auto ptr = reinterpret_cast<const unsigned char*>(pData);
		auto faceIndexLimit = pOffset[2] + pSize[2];
		for (unsigned faceIndex = pOffset[2]; faceIndex < faceIndexLimit; ++faceIndex, ptr += writeSize)
		{
			glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, mipmapLevel, pOffset[0], pOffset[1],
				pSize[0], pSize[1], dataFormat, dataType, ptr);
		}
		break;
	}
	case TextureTarget::TextureCubemapArray:
	{
		unsigned writeSize = pSize[0] * pSize[1] * GetPixelDataSizeInBytes(pixelDataFormat, pixelDataType);
		auto ptr = reinterpret_cast<const unsigned char*>(pData);
		auto faceIndexLimit = pOffset[3] + pSize[3];
		for (unsigned faceIndex = pOffset[3]; faceIndex < faceIndexLimit; ++faceIndex, ptr += writeSize)
		{
			glTexSubImage3D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, mipmapLevel, pOffset[0], pOffset[1],
				pOffset[2], pSize[0], pSize[1], pSize[2], dataFormat, dataType, ptr);
		}
		break;
	}
	default: break;
	}
}

void Texture2D::GenerateMipmaps()
{
	assert(m_TextureDescription.HasMipmaps == true);
	glGenerateMipmap((GLenum)m_TextureDescription.Target);
}