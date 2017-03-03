// OpenGLRender/Resources/TextureEnums.h

#ifndef _OPENGLRENDER_TEXTUREENUMS_H_INCLUDED_
#define _OPENGLRENDER_TEXTUREENUMS_H_INCLUDED_

#include <OpenGLRender/OpenGL.h>

#include <cassert>

namespace OpenGLRender
{
	enum class TextureTarget
	{
		Texture1D = GL_TEXTURE_1D,
		Texture1DArray = GL_TEXTURE_1D_ARRAY,
		Texture2D = GL_TEXTURE_2D,
		Texture2DArray = GL_TEXTURE_2D_ARRAY,
		Texture2DMS = GL_TEXTURE_2D_MULTISAMPLE,
		Texture2DMSArray = GL_TEXTURE_2D_MULTISAMPLE_ARRAY,
		Texture3D = GL_TEXTURE_3D,
		TextureCubemap = GL_TEXTURE_CUBE_MAP,
		TextureCubemapArray = GL_TEXTURE_CUBE_MAP_ARRAY,
		TextureRectangle = GL_TEXTURE_RECTANGLE
	};

	// The texture's storage format, often referenced as "internal format" in the OpenGL documentation.
	// We exclude base formats to force the usage of sized and compressed formats.
	enum class TextureFormat
	{
		Unknown = 0,

		// Sized formats.

		R8 = GL_R8,
		RG8 = GL_RG8,
		RGB8 = GL_RGB8,
		RGBA8 = GL_RGBA8,
		R16 = GL_R16,
		RG16 = GL_RG16,
		RGB16 = GL_RGB16,
		RGBA16 = GL_RGBA16,

		R8_SNORM = GL_R8_SNORM,
		RG8_SNORM = GL_RG8_SNORM,
		RGB8_SNORM = GL_RGB8_SNORM,
		RGBA8_SNORM = GL_RGBA8_SNORM,
		R16_SNORM = GL_R16_SNORM,
		RG16_SNORM = GL_RG16_SNORM,
		RGB16_SNORM = GL_RGB16_SNORM,
		RGBA16_SNORM = GL_RGBA16_SNORM,

		R3G3B2 = GL_R3_G3_B2,
		RGB4 = GL_RGB4,
		RGB5 = GL_RGB5,
		RGB10 = GL_RGB10,
		RGB12 = GL_RGB12,
		RGBA2 = GL_RGBA2,
		RGBA4 = GL_RGBA4,
		RGB5A1 = GL_RGB5_A1,
		RGB10A2 = GL_RGB10_A2,
		RGBA12 = GL_RGBA12,
		RGB9E5 = GL_RGB9_E5,

		SRGB8 = GL_SRGB8,
		SRGB8ALPHA8 = GL_SRGB8_ALPHA8,

		R16F = GL_R16F,
		R32F = GL_R32F,
		RG16F = GL_RG16F,
		RG32F = GL_RG32F,
		RGB16F = GL_RGB16F,
		RGB32F = GL_RGB32F,
		RGBA16F = GL_RGBA16F,
		RGBA32F = GL_RGBA32F,

		R11G11B10F = GL_R11F_G11F_B10F,

		R8I = GL_R8I,
		R8UI = GL_R8UI,
		R16I = GL_R16I,
		R16UI = GL_R16UI,
		R32I = GL_R32I,
		R32UI = GL_R32UI,
		RG8I = GL_RG8I,
		RG8UI = GL_RG8UI,
		RG16I = GL_RG16I,
		RG16UI = GL_RG16UI,
		RG32I = GL_RG32I,
		RG32UI = GL_RG32UI,
		RGB8I = GL_RGB8I,
		RGB8UI = GL_RGB8UI,
		RGB16I = GL_RGB16I,
		RGB16UI = GL_RGB16UI,
		RGB32I = GL_RGB32I,
		RGB32UI = GL_RGB32UI,
		RGBA8I = GL_RGBA8I,
		RGBA8UI = GL_RGBA8UI,
		RGBA16I = GL_RGBA16I,
		RGBA16UI = GL_RGBA16UI,
		RGBA32I = GL_RGBA32I,
		RGBA32UI = GL_RGBA32UI,

		RGB10A2UI = GL_RGB10_A2UI,

		DepthComponent16 = GL_DEPTH_COMPONENT16,
		DepthComponent24 = GL_DEPTH_COMPONENT24,
		DepthComponent32 = GL_DEPTH_COMPONENT32,
		DepthComponent32F = GL_DEPTH_COMPONENT32F,

		Depth24Stencil8 = GL_DEPTH24_STENCIL8,
		Depth32FStencil8 = GL_DEPTH32F_STENCIL8,
		StencilIndex8 = GL_STENCIL_INDEX8,

		// Compressed formats.

		CompressedRed = GL_COMPRESSED_RED,
		CompressedRG = GL_COMPRESSED_RG,
		CompressedRGB = GL_COMPRESSED_RGB,
		CompressedRGBA = GL_COMPRESSED_RGBA,
		CompressedSRGB = GL_COMPRESSED_SRGB,
		CompressedSRGBAlpha = GL_COMPRESSED_SRGB_ALPHA,
		CompressedRedRGTC1 = GL_COMPRESSED_RED_RGTC1,
		CompressedSignedRedRGTC1 = GL_COMPRESSED_SIGNED_RED_RGTC1,
		CompressedRG_RGTC2 = GL_COMPRESSED_RG_RGTC2,
		CompressedSignedRG_RGTC2 = GL_COMPRESSED_SIGNED_RG_RGTC2,
		CompressedRGBA_BPTC_Unorm = GL_COMPRESSED_RGBA_BPTC_UNORM,
		CompressedSRGBAlpha_BPTC_Unorm = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
		CompressedRGB_BPTC_SignedFloat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
		CompressedRGB_BPTC_UnsignedFloat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
	};

	enum class TextureMinifyingFilter
	{
		Nearest = GL_NEAREST,
		Linear = GL_LINEAR,
		Nearest_Mipmap_Nearest = GL_NEAREST_MIPMAP_NEAREST,
		Linear_Mipmap_Nearest = GL_LINEAR_MIPMAP_NEAREST,
		Nearest_Mipmap_Linear = GL_NEAREST_MIPMAP_LINEAR,
		Linear_Mipmap_Linear = GL_LINEAR_MIPMAP_LINEAR,
		_Default = Nearest_Mipmap_Linear
	};

	enum class TextureMagnificationFilter
	{
		Nearest = GL_NEAREST,
		Linear = GL_LINEAR,
		_Default = Linear
	};

	enum class TextureWrapMode
	{
		ClampToEdge = GL_CLAMP_TO_EDGE,
		ClampToBorder = GL_CLAMP_TO_BORDER,
		MirroredRepeat = GL_MIRRORED_REPEAT,
		Repeat = GL_REPEAT,
		MirroredClampToEdge = GL_MIRROR_CLAMP_TO_EDGE,
		_Default = Repeat
	};

	enum class TextureComparisonMode
	{
		None = GL_NONE,
		CompareRefToTexture = GL_COMPARE_REF_TO_TEXTURE
	};

	enum class TextureComparisonFunction
	{
		Less = GL_LESS,
		LessEqual = GL_LEQUAL,
		GreaterEqual = GL_GEQUAL,
		Greater = GL_GREATER,

		Equal = GL_EQUAL,
		NotEqual = GL_NOTEQUAL,

		Always = GL_ALWAYS,
		Never = GL_NEVER
	};

	enum class PixelDataFormat
	{
		Red = GL_RED,
		RG = GL_RG,
		RGB = GL_RGB,
		BGR = GL_BGR,
		RGBA = GL_RGBA,
		BGRA = GL_BGRA,
		DepthComponent = GL_DEPTH_COMPONENT,
		StencilIndex = GL_STENCIL_INDEX
	};

	enum class PixelDataType
	{
		Int8 = GL_BYTE,
		Uint8 = GL_UNSIGNED_BYTE,
		Int16 = GL_SHORT,
		Uint16 = GL_UNSIGNED_SHORT,
		Int32 = GL_INT,
		Uint32 = GL_UNSIGNED_INT,
		Float = GL_FLOAT,
		Uint_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
		Uint_2_3_3_Rev = GL_UNSIGNED_BYTE_2_3_3_REV,
		Uint_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
		Uint_5_6_5_Rev = GL_UNSIGNED_SHORT_5_6_5_REV,
		Uint_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
		Uint_4_4_4_4_Rev = GL_UNSIGNED_SHORT_4_4_4_4_REV,
		Uint_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
		Uint_1_5_5_5_Rev = GL_UNSIGNED_SHORT_1_5_5_5_REV,
		Uint_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
		Uint_8_8_8_8_Rev = GL_UNSIGNED_INT_8_8_8_8_REV,
		Uint_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
		Uint_10_10_10_2_Rev = GL_UNSIGNED_INT_2_10_10_10_REV
	};

	inline unsigned GetPixelDataSizeInBits(PixelDataFormat format, PixelDataType type)
	{
		unsigned channelSize, countChannels;
		switch (type)
		{
		case PixelDataType::Int8:
		case PixelDataType::Uint8: channelSize = 8; break;
		case PixelDataType::Int16:
		case PixelDataType::Uint16: channelSize = 16; break;
		case PixelDataType::Int32:
		case PixelDataType::Uint32:
		case PixelDataType::Float: channelSize = 32; break;
		case PixelDataType::Uint_3_3_2:
		case PixelDataType::Uint_2_3_3_Rev: return 8;
		case PixelDataType::Uint_5_6_5:
		case PixelDataType::Uint_5_6_5_Rev:
		case PixelDataType::Uint_4_4_4_4:
		case PixelDataType::Uint_4_4_4_4_Rev:
		case PixelDataType::Uint_5_5_5_1:
		case PixelDataType::Uint_1_5_5_5_Rev: return 16;
		case PixelDataType::Uint_8_8_8_8:
		case PixelDataType::Uint_8_8_8_8_Rev:
		case PixelDataType::Uint_10_10_10_2:
		case PixelDataType::Uint_10_10_10_2_Rev: return 32;
		}
		switch (format)
		{
		case PixelDataFormat::Red: 
		case PixelDataFormat::DepthComponent:
		case PixelDataFormat::StencilIndex: countChannels = 1; break;
		case PixelDataFormat::RG: countChannels = 2; break;
		case PixelDataFormat::RGB:
		case PixelDataFormat::BGR: countChannels = 3; break;
		case PixelDataFormat::RGBA:
		case PixelDataFormat::BGRA: countChannels = 4; break;
		}
		return channelSize * countChannels;
	}

	inline unsigned GetPixelDataSizeInBytes(PixelDataFormat format, PixelDataType type)
	{
		assert((GetPixelDataSizeInBits(format, type) & 7) == 0);
		return GetPixelDataSizeInBits(format, type) >> 3;
	}

	enum class TextureReadFormat
	{
		Unknown = 0,

		Red = GL_RED,
		Green = GL_GREEN,
		Blue = GL_BLUE,
		RGB = GL_RGB,
		BGR = GL_BGR,
		RGBA = GL_RGBA,
		BGRA = GL_BGRA,
		DepthComponent = GL_DEPTH_COMPONENT,
		DepthStencil = GL_DEPTH_STENCIL,
		StencilIndex = GL_STENCIL_INDEX
	};
}

#endif