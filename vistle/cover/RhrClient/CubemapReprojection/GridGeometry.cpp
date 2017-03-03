// CubemapStreaming_Client/GridGeometry.cpp

#include <GridGeometry.h>

using namespace EngineBuildingBlocks::Graphics;

inline void SetGridStripVertices(Core::SimpleTypeVectorU<glm::vec2>& textureCoordinates,
	unsigned count, unsigned countInTileX, unsigned countInTileY)
{
	textureCoordinates.Resize(countInTileX * countInTileY);

	int index = -1;
	for (unsigned i = 0; i < countInTileY; i++)
	{
		float y = ((float)i + 0.5f) / (float)count;
		for (unsigned j = 0; j < countInTileX; j++)
		{
			float x = ((float)j + 0.5f) / (float)count;
			textureCoordinates[++index] = glm::vec2(x, y);
		}
	}
}

inline void SetGridStripIndices(Core::IndexVectorU& indices, unsigned countX, unsigned countY)
{
	indices.Resize((countY - 1) * (countX * 2 + 2));

	int index = 0;
	for (unsigned i = 0; i < countY - 1; i++)
	{
		for (unsigned j = 0; j < countX; j++)
		{
			indices[index++] = (i + 1) * countX + j;
			indices[index++] = i * countX + j;
		}
		indices[index++] = (i + 1) * countX - 1;
		indices[index++] = (i + 2) * countX;
	}
}

inline unsigned GetCubemapGridSideConnectionIndex(unsigned side, unsigned type, unsigned typeIndex, unsigned count)
{
	return (side * 4 + type) * count + typeIndex;
}

inline void SetCubemapGridSideConnectionSideIndex(unsigned* indices, unsigned count, int& index,
	unsigned side1, unsigned type1, unsigned side2, unsigned type2, unsigned reverse)
{
	bool rev1 = ((reverse & 0x01) != 0);
	bool rev2 = ((reverse & 0x02) != 0);
	indices[index++] = GetCubemapGridSideConnectionIndex(side1, type1, (rev1 ? (count - 1) : 0), count);
	for (unsigned i = 0; i < count; i++)
	{
		indices[index++] = GetCubemapGridSideConnectionIndex(side1, type1, (rev1 ? (count - i - 1) : i), count);
		indices[index++] = GetCubemapGridSideConnectionIndex(side2, type2, (rev2 ? (count - i - 1) : i), count);
	}
	indices[index++] = GetCubemapGridSideConnectionIndex(side2, type2, (rev2 ? 0 : (count - 1)), count);
}

inline void SetCubemapGridSideConnectionEdgeIndex(unsigned* indices, unsigned count, int& index,
	unsigned side1, unsigned type1, unsigned side2, unsigned type2, unsigned side3, unsigned type3, unsigned reverse)
{
	bool rev1 = ((reverse & 0x01) != 0);
	bool rev2 = ((reverse & 0x02) != 0);
	bool rev3 = ((reverse & 0x04) != 0);
	indices[index++] = GetCubemapGridSideConnectionIndex(side1, type1, (rev1 ? (count - 1) : 0), count);
	indices[index++] = GetCubemapGridSideConnectionIndex(side1, type1, (rev1 ? (count - 1) : 0), count);
	indices[index++] = GetCubemapGridSideConnectionIndex(side2, type2, (rev2 ? (count - 1) : 0), count);
	indices[index++] = GetCubemapGridSideConnectionIndex(side3, type3, (rev3 ? (count - 1) : 0), count);
	indices[index++] = GetCubemapGridSideConnectionIndex(side3, type3, (rev3 ? (count - 1) : 0), count);
}

void CreateFullReprojectionGrid_TexCoord(unsigned count,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData)
{
	assert(count > 0);

	Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
	SetGridStripVertices(textureCoordinates, count, count, count);

	SetGridStripIndices(indexData.Data, count, count);

	vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
	indexData.Topology = PrimitiveTopology::TriangleStrip;
}

void CreatePartialReprojectionGrid_TexCoord(unsigned count, unsigned countTilesX, unsigned countTilesY,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData)
{
	assert(count > 0 && count % countTilesX == 0 && count % countTilesY == 0);

	unsigned countVerticesInTileX = count / countTilesX + 1;
	unsigned countVerticesInTileY = count / countTilesY + 1;

	Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
	SetGridStripVertices(textureCoordinates, count, countVerticesInTileX, countVerticesInTileY);

	SetGridStripIndices(indexData.Data, countVerticesInTileX, countVerticesInTileY);

	vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
	indexData.Topology = PrimitiveTopology::TriangleStrip;
}

void CreateReproectionGridEdge_TexCoord_FaceIndex(unsigned count,
	EngineBuildingBlocks::Graphics::Vertex_SOA_Data& vertexData,
	EngineBuildingBlocks::Graphics::IndexData& indexData)
{
	assert(count > 0);

	Core::SimpleTypeVectorU<glm::vec2> textureCoordinates;
	Core::IndexVectorU vertexIndices;

	auto& indices = indexData.Data;

	// Setting vertices.
	{
		textureCoordinates.Resize(count * 24);

		for (unsigned i = 0; i < 6; i++)
		{
			vertexIndices.PushBack(i, count * 4);
		}

		float lowerTC = 0.5f / (float)count;
		float upperTC = ((float)count - 0.5f) / (float)count;

		int index = -1;
		for (unsigned side = 0; side < 6; side++)
		{
			for (unsigned i = 0; i < count; i++) textureCoordinates[++index] = glm::vec2(lowerTC, ((float)i + 0.5f) / (float)count);
			for (unsigned i = 0; i < count; i++) textureCoordinates[++index] = glm::vec2(upperTC, ((float)i + 0.5f) / (float)count);
			for (unsigned i = 0; i < count; i++) textureCoordinates[++index] = glm::vec2(((float)i + 0.5f) / (float)count, lowerTC);
			for (unsigned i = 0; i < count; i++) textureCoordinates[++index] = glm::vec2(((float)i + 0.5f) / (float)count, upperTC);
		}
	}

	// Setting indices.
	{
		indices.Resize((count * 2 + 2) * 12 + 40);
		int index = 0;

		// Sides.
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 0, 0, 5, 1, 0);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 0, 1, 4, 0, 3);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 0, 2, 3, 0, 3);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 0, 3, 2, 0, 2);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 1, 0, 4, 1, 0);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 1, 1, 5, 0, 3);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 1, 2, 3, 1, 1);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 1, 3, 2, 1, 0);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 2, 2, 4, 3, 3);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 2, 3, 5, 3, 2);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 3, 2, 5, 2, 1);
		SetCubemapGridSideConnectionSideIndex(indices.GetArray(), count, index, 3, 3, 4, 2, 0);

		// Corners.
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 0, 0, 3, 0, 5, 1, 0);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 0, 1, 3, 0, 4, 0, 2);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 0, 0, 5, 1, 2, 0, 7);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 0, 1, 4, 0, 2, 0, 3);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 1, 0, 3, 1, 4, 1, 2);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 1, 1, 3, 1, 5, 0, 0);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 1, 0, 4, 1, 2, 1, 3);
		SetCubemapGridSideConnectionEdgeIndex(indices.GetArray(), count, index, 1, 1, 5, 0, 2, 1, 7);
	}

	vertexData.AddTextureCoordinateVertexElement(textureCoordinates.GetArray(), textureCoordinates.GetSize());
	vertexData.AddVertexIndexVertexElement(vertexIndices.GetArray(), vertexIndices.GetSize());
	indexData.Topology = PrimitiveTopology::TriangleStrip;
}