#include "conduitToVistle.h"

#include <catalyst_api.h>
#include <catalyst_conduit.hpp>
#include <catalyst_conduit_blueprint.hpp>
#include <catalyst_stub.h>

#include <iostream>

#include <vistle/insitu/adapter/adapter.h>

#include <vistle/core/unstr.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/object.h>
#include <vistle/core/celltypes.h>

using namespace vistle;


std::array<vistle::Index, 3> getDims(const conduit_cpp::Node &dims)
{
    auto twoD = dims.number_of_children() == 2;
    assert(twoD || dims.number_of_children() == 3);
    return {dims["i"].to_uint32(), dims["j"].to_uint32(), twoD ? 1 : dims["k"].to_uint32()};
}

void getCoord(const conduit_cpp::Node &coord, Scalar *out)
{
    assert(coord.dtype().is_float());
    const float *data = static_cast<const float *>(coord.data_ptr());
    auto dt = coord.dtype();
    for (conduit_index_t i = 0; i < coord.number_of_elements(); i++) {
        out[i] = data[(dt.offset() + i * dt.stride()) / dt.element_bytes()];
    }
}

void getCoords(const conduit_cpp::Node &coords, Scalar *xOut, Scalar *yOut, Scalar *zOut)
{
    getCoord(coords["values/x"], xOut);
    getCoord(coords["values/y"], yOut);
    if (coords["values"].has_child("z"))
        getCoord(coords["values/z"], zOut);
}

StructuredGrid::ptr toStructured(const conduit_cpp::Node &coords, const conduit_cpp::Node &dims)
{
    auto d = getDims(dims);
    StructuredGrid::ptr grid = std::make_shared<StructuredGrid>(d[0], d[1], d[2]);
    auto x = grid->x().data();
    auto y = grid->y().data();
    auto z = grid->z().data();
    getCoords(coords, x, y, z);
    return grid;
}

constexpr int numElementTypes = 7;
constexpr int elementTypesVistle[numElementTypes] = {
    UnstructuredGrid::Type::POINT,       UnstructuredGrid::Type::TRIANGLE,   UnstructuredGrid::Type::QUAD,
    UnstructuredGrid::Type::TETRAHEDRON, UnstructuredGrid::Type::HEXAHEDRON, UnstructuredGrid::Type::PYRAMID,
    UnstructuredGrid::Type::PRISM};
const char *elementTypeStrings[numElementTypes] = {"point", "tri", "quad", "tet", "hex", "pyramid", "wedge"};
constexpr int elementCorners[numElementTypes] = {1, 3, 4, 4, 8, 5, 6};

Index getNumCorners(const std::string &elementType)
{
    for (size_t i = 0; i < numElementTypes; i++) {
        if (elementType == elementTypeStrings[i]) {
            return elementCorners[i];
        }
    }
    std::cerr << elementType << " is not a supported" << std::endl;
    return 0;
}

std::map<int, UnstructuredGrid::Type> getShapeMap(const conduit_cpp::Node &shapeMap)
{
    std::map<int, UnstructuredGrid::Type> vistleShapeMap;
    for (conduit_index_t i = 0; i < shapeMap.number_of_children(); i++) {
        auto shape = shapeMap.child(i);
        auto shapeName = shape.name();
        auto shapeCode = shape.as_int();
        for (int i = 0; i < numElementTypes; i++) {
            if (shapeName == elementTypeStrings[i]) {
                vistleShapeMap[shapeCode] = static_cast<UnstructuredGrid::Type>(elementTypesVistle[i]);
                break;
            }
        }
    }
    return vistleShapeMap;
}

UnstructuredGrid::Type toVistleType(const std::string &elementType)
{
    for (size_t i = 0; i < numElementTypes; i++) {
        if (elementType == elementTypeStrings[i]) {
            return static_cast<UnstructuredGrid::Type>(elementTypesVistle[i]);
        }
    }
    return UnstructuredGrid::Type::NONE;
}

UnstructuredGrid::ptr toUnstructured(const conduit_cpp::Node &coords, const conduit_cpp::Node &topology)
{
    size_t numElements = 0;
    const size_t numCorners = topology["elements/connectivity"].number_of_elements();
    auto elementType = topology["elements/shape"].as_string();
    size_t numCornersPerElement = getNumCorners(elementType);
    if (elementType == "mixed") {
        auto shapes = topology["elements/shapes"];
        numElements = shapes.number_of_elements();
    } else {
        if (!numCornersPerElement) {
            return nullptr;
        }
        numElements = numCorners / numCornersPerElement;
    }

    const size_t numVertices = coords["values/x"].number_of_elements(); //assume x/y/z have the same number of elements
    auto grid = std::make_shared<UnstructuredGrid>(numElements, numCorners, numVertices);
    auto cl = grid->cl().data();
    auto tl = grid->tl().data();
    auto el = grid->el().data();
    auto x = grid->x().data();
    auto y = grid->y().data();
    auto z = grid->z().data();
    getCoords(coords, x, y, z);
    for (size_t i = 0; i < numCorners; i++) {
        cl[i] = static_cast<const int *>(topology["elements/connectivity"].data_ptr())[i];
    }
    if (elementType == "mixed") {
        std::map<int, UnstructuredGrid::Type> shapeMap = getShapeMap(topology["elements/shape/shape_map"]);
        auto shapes = static_cast<const int *>(topology["elements/shapes"].data_ptr());
        Index pos = 0;
        for (size_t i = 0; i < numElements; i++) {
            el[i] = pos;
            pos += UnstructuredGrid::NumVertices[shapeMap[shapes[i]]];
            tl[i] = shapeMap[shapes[i]];
        }
        el[numElements] = pos;

    } else {
        auto type = toVistleType(elementType);
        for (size_t i = 0; i < numElements; i++) {
            el[i] = i * numCornersPerElement;
            tl[i] = type;
        }
        el[numElements] = numElements * numCornersPerElement;
    }
    return grid;
}


Object::ptr conduitMeshToVistle(const conduit_cpp::Node &mesh, int sourceId)
{
    // create vistle object
    conduit_cpp::Node info;
    bool is_valid = conduit_cpp::Blueprint::verify("mesh", mesh, info);
    if (!is_valid) {
        std::cerr << "conduitToVistle: mesh is not a valid mesh" << std::endl;
        std::cerr << info.to_yaml() << std::endl;
        return nullptr;
    }
    for (conduit_index_t i = 0; i < mesh["topologies"].number_of_children(); i++) {
        auto topologyNode = mesh["topologies"].child(i);
        auto type = topologyNode["type"].as_string();
        auto coordset = mesh["coordsets/" + topologyNode["coordset"].as_string()];

        if (type == "uniform") {
            std::cerr << "uniform not implemented" << std::endl;
        } else if (type == "rectilinear") {
            std::cerr << "rectilinear not implemented" << std::endl;
        } else if (type == "structured") {
            return toStructured(coordset, topologyNode["elements/dims"]);
        } else if (type == "unstructured") {
            return toUnstructured(coordset, topologyNode);
        }
    }


    return nullptr;
}

// Material-Independent Fields:
// fields/field/association: “vertex” | “element”
// not supported: fields/field/grid_function: (mfem-style finite element collection name) (replaces “association”)
// not considered: fields/field/volume_dependent: “true” | “false”
// fields/field/topology: “topo”
// fields/field/values: (mcarray)
// fields/field/offsets: (integer array) (optional - for strided structured topology)
// fields/field/strides: (integer array) (optional - for strided structured topology)

vistle::DataBase::ptr conduitDataToVistle(const conduit_cpp::Node &field, int sourceId)
{
    std::string mappingName = field["association"].as_string();
    assert(mappingName == "vertex" || mappingName == "element");
    DataBase::Mapping mapping = mappingName == "vertex" ? DataBase::Mapping::Vertex : DataBase::Mapping::Element;
    auto values = field["values"];
    conduit_cpp::Node info;
    auto ok = conduit_cpp::Blueprint::verify("mcarray", values, info);
    if (!ok) {
        std::cerr << "conduitToVistle: field is not a valid mcarray" << std::endl;
        std::cerr << info.to_yaml() << std::endl;
        return nullptr;
    }
    if (!values.has_child("x")) {
        std::cerr << "conduitToVistle: field does not have x values" << std::endl;
        return nullptr;
    }
    if (values.has_child("y")) {
        if (values.has_child("z")) {
            auto vec =
                std::make_shared<vistle::Vec<vistle::Scalar, 3>>((size_t)values["x"].dtype().number_of_elements());
            getCoords(field, vec->x().data(), vec->y().data(), vec->z().data());
            vec->setMapping(mapping);
            vec->describe(field.name(), sourceId);
            return vec;
        } else {
            std::cerr << "conduitToVistle: 2D fieldss are not supported" << std::endl;
            return nullptr;
        }
    } else {
        auto vec = std::make_shared<vistle::Vec<vistle::Scalar, 1>>((size_t)values["x"].dtype().number_of_elements());
        getCoord(values, vec->x().data());
        vec->setMapping(mapping);
        vec->describe(field.name(), sourceId);
        return vec;
    }

    return nullptr;
}
