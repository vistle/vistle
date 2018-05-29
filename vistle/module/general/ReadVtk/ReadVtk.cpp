#include <boost/algorithm/string/predicate.hpp>

#include <core/object.h>
#include <core/vec.h>
#include <core/polygons.h>
#include <core/lines.h>
#include <core/points.h>
#include <core/unstr.h>
#include <core/uniformgrid.h>
#include <core/rectilineargrid.h>
#include <core/structuredgrid.h>

#include <vtkDataSetReader.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkInformation.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkCompositeDataSet.h>
#include <vtkGenericDataSet.h>
#if VTK_MAJOR_VERSION < 5
#include <vtkIdType.h>
#endif
#include <vtkDataSetAttributes.h>
#include <vtkSmartPointer.h>

#include "ReadVtk.h"
#include "coVtk.h"
#include <util/filesystem.h>

#ifdef HAVE_TINYXML2
#include <tinyxml2.h>
namespace xml = tinyxml2;
#endif

MODULE_MAIN(ReadVtk)

using namespace vistle;

const std::string Invalid("(NONE)");

const double ConstantTime = -std::numeric_limits<double>::max();

struct VtkFile {
    std::string filename;
    std::string part;
    int partNum = -1;
    int pieces = 1;
    double realtime = ConstantTime;
    vtkDataObject *dataset = nullptr;
    std::vector<std::string> pointfields;
    std::vector<std::string> cellfields;
};

template<class Reader>
VtkFile readFile(const std::string &filename, int piece=-1, bool ghost=false, bool onlyMeta=false) {
   VtkFile result;
   result.filename = filename;
   if (!vistle::filesystem::is_regular_file(filename)) {
       result.pieces = 0;
       return result;
   }

   vtkSmartPointer<Reader> reader = vtkSmartPointer<Reader>::New();
   if (auto xmlreader = vtkXMLReader::SafeDownCast(reader)) {
       if (!xmlreader->CanReadFile(filename.c_str())) {
           result.pieces = 0;
           return result;
       }
   }
   reader->SetFileName(filename.c_str());
   reader->UpdateInformation();
   int numPieces = 1;
#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION>7
   if (auto unstr = vtkXMLUnstructuredDataReader::SafeDownCast(reader)) {
       numPieces = unstr->GetNumberOfPieces();
   }
#endif
   result.pieces = numPieces;

   if (auto xmlreader = vtkXMLReader::SafeDownCast(reader)) {
       const int np = xmlreader->GetNumberOfPointArrays();
       for (int i=0; i<np; ++i)
           result.pointfields.push_back(xmlreader->GetPointArrayName(i));
       const int nc = xmlreader->GetNumberOfCellArrays();
       for (int i=0; i<nc; ++i)
           result.cellfields.push_back(xmlreader->GetCellArrayName(i));
   } else if (auto dsreader = vtkDataReader::SafeDownCast(reader)) {
       const int ns = dsreader->GetNumberOfScalarsInFile();
       for (int i=0; i<ns; ++i)
           result.pointfields.push_back(dsreader->GetScalarsNameInFile(i));
       const int nv = dsreader->GetNumberOfVectorsInFile();
       for (int i=0; i<nv; ++i)
           result.pointfields.push_back(dsreader->GetVectorsNameInFile(i));
   }

   if (onlyMeta) {
       if (reader->GetOutput()) {
           reader->GetOutput()->Register(reader);
           result.dataset = reader->GetOutput();
       }
       return result;
   }

#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION>7
   if (piece >= 0 && numPieces > 1) {
       auto info = vtkSmartPointer<vtkInformation>::New();
       info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
       info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
       info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghost ? 1 : 0);
       reader->Update(info);
   } else
#endif
   {
       reader->Update();
   }

   if (reader->GetOutput()) {
      reader->GetOutput()->Register(reader);
           result.dataset = reader->GetOutput();
   }
   return result;
}


VtkFile getDataSet(const std::string &filename, int piece=-1, bool ghost=false, bool onlyMeta=false) {
   VtkFile fileinfo;
   bool triedLegacy = false;
   if (boost::algorithm::ends_with(filename, ".vtk")) {
      fileinfo = readFile<vtkDataSetReader>(filename, piece, ghost, onlyMeta);
      triedLegacy = true;
   }
   if (!fileinfo.dataset) {
      fileinfo = readFile<vtkXMLUnstructuredGridReader>(filename, piece, ghost, onlyMeta);
   }
   if (!fileinfo.dataset) {
      fileinfo = readFile<vtkXMLMultiBlockDataReader>(filename, piece, ghost, onlyMeta);
   }
   if (!fileinfo.dataset) {
      fileinfo = readFile<vtkXMLGenericDataObjectReader>(filename, piece, ghost, onlyMeta);
   }
   if (!triedLegacy && !fileinfo.dataset) {
      fileinfo = readFile<vtkDataSetReader>(filename, piece, ghost, onlyMeta);
   }
   return fileinfo;
}

VtkFile getDataSetMeta(const std::string &filename) {
    return getDataSet(filename, -1, false, true);
}

int getNumPieces(const std::string &filename) {
    return getDataSetMeta(filename).pieces;
}

struct ReadVtkData {

   std::vector<double> times;
   std::map<double, std::vector<VtkFile>> timesteps;
};


std::map<double, std::vector<VtkFile>> readPvd(const std::string &filename, bool piecesAsBlocks=false) {

    std::map<double, std::vector<VtkFile>> timesteps;

#ifdef HAVE_TINYXML2
    vistle::filesystem::path pvdpath(filename);
    if (!vistle::filesystem::is_regular_file(pvdpath)) {
        std::cerr << "readPvd: " << filename << " is not a regular file" << std::endl;
        return timesteps;
    }
    xml::XMLDocument doc;
    doc.LoadFile(filename.c_str());
    auto VTKFile = doc.FirstChildElement("VTKFile");
    if (!VTKFile) {
        std::cerr << "readPvd: did not find VTKFile element" << std::endl;
        return timesteps;
    }
    const char *type = VTKFile->Attribute("type");
    if (!type || std::string(type) != "Collection") {
        std::cerr << "readPvd: VTKFile not of required type 'Collection'" << std::endl;
        return timesteps;
    }

    auto Collection = VTKFile->FirstChildElement("Collection");
    if (!Collection) {
        std::cerr << "readPvd: did not find Collection element" << std::endl;
        return timesteps;
    }
    auto DataSet = Collection->FirstChildElement("DataSet");
    if (!DataSet) {
        std::cerr << "readPvd: did not find a DataSet element" << std::endl;
        return timesteps;
    }

    int numBlocks = 0;
    while (DataSet) {
        double time = ConstantTime;
        int p = 0;
        const char *file = DataSet->Attribute("file");
        const char *realtime = DataSet->Attribute("timestep");
        if (realtime)
            time = atof(realtime);
        const char *part = DataSet->Attribute("part");
        if (part) {
            p = atoi(part);
        }

        if (file) {
            VtkFile vtkFile;
            vistle::filesystem::path fp(file);
            if (fp.is_relative()) {
                auto p = pvdpath.parent_path();
                p /= fp;
                vtkFile.filename = p.string();
            } else {
                vtkFile.filename = file;
            }
            vtkFile.partNum = p;
            if (part)
                vtkFile.part = part;
            vtkFile.realtime = time;
            if (piecesAsBlocks) {
                vtkFile.pieces = getNumPieces(vtkFile.filename);
            }

            auto &timestep = timesteps[time];
            timestep.push_back(vtkFile);

            ++numBlocks;
        }

        DataSet = DataSet->NextSiblingElement("DataSet");
    }
#endif

    //std::cerr << "readPvd(" << filename << "): num timesteps: " << timesteps.size() << ", num blocks: " << numBlocks <<  std::endl;

    return timesteps;
}

template<class VO>
std::vector<std::string> getFields(VO *dsa)
{
    std::vector<std::string> fields;
    if (!dsa)
        return fields;
    int na = dsa->GetNumberOfArrays();
    for (int i=0; i<na; ++i)
    {
        fields.push_back(dsa->GetArrayName(i));
        //cerr << "field " << i << ": " << fields[i] << endl;
    }
    return fields;
}

void ReadVtk::setChoices(const VtkFile &fileinfo) {

    std::vector<std::string> cellFields({Invalid});
    std::copy(fileinfo.cellfields.begin(), fileinfo.cellfields.end(), std::back_inserter(cellFields));
    for (int i=0; i<NumPorts; ++i)
    {
        setParameterChoices(m_cellDataChoice[i], cellFields);
    }
    std::vector<std::string> pointFields({Invalid});
    std::copy(fileinfo.pointfields.begin(), fileinfo.pointfields.end(), std::back_inserter(pointFields));
    for (int i=0; i<NumPorts; ++i)
    {
        setParameterChoices(m_pointDataChoice[i], pointFields);
    }
}

ReadVtk::ReadVtk(const std::string &name, int moduleID, mpi::communicator comm)
   : Reader("read VTK data", name, moduleID, comm)
{

   createOutputPort("grid_out");
   m_filename = addStringParameter("filename", "name of VTK file", "", Parameter::ExistingPathname);
   m_readPieces = addIntParameter("read_pieces", "create block for every piece in an unstructured grid", true, Parameter::Boolean);
   m_ghostCells = addIntParameter("create_ghost_cells", "create ghost cells for multi-piece unstructured grids", true, Parameter::Boolean);

   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "point_field_" << i;
      m_pointDataChoice[i] = addStringParameter(spara.str(), "point data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "point_data" << i;
      m_pointPort[i] = createOutputPort(sport.str(), "vertex data");
   }
   for (int i=0; i<NumPorts; ++i) {
      std::stringstream spara;
      spara << "cell_field_" << i;
      m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);

      std::stringstream sport;
      sport << "cell_data" << i;
      m_cellPort[i] = createOutputPort(sport.str(), "cell data");
   }

   setParallelizationMode(ParallelizeTimeAndBlocks);
   observeParameter(m_filename);
   observeParameter(m_readPieces);
}

ReadVtk::~ReadVtk() {

}

bool ReadVtk::examine(const Parameter *param) {

    bool readPieces = m_readPieces->getValue();

    int maxNumPieces = 0;
    const std::string filename = m_filename->getValue();
    if (boost::algorithm::ends_with(filename, ".pvd")) {
#ifndef HAVE_TINYXML2
        sendError("not compiled against TinyXML2 - no support for parsing .pvd files");
#endif

        auto timesteps = readPvd(m_filename->getValue(), readPieces);
        int numt = 0;
        for (auto t: timesteps) {
            if (t.first != ConstantTime)
                ++numt;
        }
        setTimesteps(numt);

        for (auto t: timesteps) {
            if (!t.second.empty()) {
                setChoices(getDataSetMeta(t.second[0].filename));
                break;
            }
        }

        for (const auto &t: timesteps) {
            int npieces = 0;
            for (const auto &f: t.second)
                npieces += f.pieces;
            maxNumPieces = std::max(maxNumPieces, npieces);
        }

        setPartitions(maxNumPieces);
    } else {
        auto ds = getDataSetMeta(filename);
        setChoices(ds);
        maxNumPieces = ds.pieces;

        setTimesteps(0);

        if (readPieces)
            setPartitions(maxNumPieces);
        else
            setPartitions(0);
    }

    return true;
}

bool ReadVtk::prepareRead()
{
   if (!m_d)
       m_d = new ReadVtkData;
   m_d->times.clear();
   m_d->timesteps.clear();

   const std::string filename = m_filename->getValue();
   const bool readPieces = m_readPieces->getValue();

   if (boost::algorithm::ends_with(filename, ".pvd")) {

#ifndef HAVE_TINYXML2
       sendError("not compiled against TinyXML2 - no support for parsing .pvd files");
#endif

       m_d->timesteps = readPvd(filename, readPieces);
       if (m_d->timesteps.empty()) {
           delete m_d;
           m_d = nullptr;
           return false;
       }

       for (auto p: m_d->timesteps) {
           if (p.first == ConstantTime)
               continue;
           m_d->times.push_back(p.first);
       }
   }

   return true;
}

bool ReadVtk::finishRead() {
    delete m_d;
    m_d = nullptr;

    return true;
}

bool ReadVtk::read(Reader::Token &token, int timestep, int block)
{
    const bool readPieces = m_readPieces->getValue();
    const bool ghostCells = m_ghostCells->getValue();

    Meta m;
    m.setBlock(block);
    m.setNumBlocks(token.meta().numBlocks());
    m.setTimeStep(timestep);
    m.setNumTimesteps(token.meta().numTimesteps());

    if (m_d->timesteps.empty()) {
        const std::string filename = m_filename->getValue();
        if (readPieces) {
            return load(token, filename, m, block, ghostCells);
        } else {
            return load(token, filename);
        }
    } else {
        bool constant = true;
        double t = ConstantTime;
        if (timestep >= 0) {
            if (timestep >= m_d->times.size())
                return false;

            constant = false;
            t = m_d->times[timestep];
        }

        std::cerr << "Reading t=" << timestep << " (#=" << token.meta().numTimesteps() << ") , block=" << block << " (#=" << token.meta().numBlocks() << ")" << std::endl;

        auto it = m_d->timesteps.find(t);
        if (it != m_d->timesteps.end()) {
            int b = 0;
            for (auto f: it->second) {

                if (!constant)
                    m.setRealTime(f.realtime);

                if (b <= block && block < b+f.pieces) {
                    if (!load(token, f.filename, m, readPieces ? block-b : -1, ghostCells, f.part))
                        return false;
                }

                b += f.pieces;
            }
        }
    }

    return true;
}

#if 0
bool ReadVtk::changeParameter(const vistle::Parameter *p) {
   if (p == m_filename) {
      const std::string filename = m_filename->getValue();
      if (boost::algorithm::ends_with(filename, ".pvd")) {
          auto files = readPvd(m_filename->getValue());
          if (!files.empty()) {
              auto firstts = files.begin()->second;
              if (!firstts.empty()) {
                  setChoices(getDataSet(firstts[0].filename).first);
              }
          }
      } else {
          setChoices(getDataSet(filename).first);
      }
   }

   return Module::changeParameter(p);
}
#endif

bool ReadVtk::load(Token &token, const std::string &filename, const Meta &meta, int piece, bool ghost, const std::string &part) {

   auto ds_pieces = getDataSet(filename, piece, ghost);
   auto dobj = ds_pieces.dataset;
   if (!dobj) {
       sendError("could not read data set '%s'", filename.c_str());
       return false;
   }

   auto grid = vistle::vtk::toGrid(dobj, checkConvexity());
   if (grid) {
       grid->updateInternals();
       grid->setMeta(meta);
       if (!part.empty())
           grid->addAttribute("_part", part);
   }
   token.addObject("grid_out", grid);

   vtkFieldData *fieldData = dobj->GetFieldData();
   vtkDataSetAttributes *pointData = nullptr;
   vtkDataSetAttributes *cellData = nullptr;
   if (auto ds = vtkDataSet::SafeDownCast(dobj)) {
       pointData = ds->GetPointData();
       cellData = ds->GetCellData();
   }
   for (int i=0; i<NumPorts; ++i) {
       if (cellData && m_cellDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(cellData, m_cellDataChoice[i]->getValue(), grid);
           if (field) {
               field->setMapping(DataBase::Element);
               field->setMeta(meta);
               if (!part.empty())
                   field->addAttribute("_part", part);
           }
           token.addObject(m_cellPort[i], field);
       }

       if (pointData && m_pointDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(pointData, m_pointDataChoice[i]->getValue(), grid);
           if (!field) {
               field = vistle::vtk::getField(fieldData, m_pointDataChoice[i]->getValue(), grid);
           }
           if (field) {
               field->setMeta(meta);
               field->setMapping(DataBase::Vertex);
               if (!part.empty())
                   field->addAttribute("_part", part);
           }
           token.addObject(m_pointPort[i], field);
       }

   }

   return true;
}

#if 0
bool ReadVtk::compute() {

   const std::string filename = m_filename->getValue();
   const bool readPieces = m_readPieces->getValue();
   const bool ghostCells = m_ghostCells->getValue();

   if (boost::algorithm::ends_with(filename, ".pvd")) {

#ifndef HAVE_TINYXML2
       sendError("not compiled against TinyXML2 - no support for parsing .pvd files");
#endif

       auto files = readPvd(filename, readPieces);
       int time = 0;
       for (auto timestep: files) {
           int numBlocks=0;
           for (auto file: timestep.second) {
               if (readPieces)
                   numBlocks += file.pieces;
               else
                   ++numBlocks;
           }
           int block=0;
           for (auto file: timestep.second) {
               for (int piece=0; piece<file.pieces; ++piece) {
                   if (cancelRequested()) {
                       break;
                   }
                   Meta meta;
                   meta.setBlock(block);
                   meta.setNumBlocks(numBlocks);
                   meta.setTimeStep(time);
                   meta.setNumTimesteps(files.size());
                   meta.setRealTime(file.realtime);
                   if (block%size() == rank())
                       load(file.filename, meta, readPieces ? piece : -1, ghostCells, file.part);
                   ++block;
               }
               if (cancelRequested())
                   break;
           }
           ++time;
           if (cancelRequested())
               break;
       }
       return true;
   }

   if (readPieces) {

       auto numPieces = getDataSet(filename).second;
       for (int piece=0; piece<numPieces; ++piece) {
           if (cancelRequested()) {
               break;
           }
           Meta meta;
           meta.setBlock(piece);
           meta.setNumBlocks(numPieces);
           if (piece%size() == rank())
               load(filename, meta, piece, ghostCells);
       }
       return true;
   }

   if (rank() == 0)
       load(filename);

   return true;
}
#endif
