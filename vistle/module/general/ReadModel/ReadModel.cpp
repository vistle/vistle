#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/points.h>

#include "ReadModel.h"

MODULE_MAIN(ReadModel)

using namespace vistle;

ReadModel::ReadModel(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadModel", shmname, name, moduleID)
{

   createOutputPort("grid_out");
   addStringParameter("filename", "name of file", "");
}

ReadModel::~ReadModel() {

}

bool ReadModel::load(const std::string &name) {

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(name, aiProcess_JoinIdenticalVertices|aiProcess_SortByPType|aiProcess_PreTransformVertices);
    if (!scene) {
        std::stringstream str;
        str << "failed to read " << name << ": " << importer.GetErrorString() << std::endl;
        std::string s = str.str();
        sendError("%s", s.c_str());
        return false;
    }

    for (unsigned int m=0; m<scene->mNumMeshes; ++m) {

        const aiMesh *mesh = scene->mMeshes[m];
        if (mesh->HasPositions()) {
            if (mesh->HasFaces()) {
                Scalar *x[3] = { nullptr, nullptr, nullptr };
                Coords::ptr coords;
                auto numVert = mesh->mNumVertices;
                auto numFace = mesh->mNumFaces;
                if (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON) {
                    Index numIndex = 0;
                    for (unsigned int f=0; f<numFace; ++f) {
                        numIndex += mesh->mFaces[f].mNumIndices;
                    }
                    Polygons::ptr poly(new Polygons(numFace, numIndex, numVert));
                    coords = poly;
                    auto *el = &poly->el()[0];
                    auto *cl = &poly->cl()[0];
                    Index idx=0, vertCount=0;
                    for (unsigned int f=0; f<numFace; ++f) {
                        el[idx++] = vertCount;
                        const auto &face = mesh->mFaces[f];
                        for (unsigned int i=0; i<face.mNumIndices; ++i) {
                            cl[vertCount++] = face.mIndices[i];
                        }
                    }
                    el[idx] = vertCount;
                } else if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
                    Index numIndex = mesh->mNumFaces*3;
                    Triangles::ptr tri(new Triangles(numIndex, numVert));
                    coords = tri;
                    auto *cl = &tri->cl()[0];
                    Index vertCount=0;
                    for (unsigned int f=0; f<numFace; ++f) {
                        const auto &face = mesh->mFaces[f];
                        vassert(face.mNumIndices == 3);
                        for (unsigned int i=0; i<face.mNumIndices; ++i) {
                            cl[vertCount++] = face.mIndices[i];
                        }
                    }
                }
                if (coords) {
                    for (int c=0; c<3; ++c) {
                        x[c] = &coords->x(c)[0];
                    }
                    for (Index i=0; i<mesh->mNumVertices; ++i) {
                        const auto &vert = mesh->mVertices[i];
                        for (unsigned int c=0; c<3; ++c) {
                            x[c][i] = vert[c];
                        }
                    }
                    addObject("grid_out", coords);
                }
            }
        }
    }

    return true;
}

bool ReadModel::compute() {

   if (!load(getStringParameter("filename"))) {

   }
   return true;
}
