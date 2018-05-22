#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <boost/format.hpp>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/triangles.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/normals.h>

#include "ReadModel.h"

MODULE_MAIN(ReadModel)

using namespace vistle;

ReadModel::ReadModel(const std::string &name, int moduleID, mpi::communicator comm)
   : Module("read polygonal models with Assimp", name, moduleID, comm)
{

   createOutputPort("grid_out");
   addStringParameter("filename", "name of file (%1%: block, %2%: timestep)", "");
   addIntParameter("indexed_geometry", "create indexed geometry?", 0, Parameter::Boolean);
   addIntParameter("triangulate", "only create triangles", 0, Parameter::Boolean);

   addIntParameter("first_block", "number of first block", 0);
   addIntParameter("last_block", "number of last block", 0);

   addIntParameter("first_step", "number of first timestep", 0);
   addIntParameter("last_step", "number of last timestep", 0);
   addIntParameter("step", "increment", 1);

   addIntParameter("ignore_errors", "ignore files that are not found", 0, Parameter::Boolean);
}

ReadModel::~ReadModel() {

}

int ReadModel::rankForBlock(int block) const {

    const int numBlocks = m_lastBlock-m_firstBlock+1;
    if (numBlocks == 0)
        return 0;

    if (block == -1)
        return -1;

    return block % size();
}


Object::ptr ReadModel::load(const std::string &name) {

    Object::ptr ret;
    Assimp::Importer importer;
    bool indexed = false;
    unsigned int readFlags = aiProcess_PreTransformVertices|aiProcess_SortByPType|aiProcess_ImproveCacheLocality|aiProcess_OptimizeGraph|aiProcess_OptimizeMeshes;
    if (getIntParameter("indexed_geometry")) {
        readFlags |= aiProcess_JoinIdenticalVertices;
        indexed = true;
    }
    if (getIntParameter("triangulate"))
        readFlags |= aiProcess_Triangulate;
    const aiScene* scene = importer.ReadFile(name, readFlags);
    if (!scene) {
        if (!getIntParameter("ignore_errors")) {
            std::stringstream str;
            str << "failed to read " << name << ": " << importer.GetErrorString() << std::endl;
            std::string s = str.str();
            sendError("%s", s.c_str());
        }
        return ret;
    }

    for (unsigned int m=0; m<scene->mNumMeshes; ++m) {

        const aiMesh *mesh = scene->mMeshes[m];
        if (mesh->HasPositions()) {
            Coords::ptr coords;
            if (mesh->HasFaces()) {
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
                    auto *cl = indexed ? &poly->cl()[0] : nullptr;
                    Index idx=0, vertCount=0;
                    for (unsigned int f=0; f<numFace; ++f) {
                        el[idx++] = vertCount;
                        if (indexed) {
                            const auto &face = mesh->mFaces[f];
                            for (unsigned int i=0; i<face.mNumIndices; ++i) {
                                cl[vertCount++] = face.mIndices[i];
                            }
                        }
                    }
                    el[idx] = vertCount;
                } else if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
                    Index numIndex = indexed ? mesh->mNumFaces*3 : 0;
                    Triangles::ptr tri(new Triangles(numIndex, numVert));
                    coords = tri;
                    if (indexed) {
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
                }
            } else {
                Points::ptr points(new Points(mesh->mNumVertices));
                coords = points;
            }
            if (coords) {
                Scalar *x[3] = { nullptr, nullptr, nullptr };
                for (int c=0; c<3; ++c) {
                    x[c] = &coords->x(c)[0];
                }
                for (Index i=0; i<mesh->mNumVertices; ++i) {
                    const auto &vert = mesh->mVertices[i];
                    for (unsigned int c=0; c<3; ++c) {
                        x[c][i] = vert[c];
                    }
                }
                ret = coords;
                if (mesh->HasNormals()) {
                    Normals::ptr normals(new Normals(mesh->mNumVertices));
                    Scalar *n[3] = { nullptr, nullptr, nullptr };
                    for (int c=0; c<3; ++c) {
                        n[c] = &normals->x(c)[0];
                    }
                    for (Index i=0; i<mesh->mNumVertices; ++i) {
                        const auto &norm = mesh->mNormals[i];
                        for (unsigned int c=0; c<3; ++c) {
                            n[c][i] = norm[c];
                        }
                    }
                    normals->updateInternals();

                    coords->setNormals(normals);
                }
            }
        }

        break;
    }

    if (scene->mNumMeshes > 1) {
        sendInfo("file %s contains %d meshes, all but the first have been ignored", name.c_str(), scene->mNumMeshes);
    }

    return ret;
}

bool ReadModel::compute() {

   m_firstBlock = getIntParameter("first_block");
   m_lastBlock = getIntParameter("last_block");
   m_firstStep = getIntParameter("first_step");
   m_lastStep = getIntParameter("last_step");
   m_step = getIntParameter("step");
   if ((m_lastStep-m_firstStep)*m_step < 0)
       m_step *= -1;
   bool reverse = false;
   int step = m_step, first = m_firstStep, last = m_lastStep;
   if (step < 0) {
       reverse = true;
       step *= -1;
       first *= -1;
       last *= -1;
   }

   std::string filename = getStringParameter("filename");

   int timeCounter = 0;
   for (int t=first; t<=last; t += step) {
       int timestep = reverse ? -t : t;
       bool loaded = false;
       for (int b=m_firstBlock; b<=m_lastBlock; ++b) {
           if (rankForBlock(b) == rank()) {
               std::string f;
               try {
                   using namespace boost::io;
                   boost::format fmter(filename);
                   fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit));
                   fmter % b;
                   fmter % timestep;
                   f = fmter.str();
               } catch (boost::io::bad_format_string except) {
                   sendError("bad format string in filename");
                   return true;
               }
               auto obj = load(f);
               if (!obj) {
                   if (!getIntParameter("ignore_errors")) {
                       sendError("failed to load %s", f.c_str());
                       return true;
                   }
               } else {
                   obj->setBlock(b);
                   obj->setTimestep(timeCounter);
                   loaded = true;
                   addObject("grid_out", obj);
               }
               if (cancelRequested()) {
                   break;
               }
           }
       }
       if (loaded)
           ++timeCounter;
       if (cancelRequested())
           break;
   }
   return true;
}
