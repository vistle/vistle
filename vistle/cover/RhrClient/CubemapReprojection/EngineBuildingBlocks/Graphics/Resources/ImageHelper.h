// EngineBuildingBlocks/Graphics/Resources/ImageHelper.h

#ifndef _ENGINEBUILDINGBLOCKS_IMAGEHELPER_H_INCLUDED_
#define _ENGINEBUILDINGBLOCKS_IMAGEHELPER_H_INCLUDED_

#include <Core/DataStructures/SimpleTypeVector.hpp>
#include <Core/Enum.h>

#include <string>
#include <vector>

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		enum class ImagePixelType
		{
			UnsignedByte, Float
		};

		struct ChannelSource
		{
			unsigned Source;
			unsigned Channel;
		};

		struct Image2DLoadRequestDescription
		{
			std::vector<std::string> FileNames;
			Core::SimpleTypeVectorU<ChannelSource> Sources;
		};

		inline Core::SimpleTypeVectorU<ChannelSource> GetChannelSourceForDiffuseAndOpacityMixture(unsigned opacityChannelIndex)
		{
			return { { 0, 0 },{ 0, 1 },{ 0, 2 },{ 1, opacityChannelIndex } };
		}

		struct Image2DDescription
		{
			unsigned Width;
			unsigned Height;
			unsigned ChannelCount;
			unsigned PixelByteCount;
			unsigned ArraySize;
			ImagePixelType Type;
			bool IsCubeMap;

			unsigned GetSizeInBytes() const;

			static Image2DDescription ColorImage(unsigned width, unsigned height, unsigned channelCount = 4);
		};

		enum class ImageSaveFlags : unsigned
		{
			None = 0x00,
			IsConverting_RGB_BGR = 0x01,
			IsFlippingY = 0x02
		};
		UseEnumAsFlagSet(ImageSaveFlags);

		class FormatSupportHandler
		{
		public:

			virtual void HandleFormatSupport(Image2DDescription& description) const {}
		};

		struct ImageRawData
		{
			unsigned Width, Height;
			std::string Extension;
			Core::ByteVectorU Data;

			void SerializeSB(Core::ByteVector& bytes) const;
			void DeserializeSB(const unsigned char*& bytes);
		};

		void LoadImageFromMemory(
			std::vector<Core::ByteVectorU>& data,
			Image2DDescription& description,
			const ImageRawData& imageRawData,
			const FormatSupportHandler& formatSupportHandler = FormatSupportHandler());

		void LoadImageFromFile(
			std::vector<Core::ByteVectorU>& data,
			Image2DDescription& description,
			const std::string& fileName,
			const FormatSupportHandler& formatSupportHandler = FormatSupportHandler());

		void LoadImageFromFile(
			std::vector<Core::ByteVectorU>& data,
			Image2DDescription& description,
			const Image2DLoadRequestDescription& loadRequestDescription,
			const FormatSupportHandler& formatSupportHandler = FormatSupportHandler());

		void SaveImageToFile(const unsigned char* data,
			const Image2DDescription& imageDescription,
			const std::string& fileName,
			ImageSaveFlags saveFlags = ImageSaveFlags::None);

		void DownsampleLinearUnormWithBoxFilterPow2(
			const unsigned char* sourceBuffer, unsigned sourceWidth, unsigned sourceHeight,
			unsigned char* targetBuffer, unsigned channelCount);
	}
}

#endif