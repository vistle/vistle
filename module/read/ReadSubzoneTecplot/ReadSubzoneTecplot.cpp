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

//Include TecIO for szPlot
#if !defined TECIOMPI
#include <TecioPLT.h>
#endif
#include <TECIO.h>
#if defined TECIOMPI
#include <mpi.h>
#endif

using namespace vistle;
namespace fs = vistle::filesystem;
using vistle::Scalar;
using vistle::Vec;
using vistle::Index;

MODULE_MAIN(ReadSubzoneTecplot)


ReadSubzoneTecplot::ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    //m_filename = addStringParameter("filename", "name of szTecplot file", "", vistle::Parameter::ExistingFilename);

    m_filedir =
        addStringParameter("file_dir", "name of szTecplot files directory", "", vistle::Parameter::ExistingDirectory);

    m_grid = createOutputPort("grid_out", "grid or geometry");

    setParallelizationMode(Serial);
    //setParallelizationMode(ParallelizeTimeAndBlocks); // Parallelization does not work, because it distortes the grid structure

    std::vector<std::string> varChoices{Reader::InvalidChoice};
    for (int i = 0; i < NumPorts; i++) {
        std::stringstream choiceFieldName;
        choiceFieldName << "tecplotfield_" << i;


        m_fieldChoice[i] = addStringParameter(
            "tecplotfield_" + std::to_string(i),
            "This data field from the tecplot file will be added to output port field_out_" + std::to_string(i) + ".",
            "", Parameter::Choice);

        m_fieldsOut[i] = createOutputPort("field_out_" + std::to_string(i), "data field");
    }

    observeParameter(m_filedir); // examine method is called when parameter is changed
}

ReadSubzoneTecplot::~ReadSubzoneTecplot()
{
    tecFileReaderClose(&fileHandle);
}

bool ReadSubzoneTecplot::examine(const vistle::Parameter *param)
{
    std::cout << "examine called" << std::endl;
    if (param != nullptr || param == m_filedir) {
        // Check if the directory is valid
        if (!inspectDir()) {
            sendError("Directory %s does not exist or is not valid", m_filedir->getValue().c_str());
            return false;
        }

        const std::string filename = fileList.front().c_str();
        try {
            setTimesteps(numFiles - 1); //-1 because the last step is -1
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
            // comment in for files that have no ordered filenames
            //solutionTimes = orderSolutionTimes(fileList);
            tecFileReaderClose(&fileHandle);
        } catch (const std::exception &e) {
            std::cerr << "failed to read number of Variables and Zones of " << filename << ": " << e.what() << '\n';
            setPartitions(0);
            return false;
        }
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

    std::cout << "current zone : " << inputZone << " " << std::endl;
    std::cout << "size of current zone: " << yCoords.size() << " " << std::endl;
    std::cout << "vertex x: " << m_numVert_x << " " << std::endl;
    std::cout << "vertex y: " << m_numVert_y << " " << std::endl;
    std::cout << "vertex z: " << m_numVert_z << " " << std::endl;

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

template<typename T>
Vec<Scalar, 3>::ptr ReadSubzoneTecplot::combineVarstoOneOutput(std::vector<T> x, std::vector<T> y, std::vector<T> z,
                                                               int32_t numValues)
{ //TODO: set variables that begin the same and end X, Y, Z to a combined field
    Vec<Scalar, 3>::ptr result(new Vec<Scalar, 3>(numValues));
    // Ensure all vectors have the same size
    if (x.size() != y.size() || y.size() != z.size()) {
        throw std::invalid_argument("Input vectors must have the same size");
    }

    std::cout << "add vectors to field" << std::endl;
    for (size_t i = 0; i < x.size(); i++) {
        result->x()[i] = x[i];
        result->y()[i] = y[i];
        result->z()[i] = z[i];
    }
    return result;
}

std::unordered_map<std::string, std::vector<size_t>>
ReadSubzoneTecplot::findSimilarStrings(const std::vector<std::string> &strings)
{
    std::unordered_map<std::string, std::vector<size_t>> prefixMap;

    for (size_t i = 0; i < strings.size(); ++i) {
        if (strings[i].size() > 1) {
            // Remove the last character to create the prefix
            std::string prefix = strings[i].substr(0, strings[i].size() - 1);
            prefixMap[prefix].push_back(i + 3); //First 3 variables are coordinates, so we add 3 to the index
        }
    }

    // Filter out prefixes with only one occurrence
    for (auto it = prefixMap.begin(); it != prefixMap.end();) {
        if (it->second.size() <= 1) {
            it = prefixMap.erase(it); // Remove prefixes with only one match
        } else {
            ++it;
            std::cout << "Added prefix: " << it->first << " with indices: ";
            std::cout << it->second[0] << ", " << it->second[1] << ", " << it->second[2] << std::endl;
        }
    }

    return prefixMap;
}

std::unordered_map<std::string, std::vector<size_t>> ReadSubzoneTecplot::setFieldChoices(void *fileHandle)
{
    std::vector<std::string> choices{Reader::InvalidChoice};
    // get number of variables
    int32_t NumVar = 0;
    tecDataSetGetNumVars(fileHandle, &NumVar);

    //TODO: define choices
    std::ostringstream outputStream;
    for (int32_t var = 4; var <= NumVar; ++var) { // begin with 4 because first 3 are coordinates
        char *name = NULL;
        std::cout << "setFieldChoices: " << var << std::endl;
        tecVarGetName(fileHandle, var, &name);
        std::string nameStr(name);
        choices.push_back(nameStr);
        tecStringFree(&name);
    }

    auto groups = findSimilarStrings(choices);

    for (const auto &group: groups) {
        std::cout << "The following variables are grouped to one output port: ";
        for (size_t index: group.second) {
            std::cout << index << " (" << group.first << ") ";
        }
        std::cout << std::endl;
    }

    // Add the grouped variables to the choices

    for (const auto &group: groups) {
        std::string combinedName = group.first; //choices[group[0]].substr(0, choices[group[0]].size() - 1);
        choices.push_back(combinedName);
    }

    // Set the choices for each output port
    for (int i = 0; i < NumPorts; i++) {
        setParameterChoices(m_fieldChoice[i], choices);
    }

    return groups;
}

// TODO: update getIndexOfTecVar to use the new choices (combined variables)
std::vector<int> ReadSubzoneTecplot::getIndexOfTecVar(
    const std::string &varName, const std::unordered_map<std::string, std::vector<size_t>> &indicesCombinedVariables,
    void *fileHandle) const
{
    std::vector<int> result;
    // get number of variables
    int32_t NumVar = 0;
    tecDataSetGetNumVars(fileHandle, &NumVar);
    std::cout << "getIndexOfTecVar: " << varName << std::endl;
    for (int32_t var = 1; var <= NumVar; ++var) {
        char *name = NULL;
        tecVarGetName(fileHandle, var, &name);
        if (name == nullptr) {
            continue;
        }
        if (varName == name) {
            tecStringFree(&name);
            return result = {var}; // Return the index if the variable name matches
        } else {
            for (const auto &group: indicesCombinedVariables) {
                std::string combinedName = group.first;

                if (combinedName == varName) {
                    // If the variable name matches a combined variable, return the first index in the group
                    tecStringFree(&name);
                    result = {(int)group.second[0], (int)group.second[1],
                              (int)group.second[2]}; // Return the indices of the combined variable
                    std::cout << "Found combined variable: " << combinedName << " with indices: " << group.second[0]
                              << ", " << group.second[1] << ", " << group.second[2] << std::endl;
                    return result;
                }
            }
        }
        tecStringFree(&name);
    }
    result.push_back(-1); // not found
    return result;
}

//checks if choice is a valid input
bool ReadSubzoneTecplot::emptyValue(vistle::StringParameter *ch) const
{
    auto name = ch->getValue();
    return name.empty() || name == Reader::InvalidChoice;
}

bool ReadSubzoneTecplot::inspectDir()
{
    //TODO :: check if file is NC format!
    std::string sFileDir = m_filedir->getValue();

    if (sFileDir.empty()) {
        sendInfo("Szplt filename is empty!");
        return false;
    }

    try {
        fs::path dir(sFileDir);
        fileList.clear();
        numFiles = 0;

        if (fs::is_directory(dir)) {
            sendInfo("Locating files in %s", dir.string().c_str());
            for (fs::directory_iterator it(dir); it != fs::directory_iterator(); ++it) {
                if (fs::is_regular_file(it->path()) && (it->path().extension() == ".szplt")) {
                    // std::string fName = it->path().filename().string();
                    std::string fPath = it->path().string();
                    fileList.push_back(fPath);
                    ++numFiles;
                }
            }
        } else if (fs::is_regular_file(dir)) {
            if (dir.extension() == ".szplt") {
                std::string fName = dir.string();
                sendInfo("Loading file %s", fName.c_str());
                fileList.push_back(fName);
                ++numFiles;
            } else {
                sendError("File does not end with '.szplt' ");
            }
        } else {
            sendInfo("Could not find given directory %s. Please specify a valid path", sFileDir.c_str());
            return false;
        }
    } catch (std::exception &ex) {
        sendError("Could not read %s: %s", sFileDir.c_str(), ex.what());
        return false;
    }

    if (numFiles > 1) {
        std::sort(fileList.begin(), fileList.end(), [](std::string a, std::string b) { return a < b; });
    }
    sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
        return false;
    return true;
}

std::unordered_map<int, double> ReadSubzoneTecplot::orderSolutionTimes(std::vector<std::string> fileList) {
    std::vector<std::pair<double, int>> solutionTimeTimestepPairs;

    // Collect all solution times and their corresponding timesteps
    for (const auto &file : fileList) {
        try {
            tecFileReaderOpen(file.c_str(), &fileHandle);

            int32_t numZones = 0;
            tecDataSetGetNumZones(fileHandle, &numZones);

            for (int32_t zone = 1; zone <= numZones; ++zone) {
                double solutionTime = 0.0;
                tecZoneGetSolutionTime(fileHandle, zone, &solutionTime);
                solutionTimeTimestepPairs.emplace_back(solutionTime, solutionTimeTimestepPairs.size());
            }

            tecFileReaderClose(&fileHandle);
        } catch (const std::exception &e) {
            sendError("Failed to read solution time from file %s: %s", file.c_str(), e.what());
        }
    }

    // Sort and create the mapping in one step
    std::unordered_map<int, double> orderedSolutionTimes;
    std::sort(solutionTimeTimestepPairs.begin(), solutionTimeTimestepPairs.end());

    for (size_t i = 0; i < solutionTimeTimestepPairs.size(); ++i) {
        orderedSolutionTimes[i] = solutionTimeTimestepPairs[i].first;
    }

    return orderedSolutionTimes;
}

int ReadSubzoneTecplot::getTimestepForSolutionTime(std::unordered_map<int, double> &orderedSolutionTimes, double solutionTime) {
    auto it = std::lower_bound(orderedSolutionTimes.begin(), orderedSolutionTimes.end(), solutionTime,
                               [](const std::pair<int, double> &pair, double value) {
                                   return pair.second < value; // Compare solution time
                               });

    if (it != orderedSolutionTimes.end() && it->second == solutionTime) {
        return it->first; // Return the timestep if the solution time matches
    }
    return -1; // Return -1 if the solution time is not found
}

bool ReadSubzoneTecplot::read(Reader::Token &token, int timestep, int block)
{
    // TODO: Update when surface and  valume dat is read at once
    if (timestep < 0 || timestep >= numFiles) {
        std::cout << "Constant timestep: " << timestep << std::endl;
        return true;
    } else {
        std::cout << "Reading timestep: " << timestep << std::endl;
        const std::string &filename = fileList[timestep];
        std::cout << "Using file: " << filename << std::endl;
        try {
            tecFileReaderOpen(filename.c_str(), &fileHandle);
            sendInfo("Reading file %s for timestep %d", filename.c_str(), timestep);
            // Read grids of all zones:
            int32_t numZones = 0;
            tecDataSetGetNumZones(fileHandle, &numZones);
            int32_t zone = block + 1; // zone numbers start with 1, not 0

            StructuredGrid::ptr strGrid = NULL;
            strGrid = ReadSubzoneTecplot::createStructuredGrid(fileHandle, zone);
            auto solutionTime = 0.0;
            tecZoneGetSolutionTime(fileHandle, zone, &solutionTime);
            //int step = getTimestepForSolutionTime(solutionTimes, solutionTime);
            strGrid->setTimestep(timestep);
            strGrid->setMapping(
                vistle::DataBase::Vertex); // set mapping to vertex, because coordinates are vertex-centered
            std::cout << "reading zone number " << zone << " of " << numZones << " zones" << std::endl;
            std::cout << "timestep: " << timestep << std::endl;
            std::cout << "solution time: " << solutionTime << std::endl;
            token.applyMeta(strGrid);
            token.addObject(m_grid, strGrid);

            // TODO: define function that gives understandable strings for tecFileType

            // Drittelmodel variables: "CoordinateX,CoordinateY,CoordinateZ, (vertex coordinates)
            // VelocityX,VelocityY,VelocityZ,Density,Iblank,Lambda2 (cell-centered variables)
            // Drittemodel variables surface: "CoordinateX,CoordinateY,CoordinateZ,VelocityX,
            // VelocityY,VelocityZ,Density,Iblank,CoefPressure"

            //Define options of variable ports
            auto indices = setFieldChoices(fileHandle);

            // TODO: change for more than structured gird (zoneType=0)
            // check if solution is included in the file
            int32_t fileType;
            if (tecFileGetType(fileHandle, &fileType) != 1) {
                int32_t NumVar = 0;
                tecDataSetGetNumVars(fileHandle, &NumVar);
                // TODO: change to output just varChoices if not Invalid and not just as many variables as output ports
                char *varName = NULL;
                int64_t numValues;
                for (int32_t var = 0; var < NumPorts; var++) { // loop over output ports

                    std::string name = m_fieldChoice[var]->getValue();
                    if (!emptyValue(m_fieldChoice[var])) {
                        std::cout << "Reading variable: " << name << " on port: " << var << std::endl;
                        std::vector<int> varInFile = getIndexOfTecVar(name, indices, fileHandle);
                        tecVarGetName(fileHandle, varInFile[0], &varName);

                        if (varInFile.size() == 1) {
                            tecZoneVarGetNumValues(fileHandle, zone, varInFile[0], &numValues);
                            std::vector<float> values(
                                numValues); //TODO: change to template type T (! does not work in read method but in seperate function)
                            int startIndex = 1;
                            // TODO: update readVariables method to output a Vec<Scalar, 1> object
                            //Vec<Scalar, 1>::ptr field = readVariables(fileHandle, numValues, zone, var);
                            tecZoneVarGetFloatValues(fileHandle, zone, varInFile[0], startIndex, numValues, &values[0]);

                            std::cout << "Variable name in file: " << varName << std::endl;
                            // copy data to the output vector
                            Vec<Scalar, 1>::ptr field(new Vec<Scalar, 1>(numValues));
                            for (size_t i = 0; i < numValues; i++) {
                                field->x()[i] = values[i];
                                //std::cout << "field->x()[" << i << "]: " << field->x()[i] << std::endl;
                            }

                            if (field) {
                                field->addAttribute(vistle::attribute::Species, name);
                                // map to the grid cell, because FLOWer data is cell-centered, though the coordinate system is vertex-centered
                                field->setMapping(vistle::DataBase::Element);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                //std::cout << "m_fieldsOut 0: " << m_fieldsOut[0] << std::endl;
                                std::cout << "Accessing m_fieldsOut at index: " << var << std::endl;
                                token.addObject(m_fieldsOut[var], field);
                            }
                            tecStringFree(&varName);
                        } else if (varInFile.size() > 1) {
                            // If the variable is a combined variable, read all indices
                            std::cout << "Reading combined variable: " << name << " on port: " << var << std::endl;
                            tecZoneVarGetNumValues(fileHandle, zone, varInFile[0], &numValues);
                            int startIndex = 1;
                            std::vector<float> xValues(numValues);
                            std::vector<float> yValues(numValues);
                            std::vector<float> zValues(numValues);
                            tecZoneVarGetFloatValues(fileHandle, zone, varInFile[0], startIndex, numValues,
                                                     &xValues[0]);
                            tecZoneVarGetFloatValues(fileHandle, zone, varInFile[1], startIndex, numValues,
                                                     &yValues[0]);
                            tecZoneVarGetFloatValues(fileHandle, zone, varInFile[2], startIndex, numValues,
                                                     &zValues[0]);
                            Vec<Scalar, 3>::ptr field =
                                combineVarstoOneOutput<float>(xValues, yValues, zValues, numValues);
                            if (field) {
                                field->addAttribute(vistle::attribute::Species, name);
                                field->setMapping(vistle::DataBase::Element);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                token.addObject(m_fieldsOut[var], field);
                            }
                        }
                    } else {
                        std::cout << "Skipping variable: " << m_fieldChoice[var]->getValue() << " on position: " << var
                                  << std::endl;
                        continue; // skip if the variable is not selected
                    }
                } //close for loop over ports
                // TODO: change for more than structured gird (zoneType=0)
            } // close if fileType != 1
            else {
                std::cerr << "Tecplot does not contain solution variables but just a grid. " << std::endl;
            }
            tecFileReaderClose(&fileHandle);
        } //close try
        catch (const std::exception &e) {
            sendError("Failed to read file %s: %s", filename.c_str(), e.what());
            return false;
        }
        return true;
    }
}