#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <boost/format.hpp>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/polygons.h>
#include <vistle/core/triangles.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/normals.h>
#include <vistle/core/unstr.h>

#include "ReadModel.h"

MODULE_MAIN(ReadModel)

using namespace vistle;

ReadModel::ReadModel(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    createOutputPort("grid_out", "grid or geometry");
    auto filename = addStringParameter("filename", "name of file (%1%: block, %2%: timestep)", "", Parameter::Filename);
    setParameterFilters(filename, "Wavefront Obj (*.obj)/3D Systems stereolithography CAD (*.stl)/Stanford Polygon "
                                  "Library (*.ply)/Autodesk FBX (*.fbx)");
    observeParameter(filename);

    addIntParameter("indexed_geometry", "create indexed geometry?", 0, Parameter::Boolean);
    addIntParameter("triangulate", "only create triangles", 0, Parameter::Boolean);
    addIntParameter("normals", "read normals, if available", 1, Parameter::Boolean);

    auto firstBlock = addIntParameter("first_block", "number of first block", 0);
    observeParameter(firstBlock);
    auto lastBlock = addIntParameter("last_block", "number of last block", 0);
    observeParameter(lastBlock);

    addIntParameter("ignore_errors", "ignore files that are not found", 0, Parameter::Boolean);

    observeParameter(m_first);
    observeParameter(m_last);
}

bool ReadModel::examine(const Parameter *param)
{
    vistle::Integer numtime = 0;
    if (m_first && m_first->getValue() > numtime)
        numtime = m_first->getValue();
    if (m_last && m_last->getValue() > numtime)
        numtime = m_last->getValue();
    setTimesteps((int)numtime);

    m_firstBlock = getIntParameter("first_block");
    m_lastBlock = getIntParameter("last_block");
    setPartitions(m_lastBlock - m_firstBlock + 1);

    return true;
}

bool ReadModel::read(Reader::Token &token, int timestep, int block)
{
    std::string filename = getStringParameter("filename");

    std::string f;
    try {
        using namespace boost::io;
        boost::format fmter(filename);
        fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit));
        fmter % block;
        fmter % timestep;
        f = fmter.str();
    } catch (boost::io::bad_format_string &except) {
        sendError("bad format string in filename %s", filename.c_str());
        return false;
    }

    auto obj = load(f);
    if (!obj) {
        if (!getIntParameter("ignore_errors")) {
            sendError("failed to load %s", f.c_str());
            return false;
        }
    } else {
        token.applyMeta(obj);
        token.addObject("grid_out", obj);
    }

    return true;
}

bool ReadModel::prepareRead()
{
    m_firstBlock = getIntParameter("first_block");
    m_lastBlock = getIntParameter("last_block");

    return true;
}

Object::ptr ReadModel::load(const std::string &name) const
{
    Assimp::Importer importer;
    bool indexed = false;
    bool readNormals = getIntParameter("normals");
    unsigned int readFlags = aiProcess_PreTransformVertices | aiProcess_SortByPType | aiProcess_ImproveCacheLocality |
                             aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes;
    if (getIntParameter("indexed_geometry")) {
        readFlags |= aiProcess_JoinIdenticalVertices;
        indexed = true;
    }
    if (getIntParameter("triangulate"))
        readFlags |= aiProcess_Triangulate;
    const aiScene *scene = importer.ReadFile(name, readFlags);
    if (!scene) {
        if (!getIntParameter("ignore_errors")) {
            std::stringstream str;
            str << "failed to read " << name << ": " << importer.GetErrorString() << std::endl;
            std::string s = str.str();
            sendError("%s", s.c_str());
        }
        return {};
    }

    enum OutputType {
        Unknown,
        Point,
        Line,
        Triangle,
        Polygon,
        Unstructured,
    };
    OutputType outputType = Unknown;

    size_t totNumVert = 0, totNumIndex = 0, totNumFace = 0;
    bool haveNormals = false;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh *mesh = scene->mMeshes[m];
        if (!mesh->HasPositions()) {
            continue;
        }
        if (mesh->HasNormals() && readNormals) {
            haveNormals = true;
        }

        if (mesh->HasFaces()) {
            if (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON) {
                if (outputType == Polygon || outputType == Triangle || outputType == Unknown)
                    outputType = Polygon;
                else
                    outputType = Unstructured;
                totNumFace += mesh->mNumFaces;
                totNumVert += mesh->mNumVertices;
                Index numIndex = 0;
                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    numIndex += mesh->mFaces[f].mNumIndices;
                }
                totNumIndex += numIndex;
            }
            if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
                if (outputType == Triangle || outputType == Unknown)
                    outputType = Triangle;
                else if (outputType != Polygon)
                    outputType = Unstructured;
                totNumFace += mesh->mNumFaces;
                totNumVert += mesh->mNumVertices;
                Index numIndex = indexed ? mesh->mNumFaces * 3 : 0;
                totNumIndex += numIndex;
            }
        } else {
            if (outputType == Point || outputType == Unknown)
                outputType = Point;
            else
                outputType = Unstructured;
            totNumVert += mesh->mNumVertices;
        }
    }

    Normals::ptr normals;

    Coords::ptr coords;
    Points::ptr points;
    Triangles::ptr tri;
    Polygons::ptr poly;
    UnstructuredGrid::ptr unstr;
    Index *el = nullptr, *cl = nullptr;
    Byte *tl = nullptr;
    Scalar *x[3] = {};
    Scalar *norm[3] = {};
    switch (outputType) {
    case Unknown:
        return {};
        break;
    case Point:
        coords = points = std::make_shared<Points>(totNumVert);
        break;
    case Triangle:
        coords = tri = std::make_shared<Triangles>(totNumIndex, totNumVert);
        if (indexed)
            cl = tri->cl().data();
        break;
    case Polygon:
        coords = poly = std::make_shared<Polygons>(totNumFace, totNumIndex, totNumVert);
        el = poly->el().data();
        if (indexed)
            cl = poly->cl().data();
        break;
    case Unstructured:
        coords = unstr = std::make_shared<UnstructuredGrid>(totNumFace, totNumIndex, totNumVert);
        tl = unstr->tl().data();
        el = unstr->el().data();
        if (indexed)
            cl = poly->cl().data();
        break;
    }
    if (coords) {
        for (int c = 0; c < 3; ++c) {
            x[c] = coords->x(c).data();
        }
    }
    if (haveNormals) {
        normals = std::make_shared<Normals>(totNumVert);
        for (int c = 0; c < 3; ++c) {
            norm[c] = normals->x(c).data();
        }
    }

    Index vertCount = 0, coordCount = 0;
    Index idx = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh *mesh = scene->mMeshes[m];
        if (mesh->HasPositions()) {
            if (mesh->HasFaces()) {
                auto numFace = mesh->mNumFaces;
                if (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON) {
                    for (unsigned int f = 0; f < numFace; ++f) {
                        if (indexed) {
                            const auto &face = mesh->mFaces[f];
                            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                                cl[vertCount++] = face.mIndices[i];
                            }
                        }
                        if (tl)
                            tl[idx] = UnstructuredGrid::POLYGON;
                        el[idx++] = vertCount;
                    }
                } else if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
                    if (indexed) {
                        for (unsigned int f = 0; f < numFace; ++f) {
                            const auto &face = mesh->mFaces[f];
                            assert(face.mNumIndices == 3);
                            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                                cl[vertCount++] = face.mIndices[i];
                            }
                            if (tl)
                                tl[idx] = UnstructuredGrid::TRIANGLE;
                            if (el)
                                el[idx++] = vertCount;
                        }
                    }
                }
            } else {
                ++vertCount;
                if (tl)
                    tl[idx] = UnstructuredGrid::POINT;
                if (el)
                    el[idx++] = vertCount;
            }
            if (coords) {
                for (Index i = 0; i < mesh->mNumVertices; ++i) {
                    const auto &vert = mesh->mVertices[i];
                    for (unsigned int c = 0; c < 3; ++c) {
                        x[c][coordCount + i] = vert[c];
                    }
                }
                if (mesh->HasNormals() && readNormals) {
                    for (Index i = 0; i < mesh->mNumVertices; ++i) {
                        const auto &n = mesh->mNormals[i];
                        for (unsigned int c = 0; c < 3; ++c) {
                            norm[c][coordCount + i] = n[c];
                        }
                    }
                }

                coordCount += mesh->mNumVertices;
            }
        }
    }

    if (normals) {
        updateMeta(normals);
        coords->setNormals(normals);
    }

    return coords;
}
