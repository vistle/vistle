// Message.h

#ifndef _CUBEMAPSTREAMING_MESSAGE_H_INCLUDED_
#define _CUBEMAPSTREAMING_MESSAGE_H_INCLUDED_

#include <Core/SimpleBinarySerialization.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>
#include <EngineBuildingBlocks/Graphics/Camera/CameraProjection.h>

namespace CubemapStreaming
{
	enum class ServerMessageType : unsigned
	{
		CubemapUpdate
	};

	struct Server_CubemapUpdateData
	{
		unsigned TextureWidth, TextureHeight;
		bool IsContainingAlpha;
		float SideVisiblePortion; // Side 0, 1, 2, 3.
		float PosZVisiblePortion; // Side 4.
		glm::mat3 CameraOrientation;
		glm::vec3 CameraPosition;
		EngineBuildingBlocks::Graphics::CameraProjection CameraProjection;

		unsigned GetImageDataSize() const;
	};

	void GetImageDataSideBounds(unsigned width, unsigned height,
		float sideVisiblePortion, float posZVisiblePortion,
		glm::uvec2* start, glm::uvec2* end);

	unsigned GetImageDataSize(unsigned width, unsigned height,
		bool isContainingAlpha,
		float sideVisiblePortion, float posZVisiblePortion);

	void GetImageDataOffsets(unsigned width, unsigned height,
		bool isContainingAlpha,
		float sideVisiblePortion, float posZVisiblePortion,
		unsigned* colorOffsets, unsigned* depthOffsets);

	void ValidateDimensions(unsigned width, unsigned height,
		float sideVisiblePortion, float posZVisiblePortion);

	struct ServerMessage
	{
		ServerMessageType Type;

		union MessageData
		{
			Server_CubemapUpdateData CubemapUpdate;

			MessageData() {}
		} Data;
	};

	enum class ClientMessageType : unsigned
	{
		CameraUpdate, DataReadReady
	};

	struct Client_CameraUpdate_MessageData
	{
		glm::mat3 CameraOrientation;
		glm::vec3 CameraPosition;
	};

	struct ClientMessage
	{
		ClientMessageType Type;

		union MessageData
		{
			Client_CameraUpdate_MessageData CameraUpdate;

			MessageData() {}
		} Data;
	};
}

#endif