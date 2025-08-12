#include "ReadSubzoneTecplot.h"
#include "topo.h"
#include "mesh.h"
#include "sztecplotfile.h"

#include <boost/algorithm/string/predicate.hpp>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>

#include <vistle/util/filesystem.h>

//Include TecIO for szPLot:
//extern "C" {
#if !defined TECIOMPI
#include <TecioPLT.h>
#endif
#include <TECIO.h>
//#include <TecioSZL.h>
//}
#if defined TECIOMPI
#include <mpi.h>
#endif
//#include <qt6/QtCore/qcborvalue.h>

using namespace vistle;
//using namespace tecplot::tecioszl;
using vistle::Scalar;
using vistle::Vec;
using vistle::Index;

MODULE_MAIN(ReadSubzoneTecplot)

bool ReadSubzoneTecplot::examine(const vistle::Parameter *param)
{
    if (param != nullptr && param != m_filename)
        return true;

    const std::string filename = m_filename->getValue();
    try {
        //get File Handle
        tecFileReaderOpen(filename.c_str(), &fileHandle);
        char *dataSetTitle = NULL;
        tecDataSetGetTitle(fileHandle, &dataSetTitle);

        // get number of variables
        int32_t NumVar = 0;
        tecDataSetGetNumVars(fileHandle, &NumVar);

        //TODO: output that 1, 2, 3 variables are read as x, y, z coordinates
        // second step: make it possible to search for the position of the x, y, z coordinates
        std::ostringstream outputStream;
        for (int32_t var = 1; var <= NumVar; ++var) {
            char *name = NULL;
            tecVarGetName(fileHandle, var, &name);
            outputStream << name;
            if (var < NumVar)
                outputStream << ',';
            tecStringFree(&name);
        }
        std::cerr << "variables: " << outputStream.str() << std::endl;

        // Check file type
        // 0 = contains grid and solution, 1 = grid only, 2 = solution only
        int32_t fileType;
        // TODO: return error if it contains no grid
        tecFileGetType(fileHandle, &fileType);

        int32_t numZones = 0;
        tecDataSetGetNumZones(fileHandle, &numZones);
        //setPartitions(std::min(size_t(size() * 32), numZones));
        setPartitions(numZones);
        //setNumPartitions(numZones);

        // TODO: output warning that code is not defined for  geometries
        // check geometries in data:
        int32_t numGeoms = 0;
        tecGeomGetNumGeoms(fileHandle, &numGeoms);

        int32_t zoneType;
        // Check zone type for each zone
        // 0 = ordered, 1 = line segment, 2 = triangle, 3 = quadrilateral, 4 = tetraheddron, 5 = brick,
        // 6 = polygon, 7 = polyhedron, 8  = Mixed(not used)
        // TODO: implement function to check zone type for a specified zone and puts out a understandable string
        for (int32_t inputZone = 1; inputZone <= numZones; ++inputZone) {
            // Retrieve zone characteristics
            //int32_t zoneType;
            tecZoneGetType(fileHandle, inputZone, &zoneType);
        }

        if (fileType != 0) {
            std::cerr << "Tecplot Module is just defined for structured grids " << std::endl;
            return false;
        }
    } catch (const std::exception &e) {
        std::cerr << "failed to read number of Variables and Zones of " << filename << ": " << e.what() << '\n';
        setPartitions(0);
        return false;
    }
    return true;
}


template<typename T>
Vec<Scalar, 1>::ptr ReadSubzoneTecplot::readVariables(void *fileHandle, int32_t numValues, int32_t inputZone,
                                                      int32_t var)
{
    //template<typename T>
    //void ReadSubzoneTecplot::readVariables(std::vector<T> &values, void* fileHandle, int32_t numValues, int32_t inputZone, int32_t var) {
    std::vector<T> values(numValues, T()); // Initialize with default values
    // Read and write var data with specified chunk size
    //for (size_t i = 0; i < numValues; i++)
    //{
    //t64_t numValuesToRead = std::min(numValuesPerRead, numValues - numValuesRead);
    int32_t varType;
    tecZoneVarGetType(fileHandle, inputZone, var, &varType);
    Vec<Scalar, 1>::ptr field(new Vec<Scalar, 1>(numValues));
    switch ((FieldDataType_e)varType) {
    case FieldDataType_Float: {
        //std::vector<float> values(numValuesPerRead);

        tecZoneVarGetFloatValues(fileHandle, inputZone, var, 1, numValues, &values[0]);
        for (int64_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Double: {
        //std::vector<double> values(numValuesPerRead);
        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, 1, numValues, &values[0]);
    } break;
    case FieldDataType_Int32: {
        //std::vector<int32_t> values(numValuesPerRead);
        tecZoneVarGetInt32Values(fileHandle, inputZone, var, 1, numValues, &values[0]);
    } break;
    case FieldDataType_Int16: {
        //std::vector<int16_t> values(numValuesPerRead);
        tecZoneVarGetInt16Values(fileHandle, inputZone, var, 1, numValues, &values[0]);
    } break;
    case FieldDataType_Byte: {
        //std::vector<uint8_t> values(numValuesPerRead);
        tecZoneVarGetUInt8Values(fileHandle, inputZone, var, 1, numValues, &values[0]);
    }
    } // close switch

    return field;
}

//template<typename T>
StructuredGrid::ptr ReadSubzoneTecplot::createStructuredGrid(void *fileHandle, int32_t inputZone)
{
    // Define grid:
    // get size x, y, z of all zones
    int64_t m_numVert_x = 0;
    int64_t m_numVert_y = 0;
    int64_t m_numVert_z = 0;
    int32_t numZones = 0;
    tecDataSetGetNumZones(fileHandle, &numZones);

    tecZoneGetIJK(fileHandle, inputZone, &m_numVert_x, &m_numVert_y, &m_numVert_z);
    StructuredGrid::ptr str_grid = std::make_shared<StructuredGrid>(m_numVert_x, m_numVert_y, m_numVert_z);

    auto &xCoords = str_grid->x();
    auto &yCoords = str_grid->y();
    auto &zCoords = str_grid->z();

    int32_t startIndex = 1; // TODO: is start index really 1?

    std::cout << "current zone : " << inputZone<< " " << std::endl;
    std::cout << "size of current zone: " << yCoords.size() << " " << std::endl;


    // TODO: change for more than structured gird (zoneType=0) and do a for loop over zones
    // PROBLEM: resizing defined grid is not possible and yields to segmentation fault
    // Solution: crerate new grid for every zone OR evaluate the size of the grid before reading the variables
    // Read the number of values for each variable
    int64_t numValues;
    int32_t var = 1; //TODO:cs for-loop over variables
    tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);
    // Read the coordinates
    std::vector<double> result;
    //ReadSubzoneTecplot::readVariables(result, fileHandle, numValues, inputZone, var);
    // TODO: modify other cases so that they work like the float case and refactor
    int32_t varType;
    tecZoneVarGetType(fileHandle, inputZone, var, &varType);
    switch ((FieldDataType_e)varType) {
    case FieldDataType_Float: {
        std::vector<float> values(numValues);
        int32_t var = 1;
        //File Handle, Zone Index, // Variable Index, Start Index,
        // Number of Values
        tecZoneVarGetFloatValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        xCoords.resize(values.size());
        //xCoords.resize(xCoords.size() + values.size());
        //std::copy(values.begin(), values.end(), xCoords.begin() + xlastNumValues);
        std::copy(values.begin(), values.end(), xCoords.begin());
        var = 2;
        tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);
        tecZoneVarGetFloatValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        yCoords.resize(values.size());
        std::copy(values.begin(), values.end(), yCoords.begin());
        var = 3;
        tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);
        tecZoneVarGetFloatValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        zCoords.resize(values.size());
        std::copy(values.begin(), values.end(), zCoords.begin());
        // Show test values from x andd x and y variables
        for (int i = 0; i < 1; ++i) {
            std::cout << "x size: " << xCoords.size() << " " << std::endl;
            std::cout << "y size: " << yCoords.size() << " " << std::endl;
            std::cout << "z size: " << zCoords.size() << " " << std::endl;
        }
    } break;
    case FieldDataType_Double: {
        std::vector<double> values(numValues);
        var = 1;
        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        //ptrXcoords = values.data();
        var = 2;
        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        //ptrYcoords = values.data();
        var = 3;
        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
        //ptrZcoords = values.data();
    } break;
    case FieldDataType_Int32: {
        std::vector<int32_t> values(numValues);
        tecZoneVarGetInt32Values(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
    } break;
    case FieldDataType_Int16: {
        std::vector<int16_t> values(numValues);
        tecZoneVarGetInt16Values(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
    } break;
    case FieldDataType_Byte: {
        std::vector<uint8_t> values(numValues);
        tecZoneVarGetUInt8Values(fileHandle, inputZone, var, startIndex, numValues, &values[0]);
    }
    } // close switch

    return str_grid;
}

bool ReadSubzoneTecplot::read(Reader::Token &token, int timestep, int block)
{
    // Read grids of all zones:
    int32_t numZones = 0;
    tecDataSetGetNumZones(fileHandle, &numZones);


    StructuredGrid::ptr strGrid = NULL;
    strGrid = ReadSubzoneTecplot::createStructuredGrid(fileHandle, block + 1);
    auto solutionTime = 0.0;
    tecZoneGetSolutionTime(fileHandle, block + 1, &solutionTime);
    strGrid->setTimestep(timestep);
    std::cout << "reading zone number " << block + 1 << " of " << numZones << " zones" << std::endl;
    std::cout << "timestep: " << timestep << std::endl;
    std::cout << "solution time: " << solutionTime << std::endl;
    token.applyMeta(strGrid);
    token.addObject(m_grid, strGrid);


    return true;

    // TODO: define function that gives understandable strings for tecFileType

    // Drittelmodel variables: "CoordinateX,CoordinateY,CoordinateZ, (vertex coordinates)
    // VelocityX,VelocityY,VelocityZ,Density,Iblank,Lambda2 (cell-centered variables)
    // Drittemodel variables surface: "CoordinateX,CoordinateY,CoordinateZ,VelocityX,
    // VelocityY,VelocityZ,Density,Iblank,CoefPressure"

    // TODO: iterate over zones
    int32_t zone = 1; // TODO: change for more than structured gird (zoneType=0) and do a for loop over zones
    // check if solution is included in the file
    int32_t fileType;
    if (tecFileGetType(fileHandle, &fileType) != 1) {
        int32_t NumVar = 0;
        tecDataSetGetNumVars(fileHandle, &NumVar);
        for (int32_t var = 4; var < NumVar; var++) { // start with 4, because first 3 are coordinates
            char *varName = NULL;
            tecVarGetName(fileHandle, var, &varName);
            int64_t numValues;
            tecZoneVarGetNumValues(fileHandle, zone, var, &numValues);
            std::vector<float> values(numValues);


            Vec<Scalar, 1>::ptr field(new Vec<Scalar, 1>(numValues));
            //std::shared_ptr<vistle::Object> field; //(new Vec<Scalar, 1>(numValues));
            // TODO: update readVariables method to output a Vec<Scalar, 1> object
            //Vec<Scalar, 1>::ptr field = readVariables(fileHandle, numValues, zone, var);
            auto inputZone = 1; // TODO: change for more than structured gird (zoneType=0) and do a for loop over zones
            tecZoneVarGetFloatValues(fileHandle, inputZone, var, 1, numValues, &values[0]);
            for (int64_t i = 0; i < 3; i++) {
                //field->x()[i] = values[i];
                std::cout << varName << ": " << values[i] << std::endl;
            } /*
            //std::copy(values.begin(), values.end(), &field);

            //bool isNotEmpty = !values.empty();
            if (field) {
                field->addAttribute(vistle::attribute::Species, varName);
                // map to the grid cell, because FLOWer data is cell-centered, though the coordinate system is vertex-centered
                field->setMapping(vistle::DataBase::Element);
                field->setGrid(strGrid); // gird yields assertion error, because it has dimensions 168*128*1

                token.applyMeta(field);
                token.addObject(m_fieldsOut[var], field);
            } */
        }
    }

    return true;
}

ReadSubzoneTecplot::ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    m_filename = addStringParameter("filename", "name of szTecplot file", "", vistle::Parameter::ExistingFilename);

    m_grid = createOutputPort("grid_out", "grid or geometry");
    //m_p = createOutputPort("p", "pressure");
    //m_rho = createOutputPort("rho", "rho");
    //m_n = createOutputPort("n", "n");
    //m_u = createOutputPort("u", "u");
    //m_v = createOutputPort("v", "v");

    setParallelizationMode(Serial);
    //setParallelizationMode(ParallelizeTimeAndBlocks); // Parallelization does not work, because it distortes the grid structure


    /* 
    for (int i = 0; i < NumPorts; i++) {
        std::stringstream choiceFieldName;
        choiceFieldName << "tecplotfield_" << i;

        m_fieldChoice[i] = addStringParameter(
            "tecplotfield_" + std::to_string(i),
            "This data field from the tecplot file will be added to output port field_out_" + std::to_string(i) + ".",
            "", Parameter::Choice);
        m_fieldsOut[i] = createOutputPort("field_out_" + std::to_string(i), "data field");

    } */

    observeParameter(m_filename);
    // TODO: close file after reading, when?
    // tecFileReaderClose(&fileHandle);
}

ReadSubzoneTecplot::~ReadSubzoneTecplot()
{}
