#include "conduitTopology.h"
#include <cassert>

using namespace vistle;

conduit_int32 ConduitArray::operator[](size_t i) const
{
    assert(i < size);
    return data[i];
}

constexpr int numElementTypes = 9;
constexpr int elementTypesVistle[numElementTypes] = {
    UnstructuredGrid::Type::POINT,       UnstructuredGrid::Type::TRIANGLE,   UnstructuredGrid::Type::QUAD,
    UnstructuredGrid::Type::TETRAHEDRON, UnstructuredGrid::Type::HEXAHEDRON, UnstructuredGrid::Type::PYRAMID,
    UnstructuredGrid::Type::PRISM,       UnstructuredGrid::Type::POLYGON,    UnstructuredGrid::Type::POLYHEDRON};
const char *elementTypeStrings[numElementTypes] = {"point",   "tri",   "quad",      "tet",       "hex",
                                                   "pyramid", "wedge", "polygonal", "polyhedral"};

UnstructuredGrid::Type toVistleType(const std::string &elementType)
{
    for (size_t i = 0; i < numElementTypes; i++) {
        if (elementType == elementTypeStrings[i]) {
            return static_cast<UnstructuredGrid::Type>(elementTypesVistle[i]);
        }
    }
    return UnstructuredGrid::Type::NONE;
}

std::map<int, UnstructuredGrid::Type> getShapeMap(const conduit_cpp::Node &shapeMap)
{
    std::map<int, UnstructuredGrid::Type> vistleShapeMap;
    for (conduit_index_t i = 0; i < shapeMap.number_of_children(); i++) {
        auto shape = shapeMap.child(i);
        auto shapeName = shape.name();
        auto shapeCode = shape.as_int();
        for (int j = 0; j < numElementTypes; j++) {
            if (shapeName == elementTypeStrings[j]) {
                vistleShapeMap[shapeCode] = static_cast<UnstructuredGrid::Type>(elementTypesVistle[j]);
                break;
            }
        }
    }
    return vistleShapeMap;
}

std::map<int, UnstructuredGrid::Type> getShapeMapIfAvailable(const conduit_cpp::Node &node, const std::string &key)
{
    if (node.has_child(key))
        return getShapeMap(node[key]);
    return {};
}

std::string getConduitNodeStringIfAvailable(const conduit_cpp::Node &node, const std::string &key)
{
    if (node.has_child(key))
        return node[key].as_string();
    return "";
}

ConduitArray getConduitNodeInt32PtrIfAvailable(const conduit_cpp::Node &node, const std::string &key)
{
    if (node.has_child(key)) {
        // std::cerr << "Found " << key << " in Conduit node with " << node[key].number_of_elements() << " elements." << std::endl;
        auto n = node[key];
        return {n.as_int32_ptr(), n.number_of_elements()};
    }
    return {nullptr, 0};
}

bool isMixed(const conduit_cpp::Node &elementNode)
{
    auto elementTypeString = getConduitNodeStringIfAvailable(elementNode, "shape");
    return elementTypeString == "mixed";
}


ConduitTopology::ConduitTopology(const conduit_cpp::Node &mesh): ConduitTopology(mesh, mesh["elements"])
{}

ConduitTopology::ConduitTopology(const conduit_cpp::Node &mesh, const conduit_cpp::Node &meshElements)
: connectivity(getConduitNodeInt32PtrIfAvailable(meshElements, "connectivity"))
, shape(toVistleType(getConduitNodeStringIfAvailable(meshElements, "shape")))
, shapes(getConduitNodeInt32PtrIfAvailable(meshElements, "shapes"))
, shapeMap(getShapeMapIfAvailable(meshElements, "shape_map"))
, sizes(getConduitNodeInt32PtrIfAvailable(meshElements, "sizes"))
, offsets(getConduitNodeInt32PtrIfAvailable(meshElements, "offsets"))
, isMixed(::isMixed(meshElements))
, sub(getSubTopologyIfAvailable(mesh, "subelements"))
{
    assert(!isMixed && shapes);
}

std::unique_ptr<ConduitTopology> ConduitTopology::getSubTopologyIfAvailable(const conduit_cpp::Node &node,
                                                                            const std::string &key)
{
    if (node.has_child(key))
        return std::unique_ptr<ConduitTopology>(new ConduitTopology(node[key], node[key]));
    return nullptr;
}
