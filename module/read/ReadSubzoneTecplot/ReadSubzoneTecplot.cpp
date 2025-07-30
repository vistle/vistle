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
#include "../../../lib/3rdparty/tecio/teciosrc/TecioPLT.h"
#include "TECIO.h"
//#include <TecioSZL.h>
//}
#if defined TECIOMPI
    #include "mpi.h"
#endif

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
    try
    {
        int32_t NumVar = 0;
        //get File Handle 
        tecFileReaderOpen(filename.c_str(), &fileHandle); 
        char* dataSetTitle = NULL;
        tecDataSetGetTitle(fileHandle, &dataSetTitle);

        //
        tecDataSetGetNumVars(fileHandle,  &NumVar);

        std::ostringstream outputStream;
        for (int32_t var = 1; var <= NumVar; ++var)
            {
                char* name = NULL;
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
        tecFileGetType(fileHandle, &fileType);
        
        int32_t numZones = 0;
        tecDataSetGetNumZones(fileHandle, &numZones);
        //setPartitions(std::min(size_t(size() * 32), numZones));
        setPartitions(numZones);
        
        int32_t zoneType;
        // Check zone type for each zone
        // 0 = ordered, 1 = line segment, 2 = triangle, 3 = quadrilateral, 4 = tetraheddron, 5 = brick,
        // 6 = polygon, 7 = polyhedron, 8  = Mixed(not used)
        // TODO: implement function to check zone type for a specified zone
        for (int32_t inputZone = 1; inputZone <= numZones; ++inputZone)
        {
            // Retrieve zone characteristics
            //int32_t zoneType;
            tecZoneGetType(fileHandle, inputZone, &zoneType);
        }

        if(fileType!=0)
        {
            std::cerr << "Tecplot Module is just defined for structured grids " << std::endl;
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "failed to read number of Variables and Zones of " << filename << ": " << e.what() << '\n';
        setPartitions(0);
        return false;
    }
    return true;
}


//template<typename T>
//std::vector<T> ReadSubzoneTecplot::readVariables(void* fileHandle, int32_t numValues, int32_t inputZone, int32_t var) {
//template<typename T>
//void ReadSubzoneTecplot::readVariables(std::vector<T> &values, void* fileHandle, int32_t numValues, int32_t inputZone, int32_t var) {
    /* values.resize(numValues);

    // Example: fill with default values or your reading logic
    int64_t numValuesRead = 0;
    // Read and write var data with specified chunk size
    //for (size_t i = 0; i < numValues; i++)
    //{
        //t64_t numValuesToRead = std::min(numValuesPerRead, numValues - numValuesRead);
        int32_t varType;
        tecZoneVarGetType(fileHandle, inputZone, var, &varType);
        switch ((FieldDataType_e)varType) {
        case FieldDataType_Float: {
            //std::vector<float> values(numValuesPerRead);
            tecZoneVarGetFloatValues(fileHandle, inputZone, var, 0, numValues,
                                         &values[0]);
        } break;
        case FieldDataType_Double: {
            //std::vector<double> values(numValuesPerRead);
            tecZoneVarGetDoubleValues(fileHandle, inputZone, var, 0, numValues,
                                          &values[0]);
        } break;
        case FieldDataType_Int32: {
            //std::vector<int32_t> values(numValuesPerRead);
            tecZoneVarGetInt32Values(fileHandle, inputZone, var, 0, numValues,
                                         &values[0]);
        } break;
        case FieldDataType_Int16: {
            //std::vector<int16_t> values(numValuesPerRead);
            tecZoneVarGetInt16Values(fileHandle, inputZone, var, 0, numValues,
                                         &values[0]);
        } break;
        case FieldDataType_Byte: {
            //std::vector<uint8_t> values(numValuesPerRead);
            tecZoneVarGetUInt8Values(fileHandle, inputZone, var, 0, numValues,
                                         &values[0]);
        }
        } // close switch */
    //}

    //return values;
//}

//template<typename T>
StructuredGrid::ptr ReadSubzoneTecplot::createStructuredGrid(void* fileHandle, int32_t inputZone) {
    int64_t m_numVert_x = 0;
    int64_t m_numVert_y = 0;
    int64_t m_numVert_z = 0;
    tecZoneGetIJK(fileHandle, inputZone, &m_numVert_x, &m_numVert_y, &m_numVert_z);
    StructuredGrid::ptr str_grid = std::make_shared<StructuredGrid>(m_numVert_x, m_numVert_y, m_numVert_z);

    auto ptrXcoords = str_grid->x().data();
    auto ptrYcoords = str_grid->y().data();
    auto ptrZcoords = str_grid->z().data();

    int64_t numValues;
    int32_t var = 1; //TODO:cs for-loop over variables
    tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);

    // for (int64_t k = 0; k < m_numVert_z; k++)
    // {
    //     for (int64_t j = 0; j < m_numVert_y; j++)
    //     {
    //         for (int64_t i = 0; i < m_numVert_x; i++)
    //         {

                // formula: C/C++ (zero-based): I-1 + (J-1) * IMax + (K-1) * IMax * JMax
                //size_t n = i + j * m_numVert_x + k * m_numVert_x * m_numVert_y;
                std::vector<double> result;
                //ReadSubzoneTecplot::readVariables(result, fileHandle, numValues, inputZone, var);

                int64_t numValuesRead = 0;
            
                int32_t varType;
                tecZoneVarGetType(fileHandle, inputZone, var, &varType);
                switch ((FieldDataType_e)varType) {
                    case FieldDataType_Float: {
                        std::vector<float> values(numValues);
                        int32_t var = 1; 
                        tecZoneVarGetFloatValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        ptrXcoords = values.data();
                        var=2;
                        tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);
                        tecZoneVarGetFloatValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        ptrYcoords = values.data();
                        var=3;
                        tecZoneVarGetNumValues(fileHandle, inputZone, var, &numValues);
                        tecZoneVarGetFloatValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        ptrZcoords = values.data();
                    } break;
                    case FieldDataType_Double: {
                        std::vector<double> values(numValues);
                        var=1;
                        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        //ptrXcoords = values.data();
                        var=2;
                        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        //ptrYcoords = values.data();
                        var=3;
                        tecZoneVarGetDoubleValues(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                        //ptrZcoords = values.data();
                    } break;
                    case FieldDataType_Int32: {
                        std::vector<int32_t> values(numValues);
                        tecZoneVarGetInt32Values(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                    } break;
                    case FieldDataType_Int16: {
                        std::vector<int16_t> values(numValues);
                        tecZoneVarGetInt16Values(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                    } break;
                    case FieldDataType_Byte: {
                        std::vector<uint8_t> values(numValues);
                        tecZoneVarGetUInt8Values(fileHandle, inputZone, var, 0, numValues,
                                                    &values[0]);
                    }
                } // close switch
                //ptrXcoords[n] = values[n]; // 0 is X coordinate
                //ptrYcoords[n] = j; // 1 is Y coordinate
                //ptrZcoords[n] = k; // 2 is Z coordinate
/*                 ptrXcoords[i] = tecZoneVarGetNumValues(fileHandle, inputZone, 1, &numValues);
                ptrYcoords[j * m_numVert_x + i] = j;
                ptrZcoords[k * m_numVert_x * m_numVert_y + j * m_numVert_x + i] = k; */
 /*            }
        }
    } */

    return str_grid;
}

bool ReadSubzoneTecplot::read(Reader::Token &token, int timestep, int block)
{
    int32_t inputZone = 1; // TODO: change for more than structured gird (zoneType=0) and do a for loop over zones
    StructuredGrid::ptr str_grid = NULL;
    str_grid = ReadSubzoneTecplot::createStructuredGrid(fileHandle, inputZone);


    token.applyMeta(str_grid);
    token.addObject(m_grid, str_grid);

    return true;
}

ReadSubzoneTecplot::ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    m_filename = addStringParameter("filename", "name of szTecplot file", "", vistle::Parameter::ExistingFilename);

    m_grid = createOutputPort("grid_out", "grid or geometry");
    m_p = createOutputPort("p", "pressure");
    //m_rho = createOutputPort("rho", "rho");
    //m_n = createOutputPort("n", "n");
    //m_u = createOutputPort("u", "u");
    //m_v = createOutputPort("v", "v");

    //setParallelizationMode(Serial);
    setParallelizationMode(ParallelizeTimeAndBlocks);

    observeParameter(m_filename);
}

ReadSubzoneTecplot::~ReadSubzoneTecplot()
{}