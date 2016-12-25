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

MODULE_MAIN(ReadVtk)

using namespace vistle;

const std::string Invalid("(NONE)");

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

bool ReadVtk::parameterChanged(const vistle::Parameter *p) {
   if (p == m_filename) {
      setChoices(getDataSet(m_filename->getValue()));
   }

   return Module::parameterChanged(p);
}

bool ReadVtk::compute() {

   auto ds = getDataSet(m_filename->getValue());
   if (!ds) {
       sendError("could not read data set '%s'", m_filename->getValue().c_str());
       return true;
   }

   auto grid = vistle::vtk::toGrid(ds);
   addObject("grid_out", grid);

   vtkFieldData *fieldData = ds->GetFieldData();
   vtkDataSetAttributes *pointData = ds->GetPointData();
   vtkDataSetAttributes *cellData = ds->GetCellData();
   for (int i=0; i<NumPorts; ++i) {
       if (m_cellDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(cellData, m_cellDataChoice[i]->getValue(), grid);
           if (field)
               field->setMapping(DataBase::Element);
           addObject(m_cellPort[i], field);
       }

       if (m_pointDataChoice[i]->getValue() != Invalid) {
           auto field = vistle::vtk::getField(pointData, m_pointDataChoice[i]->getValue(), grid);
           if (!field) {
               field = vistle::vtk::getField(fieldData, m_pointDataChoice[i]->getValue(), grid);
           }
           if (field)
               field->setMapping(DataBase::Vertex);
           addObject(m_pointPort[i], field);
       }

   }

   return true;
}
