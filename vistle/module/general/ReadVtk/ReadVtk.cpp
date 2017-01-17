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


struct VtkFile {
    std::string filename;
    int part = -1;
    double realtime = 0.;
};


std::map<double, std::vector<VtkFile>> readPvd(const std::string &filename) {

    std::map<double, std::vector<VtkFile>> files;

#ifdef HAVE_TINYXML2
    vistle::filesystem::path pvdpath(filename);
    if (!vistle::filesystem::is_regular_file(pvdpath)) {
        std::cerr << "readPvd: " << filename << " is not a regular file" << std::endl;
        return files;
    }
    xml::XMLDocument doc;
    doc.LoadFile(filename.c_str());
    auto VTKFile = doc.FirstChildElement("VTKFile");
    if (!VTKFile) {
        std::cerr << "readPvd: did not find VTKFile element" << std::endl;
        return files;
    }
    const char *type = VTKFile->Attribute("type");
    if (!type || std::string(type) != "Collection") {
        std::cerr << "readPvd: VTKFile not of required type 'Collection'" << std::endl;
        return files;
    }

    auto Collection = VTKFile->FirstChildElement("Collection");
    if (!Collection) {
        std::cerr << "readPvd: did not find Collection element" << std::endl;
        return files;
    }
    auto DataSet = Collection->FirstChildElement("DataSet");
    if (!DataSet) {
        std::cerr << "readPvd: did not find a DataSet element" << std::endl;
        return files;
    }

    int numBlocks = 0;
    while (DataSet) {
        double time = 0.;
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
            vtkFile.part = p;
            vtkFile.realtime = time;
            auto &timestep = files[time];
            timestep.push_back(vtkFile);
            ++numBlocks;
        }

        DataSet = DataSet->NextSiblingElement("DataSet");
    }
#endif

    //std::cerr << "readPvd(" << filename << "): num timesteps: " << files.size() << ", num blocks: " << numBlocks <<  std::endl;

    return files;
}


template<class Reader>
vtkDataSet *readFile(const std::string &filename) {
   if (!vistle::filesystem::is_regular_file(filename)) {
       return nullptr;
   }
   vtkSmartPointer<Reader> reader = vtkSmartPointer<Reader>::New();
   reader->SetFileName(filename.c_str());
   reader->Update();
   if (reader->GetOutput())
      reader->GetOutput()->Register(reader);
   return vtkDataSet::SafeDownCast(reader->GetOutput());
}

vtkDataSet *getDataSet(const std::string &filename) {
   vtkDataSet *ds = nullptr;
   bool triedLegacy = false;
   if (boost::algorithm::ends_with(filename, ".vtk")) {
      ds = readFile<vtkDataSetReader>(filename);
      triedLegacy = true;
   }
   if (!ds) {
      ds = readFile<vtkXMLGenericDataObjectReader>(filename);
   }
   if (!triedLegacy && !ds) {
      ds = readFile<vtkDataSetReader>(filename);
   }
   return ds;
}

std::vector<std::string> getFields(vtkFieldData *dsa)
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

void ReadVtk::setChoices(vtkDataSet *ds) {

    std::vector<std::string> cellFields({Invalid});
    if (ds) {
        auto append = getFields(ds->GetCellData());
        std::copy(append.begin(), append.end(), std::back_inserter(cellFields));
    }
    for (int i=0; i<NumPorts; ++i)
    {
        setParameterChoices(m_cellDataChoice[i], cellFields);
    }
    std::vector<std::string> pointFields({Invalid});
    std::vector<std::string> append;
    if (ds && ds->GetPointData())
        append = getFields(ds->GetPointData());
    else if (ds && ds->GetFieldData())
        append = getFields(ds->GetFieldData());
    std::copy(append.begin(), append.end(), std::back_inserter(pointFields));
    for (int i=0; i<NumPorts; ++i)
    {
        setParameterChoices(m_pointDataChoice[i], pointFields);
    }
}

ReadVtk::ReadVtk(const std::string &shmname, const std::string &name, int moduleID)
   : Module("ReadVtk", shmname, name, moduleID)
{

   createOutputPort("grid_out");
   m_filename = addStringParameter("filename", "name of VTK file", "");

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
}

ReadVtk::~ReadVtk() {

}

bool ReadVtk::changeParameter(const vistle::Parameter *p) {
   if (p == m_filename) {
      const std::string filename = m_filename->getValue();
      if (boost::algorithm::ends_with(filename, ".pvd")) {
          auto files = readPvd(m_filename->getValue());
          if (!files.empty()) {
              auto firstts = files.begin()->second;
              if (!firstts.empty()) {
                  setChoices(getDataSet(firstts[0].filename));
              }
          }
      } else {
          setChoices(getDataSet(filename));
      }
   }

   return Module::changeParameter(p);
}

bool ReadVtk::load(const std::string &filename, const Meta &meta) {

   auto ds = getDataSet(filename);
   if (!ds) {
       sendError("could not read data set '%s'", filename.c_str());
       return true;
   }

   auto grid = vistle::vtk::toGrid(ds);
   if (grid) {
       grid->setMeta(meta);
   }
   addObject("grid_out", grid);

   vtkFieldData *fieldData = ds->GetFieldData();
   vtkDataSetAttributes *pointData = ds->GetPointData();
   vtkDataSetAttributes *cellData = ds->GetCellData();
   for (int i=0; i<NumPorts; ++i) {
       if (m_cellDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(cellData, m_cellDataChoice[i]->getValue(), grid);
           if (field) {
               field->setMapping(DataBase::Element);
               field->setMeta(meta);
           }
           addObject(m_cellPort[i], field);
       }

       if (m_pointDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(pointData, m_pointDataChoice[i]->getValue(), grid);
           if (!field) {
               field = vistle::vtk::getField(fieldData, m_pointDataChoice[i]->getValue(), grid);
           }
           if (field) {
               field->setMeta(meta);
               field->setMapping(DataBase::Vertex);
           }
           addObject(m_pointPort[i], field);
       }

   }

   return true;
}

bool ReadVtk::compute() {

   const std::string filename = m_filename->getValue();

   if (boost::algorithm::ends_with(filename, ".pvd")) {

#ifndef HAVE_TINYXML2
       sendError("not compiled against TinyXML2 - no support for parsing .pvd files");
#endif

       auto files = readPvd(m_filename->getValue());
       int time = 0;
       for (auto timestep: files) {
           int block=0;
           for (auto file: timestep.second) {
               Meta meta;
               meta.setBlock(file.part);
               meta.setBlock(block);
               meta.setNumBlocks(timestep.second.size());
               meta.setTimeStep(time);
               meta.setNumTimesteps(files.size());
               meta.setRealTime(file.realtime);
               if (block%size() == rank())
                   load(file.filename, meta);
               ++block;
           }
           ++time;
       }
       return true;
   }

   return load(filename);
}
