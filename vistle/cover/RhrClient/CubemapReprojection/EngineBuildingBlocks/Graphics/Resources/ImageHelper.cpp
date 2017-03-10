// EngineBuildingBlocks/Graphics/ImageHelper.cpp

#include <EngineBuildingBlocks/Graphics/Resources/ImageHelper.h>

#include <Core/String.hpp>
#include <Core/Platform.h>
#include <Core/Constants.h>
#include <Core/Utility.hpp>
#include <Core/System/Filesystem.h>
#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/ErrorHandling.h>

#include <External/FreeImage-3.15.4/FreeImage.h>

#define DDS_IMPLEMENTATION
#include <External/dds/dds.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <External/stb/stb_image_resize.h>

#include <sstream>
#include <cassert>

using namespace EngineBuildingBlocks;
using namespace EngineBuildingBlocks::Graphics;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

FREE_IMAGE_FORMAT GetFreeImageFormat(const std::string& fileName)
{
	auto extension = fileName.substr(fileName.find_last_of('.') + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == "bmp") return FIF_BMP;
	if (extension == "dds") return FIF_DDS;
	if (extension == "jpeg" || extension == "jpg") return FIF_JPEG;
	if (extension == "png") return FIF_PNG;
	if (extension == "tiff") return FIF_TIFF;

	EngineBuildingBlocks::RaiseException("Unsupported image format was requested.");

	return FIF_UNKNOWN;
}

inline void Convert_RGB_BGR(unsigned width, unsigned height, unsigned channelCount, unsigned pixelByteCount,
	unsigned char* pData)
{
	assert(channelCount >= 3 && pixelByteCount == channelCount);
	for (auto pDataEnd = pData + width * height * pixelByteCount; pData != pDataEnd; pData += pixelByteCount)
	{
		std::swap(pData[0], pData[2]);
	}
}

inline void FlipY(unsigned width, unsigned height, unsigned pixelByteCount, unsigned char* pData)
{
	auto temp = new unsigned char[pixelByteCount];
	unsigned h2 = height >> 1;
	unsigned m = width * pixelByteCount;
	for (unsigned j = 0; j < h2; j++)
	{
		auto ptr1 = pData + j * m;
		auto ptr2 = pData + (height - j - 1) * m;
		for (auto endPtr1 = ptr1 + m; ptr1 != endPtr1; ptr1 += pixelByteCount, ptr2 += pixelByteCount)
		{
			memcpy(temp, ptr1, pixelByteCount);
			memcpy(ptr1, ptr2, pixelByteCount);
			memcpy(ptr2, temp, pixelByteCount);
		}
	}
	delete temp;
}

#define ThrowExceptionWithBitmapRelease(str) {			\
	FreeImage_Unload(pBitmap);							\
	EngineBuildingBlocks::RaiseException(str); }

inline void CopyData(const unsigned char* pInputData,
	unsigned inputElementSize, unsigned inputPitch,
	Core::ByteVectorU& data, Image2DDescription& description, int iFit)
{
	auto imageType = static_cast<FREE_IMAGE_TYPE>(iFit);

	unsigned width = description.Width;
	unsigned height = description.Height;

	unsigned pixelByteCount = description.PixelByteCount;
	unsigned imagePitch = pixelByteCount * width;
	data.Resize(width * height * pixelByteCount);

	// Setting channel offsets.
	unsigned red, green, blue, alpha;
	red = 2; green = 1; blue = 0; alpha = 3;

	// Copying data.
	if (imageType == FIT_BITMAP)
	{
		if (inputElementSize == 1)
		{
			for (unsigned y = 0; y < height; y++)
			{
				unsigned offset = y * imagePitch;
				unsigned offset_img = y * inputPitch;
				for (unsigned x = 0; x < width; x++)
				{
					data[offset] = pInputData[offset_img];
					offset += pixelByteCount;
					offset_img += inputElementSize;
				}
			}
		}
		else if (inputElementSize == 2)
		{
			for (unsigned y = 0; y < height; y++)
			{
				unsigned offset = y * imagePitch;
				unsigned offset_img = y * inputPitch;
				for (unsigned x = 0; x < width; x++)
				{
					data[offset] = pInputData[offset_img];
					data[offset + 1] = pInputData[offset_img + 1];

					offset += pixelByteCount;
					offset_img += inputElementSize;
				}
			}
		}
		else if (inputElementSize == 3)
		{
			for (unsigned y = 0; y < height; y++)
			{
				unsigned offset = y * imagePitch;
				unsigned offset_img = y * inputPitch;
				for (unsigned x = 0; x < width; x++)
				{
					data[offset + red] = pInputData[offset_img];
					data[offset + green] = pInputData[offset_img + 1];
					data[offset + blue] = pInputData[offset_img + 2];

					offset += pixelByteCount;
					offset_img += inputElementSize;
				}
			}
			if (pixelByteCount == 4) // We must fill the alpha channel.
			{
				for (unsigned y = 0; y < height; y++)
				{
					unsigned offset = y * imagePitch + alpha;
					for (unsigned x = 0; x < width; x++)
					{
						data[offset] = 255;

						offset += pixelByteCount;
					}
				}
			}
		}
		else if (inputElementSize == 4)
		{
			for (unsigned y = 0; y < height; y++)
			{
				unsigned offset = y * imagePitch;
				unsigned offset_img = y * inputPitch;
				for (unsigned x = 0; x < width; x++)
				{
					data[offset + red] = pInputData[offset_img];
					data[offset + green] = pInputData[offset_img + 1];
					data[offset + blue] = pInputData[offset_img + 2];
					data[offset + alpha] = pInputData[offset_img + 3];

					offset += pixelByteCount;
					offset_img += inputElementSize;
				}
			}
		}
	}
	else if (imageType == FIT_FLOAT)
	{
		auto fImageData = reinterpret_cast<float*>(data.GetArray());
		auto fBitmapData = reinterpret_cast<const float*>(pInputData);
		for (unsigned int y = 0; y < height; y++)
		{
			unsigned offset = y * imagePitch;
			unsigned offset_img = y * inputPitch;
			for (unsigned x = 0; x < width; x++)
			{
				fImageData[offset] = fBitmapData[offset_img];

				offset += pixelByteCount;
				offset_img += inputElementSize;
			}
		}
	}
}

inline void LoadSimpleImage(Core::ByteVectorU& data, Image2DDescription& description, FIBITMAP* pBitmap,
	const FormatSupportHandler& formatSupportHandler)
{
	// Getting image type.
	FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(pBitmap);

	// Getting image properties.
	unsigned bpp = FreeImage_GetBPP(pBitmap);
	unsigned width = FreeImage_GetWidth(pBitmap);
	unsigned height = FreeImage_GetHeight(pBitmap);
	unsigned pitch = FreeImage_GetPitch(pBitmap);
	unsigned elementSize = bpp / 8;

	auto pBitmapData = FreeImage_GetBits(pBitmap);

	description.Width = width;
	description.Height = height;
	description.PixelByteCount = elementSize;
	description.ArraySize = 1;
	description.IsCubeMap = false;

	if (imageType == FIT_BITMAP)
	{
		description.Type = ImagePixelType::UnsignedByte;
		switch (bpp)
		{
		case 8:
		{
			description.ChannelCount = 1;
			break;
		}
		case 16:
		{
			description.ChannelCount = 2;
			break;
		}
		case 24:
		{
			description.ChannelCount = 3;
			break;
		}
		case 32:
		{
			description.ChannelCount = 4;
			break;
		}
		default: ThrowExceptionWithBitmapRelease("Unsupported BPP for bitmap.");
		}
	}
	else if (imageType == FIT_FLOAT)
	{
		if (bpp != 32)
		{
			ThrowExceptionWithBitmapRelease("Only 32 bit input float images are supported.");
		}
		description.Type = ImagePixelType::Float;
		description.ChannelCount = 1;
	}
	else
	{
		ThrowExceptionWithBitmapRelease("Image format is not supported.");
	}

	// Handling format.
	formatSupportHandler.HandleFormatSupport(description);

	// Copying data.
	CopyData(pBitmapData, elementSize, pitch, data, description, static_cast<int>(imageType));

	// Freeing bitmap.
	FreeImage_Unload(pBitmap);
}

inline void LoadSimpleImage(Core::ByteVectorU& data, Image2DDescription& description, const std::string& fileName,
	const FormatSupportHandler& formatSupportHandler, int iFif)
{
	auto fif = static_cast<FREE_IMAGE_FORMAT>(iFif);

	// Loading to bitmap.
	FIBITMAP* pBitmap = FreeImage_Load(fif, fileName.c_str(), 0);
	if (pBitmap == nullptr)
	{
		std::string message = "Cannot load image file to bitmap: ";
		EngineBuildingBlocks::RaiseException(message + fileName);
	}

	LoadSimpleImage(data, description, pBitmap, formatSupportHandler);
}

inline void LoadDDSImage(std::vector<Core::ByteVectorU>& data, Image2DDescription& description,
	const std::string& fileName, const FormatSupportHandler& formatSupportHandler)
{
	dds_info info;
	dds_additional_info additionalInfo;
	if (dds_load_from_file(fileName.c_str(), &info, &additionalInfo, nullptr) != DDS_SUCCESS)
	{
		std::stringstream ss;
		ss << "Failed to load DDS file: '" << fileName << "'.";
		RaiseException(ss);
	}

	description.Width = info.image.width;
	description.Height = info.image.height;
	description.Type = ImagePixelType::UnsignedByte;
	description.ChannelCount = additionalInfo.CountChannels;
	description.PixelByteCount = additionalInfo.BPP >> 3;
	auto sideSize = description.Width * description.Height * description.PixelByteCount;
	description.ArraySize = info.image.size / sideSize;
	description.IsCubeMap = ((info.flags & DDS_CUBEMAP_FULL) != 0);

	// Handling format.
	formatSupportHandler.HandleFormatSupport(description);

	Core::ByteVector buffer(sideSize);
	data.resize(description.ArraySize);
	auto rowPitch = description.Width * description.PixelByteCount;
	for (unsigned i = 0; i < description.ArraySize; i++)
	{
		auto sourceData = buffer.GetArray();
		info.side = i;
		dds_read(&info, sourceData);
		CopyData(sourceData, description.PixelByteCount, rowPitch, data[i], description,
			static_cast<int>(description.Type == ImagePixelType::UnsignedByte ? FIT_BITMAP : FIT_FLOAT));
	}

	dds_close(&info);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned Image2DDescription::GetSizeInBytes() const
{
	return PixelByteCount * Width * Height * ArraySize;
}

Image2DDescription Image2DDescription::ColorImage(unsigned width, unsigned height, unsigned channelCount)
{
	return{ width, height, channelCount, channelCount, 1, ImagePixelType::UnsignedByte, false };
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void ImageRawData::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Width);
	Core::SerializeSB(bytes, Height);
	Core::SerializeSB(bytes, Extension);
	Core::SerializeSB(bytes, Data);
}

void ImageRawData::DeserializeSB(const unsigned char*& bytes)
{
	Core::DeserializeSB(bytes, Width);
	Core::DeserializeSB(bytes, Height);
	Core::DeserializeSB(bytes, Extension);
	Core::DeserializeSB(bytes, Data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace EngineBuildingBlocks
{
	namespace Graphics
	{
		void LoadImageFromMemory(std::vector<Core::ByteVectorU>& data, Image2DDescription& description,
			const ImageRawData& imageRawData, const FormatSupportHandler& formatSupportHandler)
		{
			auto fif = GetFreeImageFormat(imageRawData.Extension);
			auto stream = FreeImage_OpenMemory((BYTE*)imageRawData.Data.GetArray(),
				imageRawData.Data.GetSize());
			auto pBitmap = FreeImage_LoadFromMemory(fif, stream, 0);
			if (pBitmap == nullptr)
			{
				EngineBuildingBlocks::RaiseException("Cannot load image from memory.");
			}

			if (data.size() == 0) data.resize(1);
			LoadSimpleImage(data[0], description, pBitmap, formatSupportHandler);
			FreeImage_CloseMemory(stream);
		}

		void LoadImageFromFile(std::vector<Core::ByteVectorU>& data, Image2DDescription& description,
			const std::string& fileName, const FormatSupportHandler& formatSupportHandler)
		{
			if (!Core::FileExists(fileName))
			{
				std::string message = "Image file doesn't exists: ";
				EngineBuildingBlocks::RaiseException(message + fileName);
			}

			FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

			// Checking the file signature and deducing its format (the second argument is currently not used by FreeImage).
			fif = FreeImage_GetFileType(fileName.c_str(), 0);
			if (fif == FIF_UNKNOWN)
			{
				// If there's no signature, trying to guess the file format from the file extension.
				fif = FreeImage_GetFIFFromFilename(fileName.c_str());
			}

			// Checking whether FreeImage can read the file.
			if (fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif))
			{
				std::string message = "Cannot load image file: ";
				EngineBuildingBlocks::RaiseException(message + fileName);
			}

			if (fif == FIF_DDS)
			{
				LoadDDSImage(data, description, fileName, formatSupportHandler);
			}
			else
			{
				if (data.size() == 0) data.resize(1);
				LoadSimpleImage(data[0], description, fileName, formatSupportHandler, static_cast<int>(fif));
			}
		}

		void LoadImageFromFile(std::vector<Core::ByteVectorU>& data, Image2DDescription& description,
			const Image2DLoadRequestDescription& loadRequestDescription,
			const FormatSupportHandler& formatSupportHandler)
		{
			auto& fileNames = loadRequestDescription.FileNames;
			auto& sources = loadRequestDescription.Sources;

			unsigned countChannels = sources.GetSize();
			unsigned countSources = static_cast<unsigned>(fileNames.size());

			assert(countSources > 0 && countChannels > 0);

			Core::SimpleTypeVectorU<Image2DDescription> descriptions(countSources);
			std::vector<std::vector<Core::ByteVectorU>> datas(countSources);

			// Getting source usage.
			Core::SimpleTypeVectorU<bool> isSourceUsed(countSources, false);
			unsigned firstUsedSource = sources[0].Source;
			for (unsigned i = 0; i < countChannels; i++)
			{
				assert(sources[i].Source < countSources);

				isSourceUsed[sources[i].Source] = true;
			}

			// Loading sources.
			for (unsigned i = 0; i < countSources; i++)
			{
				if (isSourceUsed[i])
				{
					LoadImageFromFile(datas[i], descriptions[i], fileNames[i], formatSupportHandler);
				}
			}

			// Checking source compatibility.
			auto& firstSourceDescription = descriptions[firstUsedSource];
			for (unsigned i = 0; i < countSources; i++)
			{
				if (isSourceUsed[i])
				{
					auto& description = descriptions[i];

					if (description.ArraySize != firstSourceDescription.ArraySize
						|| description.Type != firstSourceDescription.Type
						|| description.IsCubeMap != firstSourceDescription.IsCubeMap)
					{
						RaiseException("Incompatible image sources in composition.");
					}
				}
			}

			description.Width = firstSourceDescription.Width;
			description.Height = firstSourceDescription.Height;
			description.ArraySize = firstSourceDescription.ArraySize;
			description.Type = firstSourceDescription.Type;
			description.IsCubeMap = firstSourceDescription.IsCubeMap;
			description.ChannelCount = countChannels;

			// Creating copy data.
			struct ImageCompositionCopyData
			{
				unsigned Source;
				unsigned Offset;
				unsigned Size;
			};
			Core::SimpleTypeVectorU<ImageCompositionCopyData> copyData;
			unsigned lastSource = sources[0].Source;
			unsigned lastChannel = sources[0].Channel;
			unsigned firstChannel = lastChannel;
			unsigned channelSize = firstSourceDescription.PixelByteCount / firstSourceDescription.ChannelCount;
			unsigned copySize = channelSize;

			description.PixelByteCount = channelSize;

			for (unsigned i = 1; i < countChannels; i++)
			{
				unsigned newSource = sources[i].Source;
				unsigned newChannel = sources[i].Channel;
				if (newSource != lastSource || newChannel != lastChannel + 1)
				{
					unsigned offset = firstChannel * channelSize;

					copyData.PushBack({ lastSource, offset, copySize });
					lastSource = newSource;
					firstChannel = newChannel;
					copySize = 0;

					auto& newDescription = descriptions[newSource];
					channelSize = newDescription.PixelByteCount / newDescription.ChannelCount;
				}

				lastChannel = newChannel;
				copySize += channelSize;
				description.PixelByteCount += channelSize;
			}
			copyData.PushBack({ lastSource, firstChannel * channelSize, copySize });

			// Resizing if necessary.
			{
				for (unsigned sourceIndex = 0; sourceIndex < countSources; sourceIndex++)
				{
					if (isSourceUsed[sourceIndex])
					{
						auto& sourceDescription = descriptions[sourceIndex];

						if (sourceDescription.Width != firstSourceDescription.Width
							|| sourceDescription.Height != firstSourceDescription.Height)
						{
							RaiseWarning("Image source have different sizes in composition, resizing is used.");

							if (description.PixelByteCount != 4 || description.ChannelCount != 4
								|| description.Type != ImagePixelType::UnsignedByte)
							{
								RaiseException("Image resizing is not implemented for this image pixel format.");
							}

							for (unsigned arrayIndex = 0; arrayIndex < description.ArraySize; arrayIndex++)
							{
								auto& targetVector = datas[sourceIndex][arrayIndex];
								targetVector.Resize(firstSourceDescription.Width
									* firstSourceDescription.Height * firstSourceDescription.PixelByteCount);

								stbir_datatype dataType;
								if (sourceDescription.Type == ImagePixelType::UnsignedByte)
								{
									switch (sourceDescription.PixelByteCount / sourceDescription.ChannelCount)
									{
									case 1: dataType = STBIR_TYPE_UINT8; break;
									case 2: dataType = STBIR_TYPE_UINT16; break;
									case 4: dataType = STBIR_TYPE_UINT32; break;
									default: EngineBuildingBlocks::RaiseException("Unknown data type."); break;
									}
								}
								else dataType = STBIR_TYPE_FLOAT;

								stbir_resize(
									datas[sourceIndex][arrayIndex].GetArray(),
									sourceDescription.Width, sourceDescription.Height, 0,
									targetVector.GetArray(),
									firstSourceDescription.Width, firstSourceDescription.Height, 0,
									dataType, sourceDescription.ChannelCount,
									0, 0, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP,
									STBIR_FILTER_BOX, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR,
									nullptr);
							}

							description.Width = firstSourceDescription.Width;
							description.Height = firstSourceDescription.Height;
						}
					}
				}
			}

			unsigned countPixels = description.Width * description.Height;
			unsigned targetIncrement = description.PixelByteCount;

			data.resize(description.ArraySize);
			for (unsigned arrayIndex = 0; arrayIndex < description.ArraySize; arrayIndex++)
			{
				auto& imageData = data[arrayIndex];
				imageData.Resize(description.Width * description.Height * description.PixelByteCount);

				unsigned targetOffset = 0;
				for (unsigned i = 0; i < copyData.GetSize(); i++)
				{
					auto sourceData = datas[copyData[i].Source][arrayIndex].GetArray() + copyData[i].Offset;
					unsigned sourceIncrement = descriptions[copyData[i].Source].PixelByteCount;
					auto targetData = imageData.GetArray() + targetOffset;
					unsigned copySize = copyData[i].Size;

					for (unsigned j = 0; j < countPixels; j++)
					{
						memcpy(targetData, sourceData, copySize);
						sourceData += sourceIncrement;
						targetData += targetIncrement;
					}

					targetOffset += copySize;
				}
			}
		}

		void SaveImageToFile(const unsigned char* data, const Image2DDescription& imageDescription,
			const std::string& fileName, ImageSaveFlags saveFlags)
		{
			assert(imageDescription.ArraySize == 1 && !imageDescription.IsCubeMap);

			unsigned width = imageDescription.Width;
			unsigned height = imageDescription.Height;
			unsigned channelCount = imageDescription.ChannelCount;
			unsigned pixelByteCount = imageDescription.PixelByteCount;

			FREE_IMAGE_TYPE type = (imageDescription.Type == ImagePixelType::UnsignedByte
				? FREE_IMAGE_TYPE::FIT_BITMAP
				: FREE_IMAGE_TYPE::FIT_FLOAT);

			auto bitmap = FreeImage_AllocateT(type, width, height, imageDescription.PixelByteCount * 8);
			auto bitmapData = FreeImage_GetBits(bitmap);

			// Copying data.
			unsigned size = width * height * pixelByteCount;
			memcpy(bitmapData, data, size);

			if (HasFlag(saveFlags, ImageSaveFlags::IsConverting_RGB_BGR))
				Convert_RGB_BGR(width, height, channelCount, pixelByteCount, bitmapData);
			if (HasFlag(saveFlags, ImageSaveFlags::IsFlippingY)) FlipY(width, height, pixelByteCount, bitmapData);

			// Getting free image format.
			FREE_IMAGE_FORMAT fif = GetFreeImageFormat(fileName);

			// Saving to file.
			if (!FreeImage_Save(fif, bitmap, fileName.c_str(), 0))
			{
				FreeImage_Unload(bitmap);

				std::stringstream ss;
				ss << "An error has been occured while saving a color image to: '" << fileName << "'";
				EngineBuildingBlocks::RaiseException(ss);
			}

			// Unloading bitmap.
			FreeImage_Unload(bitmap);
		}

		void DownsampleLinearUnormWithBoxFilterPow2(
			const unsigned char* sourceBuffer, unsigned sourceWidth, unsigned sourceHeight,
			unsigned char* targetBuffer, unsigned channelCount)
		{
			assert(sourceWidth > 1 && sourceHeight > 1);
			assert(Core::IsPowerOfTwo(sourceWidth) && Core::IsPowerOfTwo(sourceHeight));
			unsigned targetWidth = sourceWidth >> 1;
			unsigned targetHeight = sourceHeight >> 1;
			unsigned targetBase0 = 0;
			unsigned sourceBase0 = 0;
			unsigned sourceBase1 = sourceWidth;
			unsigned dSW = sourceWidth << 1;
			unsigned dChC = channelCount << 1;
			for (unsigned i = 0; i < targetHeight; i++)
			{
				unsigned targetBase1 = targetBase0 * channelCount;
				unsigned sourceBase00 = sourceBase0 * channelCount;
				unsigned sourceBase01 = sourceBase00 + channelCount;
				unsigned sourceBase10 = sourceBase1 * channelCount;
				unsigned sourceBase11 = sourceBase10 + channelCount;
				for (unsigned j = 0; j < targetWidth; j++)
				{
					for (unsigned k = 0; k < channelCount; k++)
					{
						unsigned sum = (unsigned)sourceBuffer[sourceBase00 + k]
							+ (unsigned)sourceBuffer[sourceBase01 + k]
							+ (unsigned)sourceBuffer[sourceBase10 + k]
							+ (unsigned)sourceBuffer[sourceBase11 + k];
						targetBuffer[targetBase1 + k] = (unsigned char)(sum >> 2);
					}
					targetBase1 += channelCount;
					sourceBase00 += dChC;
					sourceBase01 += dChC;
					sourceBase10 += dChC;
					sourceBase11 += dChC;
				}
				targetBase0 += targetWidth;
				sourceBase0 += dSW;
				sourceBase1 += dSW;
			}
		}
	}
}