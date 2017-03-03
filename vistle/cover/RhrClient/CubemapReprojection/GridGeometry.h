// CubemapStreaming_Client/GridGeometry.h

#ifndef _CUBEMAPSTREAMING_CLIENT_GRIDGEOMETRY_H_
#define _CUBEMAPSTREAMING_CLIENT_GRIDGEOMETRY_H_

#include <EngineBuildingBlocks/Graphics/Primitives/Primitive.h>

void CreateFullReprojectionGrid_TexCoord(unsigned count,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData);

void CreatePartialReprojectionGrid_TexCoord(unsigned count,
	unsigned countTilesX, unsigned countTilesY,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData);

void CreateReproectionGridEdge_TexCoord_FaceIndex(unsigned count,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData);

#endif