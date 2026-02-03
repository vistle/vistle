#include "conduitToVistle.h"
#include "conduitTopology.h"
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

template<typename ScalarA, typename ScalarB>
void copy(const ScalarA *in, ScalarB *out, conduit_index_t numElements, conduit_index_t offset, conduit_index_t stride)
{
    for (conduit_index_t i = 0; i < numElements; i++) {
        out[i] = static_cast<ScalarB>(in[offset / sizeof(ScalarA) + i * stride / sizeof(ScalarA)]);
    }
}

void getCoord(const conduit_cpp::Node &coord, Scalar *out)
{
    auto dt = coord.dtype();
    if (dt.is_float()) {
        const float *data = static_cast<const float *>(coord.data_ptr());
        copy(data, out, coord.number_of_elements(), dt.offset(), dt.stride());
    } else if (dt.is_double()) {
        const double *data = static_cast<const double *>(coord.data_ptr());
        copy(data, out, coord.number_of_elements(), dt.offset(), dt.stride());
    } else {
        std::cerr << "unsupported coordinate data type " << dt.type_id() << ", " << dt.name() << std::endl;
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

size_t calcPolyhedronCorners(conduit_int32 numFaces, const conduit_int32 *cl, UnstructuredGrid::Type subShape,
                             const std::map<int, UnstructuredGrid::Type> &subShapeMap, const conduit_int32 *subShapes,
                             const conduit_int32 *subSizes)
{
    size_t numCorners = 0;
    for (auto f = 0; f < numFaces; f++) {
        auto facePos = cl[f];
        auto faceType =
            subShape == UnstructuredGrid::Type::NONE ? subShapeMap.find(subShapes[facePos])->second : subShape;
        if (faceType == UnstructuredGrid::Type::POLYGON) {
            numCorners += subSizes[facePos];
        } else {
            numCorners += UnstructuredGrid::NumVertices[faceType];
        }
        // each polyhedron face is terminated with its starting index
        ++numCorners;
    }
    return numCorners;
}

UnstructuredGrid::ptr toUnstructured(const conduit_cpp::Node &coords, const conduit_cpp::Node &topology)
{
    ConduitTopology topo(topology);

    size_t numElements = 0;
    size_t numCorners = 0;
    size_t numCornersPerElement = 0;

    bool containsPolyhedra = false;

    if (topo.isMixed) {
        numElements = topo.shapes.size;
        for (size_t i = 0; i < numElements; i++) {
            if (topo.shapeMap.find(topo.shapes[i])->second == UnstructuredGrid::Type::POLYHEDRON) {
                containsPolyhedra = true;
                auto pos = topo.offsets[i];
                auto numFaces = topo.sizes[i];
                numCorners += calcPolyhedronCorners(numFaces, topo.connectivity.data + pos, topo.sub->shape,
                                                    topo.sub->shapeMap, topo.sub->shapes.data, topo.sub->sizes.data);

            } else
                numCorners += topo.sizes[i];
        }

    } else {
        if (topo.shape == UnstructuredGrid::Type::POLYHEDRON) {
            // warning: this case is not tested
            containsPolyhedra = true;
            for (size_t i = 0; i < numElements; i++) {
                auto pos = topo.offsets[i];
                auto numFaces = topo.sizes[i];
                numCorners += calcPolyhedronCorners(numFaces, topo.connectivity.data + pos, topo.sub->shape,
                                                    topo.sub->shapeMap, topo.sub->shapes.data, topo.sub->sizes.data);
            }

        } else if (topo.shape == UnstructuredGrid::Type::POLYGON) {
            numElements = topo.offsets.size;
            numCorners = topo.offsets[numElements - 1] + topo.sizes[numElements - 1];
        } else {
            numCornersPerElement = UnstructuredGrid::NumVertices[topo.shape];
            if (!numCornersPerElement) {
                std::cerr << "conduitToVistle: unsupported element type " << UnstructuredGrid::toString(topo.shape)
                          << std::endl;
                return nullptr;
            }
            numElements = numCorners / numCornersPerElement;
        }
    }

    const auto numVertices = coords["values/x"].number_of_elements();
    if (numVertices != coords["values/y"].number_of_elements() ||
        numVertices != coords["values/z"].number_of_elements()) {
        std::cerr << "inconsistent number of coordinate values" << std::endl;
        return nullptr;
    }
    auto grid = std::make_shared<UnstructuredGrid>(numElements, numCorners, numVertices);
    auto cl = grid->cl().data();
    auto tl = grid->tl().data();
    auto el = grid->el().data();
    auto x = grid->x().data();
    auto y = grid->y().data();
    auto z = grid->z().data();
    getCoords(coords, x, y, z);
    if (!containsPolyhedra) {
        for (size_t i = 0; i < numCorners; i++) {
            cl[i] = topo.connectivity[i];
        }
        if (topo.isMixed) {
            Index pos = 0;
            for (size_t i = 0; i < numElements; i++) {
                el[i] = pos;
                tl[i] = topo.shapeMap.find(topo.shapes[i])->second;
                pos += UnstructuredGrid::NumVertices[tl[i]];
            }
            el[numElements] = pos;

        } else {
            for (size_t i = 0; i < numElements; i++) {
                el[i] = i * numCornersPerElement;
                tl[i] = topo.shape;
            }
            el[numElements] = numElements * numCornersPerElement;
        }
    } else {
        size_t cornerPos = 0;
        size_t totalNumPolyhedronFaces = 0;
        const conduit_int32 *subOffsets = nullptr;
        if (topology.has_child("subelements/offsets"))
            subOffsets = topology["subelements/offsets"].as_int32_ptr();
        size_t subOffset = 0;
        for (size_t i = 0; i < numElements; i++) {
            auto elemType = topo.isMixed ? topo.shapeMap.find(topo.shapes[i])->second : topo.shape;
            tl[i] = elemType;
            el[i] = cornerPos;
            auto pos = topo.offsets[i];
            if (elemType == UnstructuredGrid::Type::POLYHEDRON) {
                auto numFaces = topo.sizes[i];
                for (auto f = 0; f < numFaces; f++) {
                    auto facePos = topo.connectivity[pos + f];
                    auto faceType = topo.sub->shape;
                    if (topo.sub->shapes.data) {
                        faceType = topo.sub->shapeMap.find(topo.sub->shapes[facePos])->second;
                    }
                    size_t numFaceCorners = UnstructuredGrid::NumVertices[faceType];
                    if (faceType == UnstructuredGrid::Type::POLYGON) {
                        numFaceCorners = topo.sub->sizes[facePos];
                        subOffset = subOffsets[facePos];
                    }
                    for (size_t v = 0; v < numFaceCorners; v++) {
                        cl[cornerPos] = topo.sub->connectivity[subOffset + v];
                        ++cornerPos;
                    }
                    cl[cornerPos] = cl[cornerPos - numFaceCorners]; // terminate face with starting index
                    ++cornerPos;
                    ++totalNumPolyhedronFaces;
                    subOffset += numFaceCorners;
                }
            } else {
                auto numCornersElem = topo.sizes[i];
                for (auto c = 0; c < numCornersElem; c++) {
                    cl[cornerPos] = topo.connectivity[pos + c];
                    ++cornerPos;
                }
            }
        }
        el[numElements] = cornerPos;
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

vistle::DataBase::ptr conduitDataToVistle(const conduit_cpp::Node &field, int sourceId)
{
    std::string mappingName = field["association"].as_string();
    assert(mappingName == "vertex" || mappingName == "element");
    DataBase::Mapping mapping = mappingName == "vertex" ? DataBase::Mapping::Vertex : DataBase::Mapping::Element;
    auto values = field["values"];
    conduit_cpp::Node info;
    auto isVectorField = conduit_cpp::Blueprint::verify("mcarray", values, info);
    auto isScalarField = values.dtype().is_number() && values.dtype().number_of_elements() > 1;
    if (!isVectorField && !isScalarField) {
        std::cerr << "conduitToVistle: field is not a valid array" << std::endl;
        std::cerr << info.to_yaml() << std::endl;
        return nullptr;
    }
    if (isScalarField) {
        auto vec = std::make_shared<vistle::Vec<vistle::Scalar, 1>>((size_t)values.dtype().number_of_elements());
        getCoord(values, vec->x().data());
        vec->setMapping(mapping);
        vec->describe(field.name(), sourceId);
        return vec;
    }
    if (values.has_child("y")) {
        if (!values.has_child("z")) {
            std::cerr << "conduitToVistle: 2D fields are not supported" << std::endl;
            return nullptr;
        }
        auto vec = std::make_shared<vistle::Vec<vistle::Scalar, 3>>((size_t)values["x"].dtype().number_of_elements());
        getCoords(field, vec->x().data(), vec->y().data(), vec->z().data());
        vec->setMapping(mapping);
        vec->describe(field.name(), sourceId);
        return vec;
    } else {
        auto vec = std::make_shared<vistle::Vec<vistle::Scalar, 1>>((size_t)values["x"].dtype().number_of_elements());
        getCoord(values, vec->x().data());
        vec->setMapping(mapping);
        vec->describe(field.name(), sourceId);
        return vec;
    }

    return nullptr;
}
