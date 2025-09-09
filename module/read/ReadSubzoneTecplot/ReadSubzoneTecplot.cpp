#include "ReadSubzoneTecplot.h"

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
#include <boost/mpi/communicator.hpp>

#include <vistle/util/filesystem.h>
#include <vistle/util/stopwatch.h>
#include <mutex>


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


namespace {
static std::mutex g_tecio_mutex; // serialize TecIO open/close calls per process
}


MODULE_MAIN(ReadSubzoneTecplot)

ReadSubzoneTecplot::ReadSubzoneTecplot(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    //m_filename = addStringParameter("filename", "name of szTecplot file", "", vistle::Parameter::ExistingFilename);
    //auto time1 = MPI_Wtime(); // Record start time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at construction: " << time1 << std::endl;

    m_filedir =
        addStringParameter("file_dir", "name of szTecplot files directory", "", vistle::Parameter::ExistingDirectory);

    m_grid = createOutputPort("grid_out", "grid or geometry");

    setParallelizationMode(Serial);
    //setParallelizationMode(ParallelizeTimeAndBlocks); // Parallelization does not work, leads to abortion errors

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
    setParameterRange(m_first, Integer(0), Integer(10000));
    setParameterRange(m_last, Integer(-1), Integer(10000));
    setParameterRange(m_increment, Integer(1), Integer(10000));
    //observeParameter(m_first);
    //observeParameter(m_last);
}

ReadSubzoneTecplot::~ReadSubzoneTecplot() = default;


bool ReadSubzoneTecplot::examine(const vistle::Parameter *param)
{
    StopWatch w("ReadSubzoneTecplot::examine");
    if (!param || param == m_filedir) { // FIX: correct guard
        if (!inspectDir()) {
            sendError("Directory %s does not exist or is not valid", m_filedir->getValue().c_str());
            return false;
        }

        const std::string filename = fileList.front(); // small cleanup
        try {
            // Set timesteps to user input:
            m_fileChoice = setTimestepChoice(numFiles);
            //setTimesteps(numFiles - 1);

            // compute a stable partition count across ALL timesteps ---
            int32_t maxZones = 0;

            for (const auto &f: fileList) {
                void *fh2 = nullptr;
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    /* open */ (void)tecFileReaderOpen(f.c_str(), &fh2);
                }
                if (fh2) {
                    int32_t nz = 0;
                    {
                        std::lock_guard<std::mutex> lk(g_tecio_mutex);
                        tecDataSetGetNumZones(fh2, &nz);
                    }

                    if (nz > maxZones)
                        maxZones = nz;
                    {
                        std::lock_guard<std::mutex> lk(g_tecio_mutex);
                        /* close */ tecFileReaderClose(&fh2);
                    }
                }
            }

            if (maxZones <= 0) { // FIX: guard against bad inputs
                sendError("No zones found in any timestep.");
                return false;
            }
            setPartitions(maxZones);
            setHandlePartitions(PartitionTimesteps);
            setCollectiveIo(CollectiveConstant);
            // ---end of compute

            // Open a LOCAL handle for probing
            void *fh = nullptr;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                if (tecFileReaderOpen(filename.c_str(), &fh) != 0)
                    fh = nullptr;
            }
            if (!fh) {
                sendError("Failed to open base file: %s", filename.c_str());
                return false;
            }


            char *dataSetTitle = nullptr;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecDataSetGetTitle(fh, &dataSetTitle);
            }
            if (dataSetTitle) {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecStringFree(&dataSetTitle);
            }


            // Variables listing (unchanged)
            int32_t NumVar = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecDataSetGetNumVars(fh, &NumVar);
            }

            std::ostringstream outputStream;
            for (int32_t var = 1; var <= NumVar; ++var) {
                char *name = nullptr;
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    tecVarGetName(fh, var, &name);
                }
                if (name) {
                    outputStream << name;
                    {
                        std::lock_guard<std::mutex> lk(g_tecio_mutex);
                        tecStringFree(&name);
                    }
                }
                if (var < NumVar)
                    outputStream << ',';
            }

            std::cerr << "variables: " << outputStream.str() << std::endl;

            // File type
            int32_t fileType = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecFileGetType(fh, &fileType);
            }

            if (fileType == 2) { // solution-only
                std::cerr << "Tecplot file contains no grid, only solution data." << std::endl;
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    tecFileReaderClose(&fh);
                }
                return false;
            }


            int32_t numGeoms = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecGeomGetNumGeoms(fh, &numGeoms);
            }


            int32_t numZones = 0; // <-- declare in the outer scope
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecDataSetGetNumZones(fh, &numZones);
            }

            // now this compiles:
            for (int32_t inputZone = 1; inputZone <= numZones; ++inputZone) {
                int32_t zoneType = -1;
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    tecZoneGetType(fh, inputZone, &zoneType);
                }
                if (zoneType != 0) {
                    std::cerr << "Tecplot module currently supports only structured (ordered) zones." << std::endl;
                    {
                        std::lock_guard<std::mutex> lk(g_tecio_mutex);
                        tecFileReaderClose(&fh);
                    }
                    return false;
                }
            }


            m_indicesCombinedVariables =
                setFieldChoices(fh); // build UI choices once & cache grouped indices (parallel-safe)


            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecFileReaderClose(&fh); // normal close
            }

        } catch (const std::exception &e) {
            std::cerr << "failed to probe " << filename << ": " << e.what() << '\n';
            setPartitions(0);
            return false;
        }
    }
    return true;
}

//TODO: deal with last step -1 case
std::vector<int> ReadSubzoneTecplot::setTimestepChoice(int numFiles)
{
    std::vector<int> indices;

    auto first = m_first->getValue();
    auto last = m_last->getValue();
    auto increment = m_increment->getValue();
    sendInfo("Timestep range selected: first %d, last %d, increment %d", first, last, increment);
    sendInfo("Number of possible timesteps:" + std::to_string(numFiles));

    if (last < numFiles && first <= last && increment > 0) {
        for (int i = first; i < last; i += increment) {
            indices.push_back(i);
            std::cout << "Add file index: " << i << std::endl;
        }
    } else {
        sendError("Invalid timestep range selected. Directory contains " + std::to_string(numFiles) +
                  " files. Resetting to full range.");

        for (int i = 0; i < numFiles; i += 1) {
            indices.push_back(i);
            std::cout << "Add file index: " << i << std::endl;
        }
    }
    setTimesteps(indices.size()); // Set timesteps to number of selected files
    /*     setParameterRange(m_first, Integer(0), Integer(numFiles - 1));
    setParameterRange(m_last, Integer(-1), Integer(numFiles - 1));
    setParameterRange(m_increment, Integer(1), Integer(numFiles - 1)); */

    return indices;
}

bool ReadSubzoneTecplot::prepareRead()
{
    time1 = MPI_Wtime(); // Record start time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at prepareRead: " << time1 << std::endl;
    return true;
}

bool ReadSubzoneTecplot::finishRead()
{
    //double time2 = MPI_Wtime(); // Record end time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at finishRead: " << time2 << std::endl;
    //std::cout << "ReadSubzoneTecplot total time in seconds: " << time2 - time1 << std::endl;
    return true;
}
// Check zone type for each zone
// 0 = ordered, 1 = line segment, 2 = triangle, 3 = quadrilateral, 4 = tetraheddron, 5 = brick,
// 6 = polygon, 7 = polyhedron, 8  = Mixed(not used)
Byte ReadSubzoneTecplot::tecToVistleType(int tecType)
{
    switch (tecType) {
    //case 0:
    //    return StructuredGrid;
    case 1:
        return UnstructuredGrid::BAR;
    case 2:
        return UnstructuredGrid::TRIANGLE;
    case 3:
        return UnstructuredGrid::QUAD;
    case 4:
        return UnstructuredGrid::TETRAHEDRON;
    case 5:
        return UnstructuredGrid::HEXAHEDRON;
    case 6:
        return UnstructuredGrid::POLYGON;
    case 7:
        return UnstructuredGrid::POLYHEDRON;
    default:
        std::stringstream msg;
        msg << "The tec data type with the encoding " << tecType << " is not supported.";

        std::cerr << msg.str() << std::endl;
        throw exception(msg.str());
    }
}

// Helper that reads the values of a variables with index var from a tecplot file with the specified fh for
// the given inputZone and returns a pointer to a Vec<Scalar, 1> containing the values in its x() array.
Vec<Scalar, 1>::ptr ReadSubzoneTecplot::readVariables(void *fh, int64_t numValues, const int32_t inputZone, int32_t var)
{
    int32_t varType;
    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecZoneVarGetType(fh, inputZone, var, &varType);
    }
    Vec<Scalar, 1>::ptr field(new Vec<Scalar, 1>(numValues));
    switch ((FieldDataType_e)varType) {
    case FieldDataType_Float: {
        std::vector<float> values(numValues);
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecZoneVarGetFloatValues(fh, inputZone, var, 1, numValues, values.data());
        }
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Double: {
        std::vector<double> values(numValues);
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecZoneVarGetDoubleValues(fh, inputZone, var, 1, numValues, values.data());
        }
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Int32: {
        std::vector<int32_t> values(numValues);
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecZoneVarGetInt32Values(fh, inputZone, var, 1, numValues, values.data());
        }
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Int16: {
        std::vector<int16_t> values(numValues);
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecZoneVarGetInt16Values(fh, inputZone, var, 1, numValues, values.data());
        }
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Byte: {
        std::vector<uint8_t> values(numValues);
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecZoneVarGetUInt8Values(fh, inputZone, var, 1, numValues, values.data());
        }
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    }
    } // close switch

    return field;
}


// Builds a structured grid from the first three variables in the tecplot file, which are assumed to be the
// coordinates x, y, z. The function reads the number of vertices in each direction and
// the coordinates from the tecplot file, and returns a pointer to a StructuredGrid object
StructuredGrid::ptr ReadSubzoneTecplot::createStructuredGrid(void *fh, int32_t inputZone)
{
    // Define grid:
    // get size x, y, z of all zones
    int64_t m_numVert_x = 0;
    int64_t m_numVert_y = 0;
    int64_t m_numVert_z = 0;
    int32_t numZones = 0;
    tecDataSetGetNumZones(fh, &numZones);

    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecZoneGetIJK(fh, inputZone, &m_numVert_x, &m_numVert_y, &m_numVert_z);
    }
    StructuredGrid::ptr str_grid = std::make_shared<StructuredGrid>(m_numVert_x, m_numVert_y, m_numVert_z);

    auto &xCoords = str_grid->x();
    auto &yCoords = str_grid->y();
    auto &zCoords = str_grid->z();

    int32_t startIndex = 1;

    // TODO: change for more than structured gird (zoneType=0) -> test data needed
    // Read the number of values for each variable

    int64_t n = 0;


    // X
    int64_t numValues = 0;
    int32_t var = 1;
    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    }
    auto fx = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fx->x().begin(), fx->x().end(), xCoords.begin());

    // Y
    var = 2;
    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    }
    auto fy = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fy->x().begin(), fy->x().end(), yCoords.begin());

    // Z
    var = 3;
    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    }
    auto fz = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fz->x().begin(), fz->x().end(), zCoords.begin());


    return str_grid;
}

// Combines the three input vectors x, y, z into a single Vec<Scalar, 3> object.
template<typename T>
Vec<Scalar, 3>::ptr ReadSubzoneTecplot::combineVarstoOneOutput(std::vector<T> x, std::vector<T> y, std::vector<T> z,
                                                               int32_t numValues)
{
    Vec<Scalar, 3>::ptr result(new Vec<Scalar, 3>(numValues));
    // Ensure all vectors have the same size
    if (x.size() != y.size() || y.size() != z.size()) {
        throw std::invalid_argument("Input vectors must have the same size");
    }

    for (size_t i = 0; i < x.size(); i++) {
        result->x()[i] = x[i];
        result->y()[i] = y[i];
        result->z()[i] = z[i];
    }
    return result;
}

// Finds variables with the same name in the variable list and ouputs their indices in a map.
// The map key is the variable name without the last character, and the value is a vector
// containing the indices of the variables that match this prefix.
std::unordered_map<std::string, std::vector<int>>
ReadSubzoneTecplot::findSimilarStrings(const std::vector<std::string> &strings)
{
    std::unordered_map<std::string, std::vector<int>> prefixMap;

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
        }
    }

    return prefixMap;
}

// Sets the variables of the dataset that the user can choose from in the menu.
std::unordered_map<std::string, std::vector<int>> ReadSubzoneTecplot::setFieldChoices(void *fh)
{
    std::vector<std::string> choices{Reader::InvalidChoice};

    int32_t NumVar = 0;
    tecDataSetGetNumVars(fh, &NumVar);

    // 1) Add all scalar variable names (skip coords 1..3) to choices
    std::vector<std::string> varNames(NumVar + 1);
    for (int32_t v = 1; v <= NumVar; ++v) {
        char *nm = nullptr;
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecVarGetName(fh, v, &nm);
        }
        varNames[v] = nm ? std::string(nm) : std::string();
        if (nm) {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecStringFree(&nm);
        }
        if (v >= 4 && !varNames[v].empty()) {
            choices.push_back(varNames[v]);
        }
    }


    // 2) Build combined vector groups by prefix (strip trailing X/Y/Z)
    //    Only add groups where all three components exist.
    struct Triple {
        int ix = -1, iy = -1, iz = -1;
    };
    std::unordered_map<std::string, Triple> triples;

    auto ends_with = [](const std::string &s, char c) {
        return !s.empty() && s.back() == c;
    };

    for (int32_t v = 4; v <= NumVar; ++v) {
        const auto &nm = varNames[v];
        if (nm.size() < 2)
            continue;
        char last = nm.back();
        if (last != 'X' && last != 'Y' && last != 'Z')
            continue;
        std::string pref = nm.substr(0, nm.size() - 1);
        auto &t = triples[pref];
        if (last == 'X')
            t.ix = v;
        if (last == 'Y')
            t.iy = v;
        if (last == 'Z')
            t.iz = v;
    }

    std::unordered_map<std::string, std::vector<int>> groups;
    for (auto &kv: triples) {
        const auto &pref = kv.first;
        const auto &t = kv.second;
        if (t.ix > 0 && t.iy > 0 && t.iz > 0) {
            groups[pref] = {t.ix, t.iy, t.iz};
            choices.push_back(pref); // expose combined vector as a choice
            //std::cout << "Grouped vector: " << pref << " -> {" << t.ix << "," << t.iy << "," << t.iz << "}\n";
        }
    }

    // 3) Set the choices for each output port once
    for (int i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_fieldChoice[i], choices);
    }

    // (optional) keep a copy
    m_varChoices = choices;

    return groups;
}


// Returns the index of the variable with the given name in the tecplot file.
// If the variable is a combined variable, it returns the indices of the combined variable.
// If the variable is not found, it returns a vector with -1.
std::vector<int>
ReadSubzoneTecplot::getIndexOfTecVar(const std::string &varName,
                                     const std::unordered_map<std::string, std::vector<int>> &indicesCombinedVariables,
                                     void *fh) const
{
    std::vector<int> result;
    // get number of variables
    int32_t NumVar = 0;
    {
        std::lock_guard<std::mutex> lk(g_tecio_mutex);
        tecDataSetGetNumVars(fh, &NumVar);
    }

    for (int32_t var = 1; var <= NumVar; ++var) {
        char *name = nullptr;
        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecVarGetName(fh, var, &name);
        }
        if (!name) {
            continue;
        }

        if (varName == name) {
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecStringFree(&name);
            }
            return result = {var};
        } else {
            for (const auto &group: indicesCombinedVariables) {
                std::string combinedName = group.first;
                if (combinedName == varName) {
                    {
                        std::lock_guard<std::mutex> lk(g_tecio_mutex);
                        tecStringFree(&name);
                    }
                    result = {(int)group.second[0], (int)group.second[1], (int)group.second[2]};
                    return result;
                }
            }
        }

        {
            std::lock_guard<std::mutex> lk(g_tecio_mutex);
            tecStringFree(&name);
        }
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
//checks if choice is a valid input
bool ReadSubzoneTecplot::inspectDir()
{
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
            //sendInfo("Locating files in %s", dir.string().c_str());
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
                //sendInfo("Loading file %s", fName.c_str());
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
    //sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
        return false;
    return true;
}

// orderSolutionTimes and getTimestepForSolutionTime can be used to order the solution times in the fileList
// if the times are not given in the file names.
std::unordered_map<int, double> ReadSubzoneTecplot::orderSolutionTimes(std::vector<std::string> fileList)
{
    std::vector<std::pair<double, int>> solutionTimeTimestepPairs;

    // Collect all solution times and their corresponding timesteps
    for (const auto &file: fileList) {
        try {
            void *fh = nullptr;
            tecFileReaderOpen(file.c_str(), &fh);

            int32_t numZones = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecDataSetGetNumZones(fh, &numZones);
            }


            for (int32_t zone = 1; zone <= numZones; ++zone) {
                double solutionTime = 0.0;
                tecZoneGetSolutionTime(fh, zone, &solutionTime);
                solutionTimeTimestepPairs.emplace_back(solutionTime, solutionTimeTimestepPairs.size());
            }

            tecFileReaderClose(&fh);
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

int ReadSubzoneTecplot::getTimestepForSolutionTime(std::unordered_map<int, double> &orderedSolutionTimes,
                                                   double solutionTime)
{
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
    if (timestep < 0 || timestep >= numFiles) {
        //std::cout << "Constant timestep: " << timestep << std::endl;
        return true;
    } else {
        //std::cout << "Reading timestep: " << timestep << std::endl;
        //std::cout << "Size of fileList: " << fileList.size() << std::endl;
        int i = m_fileChoice[timestep];
        const std::string &filename = fileList[i];
        //std::cout << "Using file: " << filename << std::endl;
        try {
            void *fh = nullptr;
            int open_rc = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                open_rc = tecFileReaderOpen(filename.c_str(), &fh);
            }
            if (open_rc != 0 || !fh) {
                sendError("Failed to open file %s (rc=%d)", filename.c_str(), open_rc);
                return false;
            }

            //sendInfo("Reading file %s for timestep %d", filename.c_str(), timestep);
            // Read grids of all zones:
            int32_t numZones = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecDataSetGetNumZones(fh, &numZones);
            }

            int32_t zone = block + 1; // zone numbers start with 1, not 0

            //Modified: new chunk ---skip blocks beyond this file's zone count
            if (zone < 1 || zone > numZones) {
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    tecFileReaderClose(&fh);
                }
                return true; // nothing to output for this (timestep, block)
            }

            // -

            StructuredGrid::ptr strGrid = NULL;
            strGrid = ReadSubzoneTecplot::createStructuredGrid(fh, zone);
            auto solutionTime = 0.0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecZoneGetSolutionTime(fh, zone, &solutionTime);
            }

            //int step = getTimestepForSolutionTime(solutionTimes, solutionTime);
            strGrid->setTimestep(timestep);

            int32_t loc = 0;
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecZoneVarGetValueLocation(fh, zone, 1, &loc);
            }

            strGrid->setMapping(
                loc == 0 ? vistle::DataBase::Element
                         : vistle::DataBase::Vertex); // set mapping to vertex, because coordinates are vertex-centered
            //std::cout << "reading zone number " << zone << " of " << numZones << " zones" << std::endl;
            //std::cout << "timestep: " << timestep << std::endl;
            //std::cout << "solution time: " << solutionTime << std::endl;
            token.applyMeta(strGrid);
            token.addObject(m_grid, strGrid);

            //Define options of variable ports
            //auto indices = setFieldChoices(fh);

            // check if solution is included in the file
            int32_t fileType;
            if (tecFileGetType(fh, &fileType) != 1 && !(m_indicesCombinedVariables.empty())) {
                int32_t NumVar = 0;
                {
                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                    tecDataSetGetNumVars(fh, &NumVar);
                }

                char *varName = NULL;
                int64_t numValues;
                for (int32_t var = 0; var < NumPorts; var++) { // loop over output ports

                    std::string name = m_fieldChoice[var]->getValue();
                    if (!emptyValue(m_fieldChoice[var])) {
                        //std::cout << "Reading variable: " << name << " on port: " << var << std::endl;
                        std::vector<int> varInFile = getIndexOfTecVar(name, m_indicesCombinedVariables, fh);

                        // guard against invalid indices
                        if (varInFile.empty() || varInFile[0] < 1) {
                            std::cerr << "Skipping '" << name << "' (not present in this file/zone)\n";
                            continue;
                        }


                        {
                            std::lock_guard<std::mutex> lk(g_tecio_mutex);
                            if (tecVarGetName(fh, varInFile[0], &varName) != 0)
                                varName = nullptr;
                        }


                        // varInFile is filled already; you also did: tecVarGetName(fh, varInFile[0], &varName);

                        if (varInFile.size() == 1) {
                            {
                                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                tecZoneVarGetNumValues(fh, zone, varInFile[0], &numValues);
                            }

                            Vec<Scalar, 1>::ptr field = readVariables(fh, numValues, zone, varInFile[0]);
                            //std::cout << "Variable name in file: " << (varName ? varName : "(null)") << std::endl;

                            if (field) {
                                // 0 = nodal (vertex), 1 = cell-centered (element)
                                int32_t loc = 0;
                                {
                                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                    tecZoneVarGetValueLocation(fh, zone, varInFile[0], &loc);
                                }


                                field->addAttribute(vistle::attribute::Species, name);
                                field->setMapping(loc == 0 ? vistle::DataBase::Vertex : vistle::DataBase::Element);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                std::cout << "Accessing m_fieldsOut at index: " << var << std::endl;
                                token.addObject(m_fieldsOut[var], field);
                            }

                            if (varName) {
                                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                tecStringFree(&varName);
                            }


                        } else if (varInFile.size() > 1) {
                            // combined vector (X,Y,Z)

                            if (varInFile.size() < 3 || varInFile[0] < 1 || varInFile[1] < 1 || varInFile[2] < 1) {
                                std::cerr << "Skipping vector '" << name
                                          << "' (components missing in this file/zone)\n";
                                continue;
                            }


                            //std::cout << "Reading combined variable: " << name << " on port: " << var << std::endl;

                            {
                                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                tecZoneVarGetNumValues(fh, zone, varInFile[0], &numValues);
                            }
                            const int startIndex = 1;

                            std::vector<float> xValues(numValues), yValues(numValues), zValues(numValues);
                            {
                                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                tecZoneVarGetFloatValues(fh, zone, varInFile[0], startIndex, numValues, xValues.data());
                                tecZoneVarGetFloatValues(fh, zone, varInFile[1], startIndex, numValues, yValues.data());
                                tecZoneVarGetFloatValues(fh, zone, varInFile[2], startIndex, numValues, zValues.data());
                            }

                            Vec<Scalar, 3>::ptr field = combineVarstoOneOutput(xValues, yValues, zValues, numValues);
                            if (field) {
                                // decide mapping from location of first component (all three *should* match)
                                int32_t locX = 0, locY = 0, locZ = 0;
                                {
                                    std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                    tecZoneVarGetValueLocation(fh, zone, varInFile[0], &locX);
                                    tecZoneVarGetValueLocation(fh, zone, varInFile[1], &locY);
                                    tecZoneVarGetValueLocation(fh, zone, varInFile[2], &locZ);
                                }


                                if (locX != locY || locX != locZ) {
                                    std::cerr << "Warning: vector components have mixed locations (" << locX << ","
                                              << locY << "," << locZ << "), using first.\n";
                                }

                                field->addAttribute(vistle::attribute::Species, name);
                                field->setMapping(locX == 0 ? vistle::DataBase::Vertex : vistle::DataBase::Element);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                token.addObject(m_fieldsOut[var], field);
                            }

                            if (varName) {
                                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                                tecStringFree(&varName);
                            }
                        }

                    } else {
                        //std::cout << "Skipping variable: " << m_fieldChoice[var]->getValue() << " on position: " << var
                        //          << std::endl;
                        continue; // skip if the variable is not selected
                    }
                } //close for loop over ports
            } // close if fileType != 1
            else {
                std::cerr << "Tecplot does not contain solution variables but just a grid. " << std::endl;
            }
            {
                std::lock_guard<std::mutex> lk(g_tecio_mutex);
                tecFileReaderClose(&fh);
            }
        } //close try
        catch (const std::exception &e) {
            sendError("Failed to read file %s: %s", filename.c_str(), e.what());
            return false;
        }
        return true;
    }
}
#include "ReadSubzoneTecplot.h"

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
#include <boost/mpi/communicator.hpp>

#include <vistle/util/filesystem.h>
#include <vistle/util/stopwatch.h>

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
    //auto time1 = MPI_Wtime(); // Record start time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at construction: " << time1 << std::endl;

    m_filedir =
        addStringParameter("file_dir", "name of szTecplot files directory", "", vistle::Parameter::ExistingDirectory);

    m_grid = createOutputPort("grid_out", "grid or geometry");

    setParallelizationMode(Serial);
    //setParallelizationMode(ParallelizeTimeAndBlocks); // Parallelization does not work, leads to abortion errors

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

ReadSubzoneTecplot::~ReadSubzoneTecplot() = default;

bool ReadSubzoneTecplot::examine(const vistle::Parameter *param)
{
    StopWatch w("ReadSubzoneTecplot::examine");
    if (!param || param == m_filedir) {
        if (!inspectDir()) {
            sendError("Directory %s does not exist or is not valid", m_filedir->getValue().c_str());
            return false;
        }

        const std::string filename = fileList.front();
        try {
            setTimesteps(numFiles);

            // compute a stable partition count across ALL timesteps ---
            int32_t maxZones = 0;
            for (const auto &f: fileList) {
                void *fh2 = nullptr;
                if (tecFileReaderOpen(f.c_str(), &fh2) == 0) {
                    int32_t nz = 0;
                    tecDataSetGetNumZones(fh2, &nz);
                    if (nz > maxZones)
                        maxZones = nz;
                    tecFileReaderClose(&fh2);
                }
            }
            setPartitions(maxZones);
            setHandlePartitions(PartitionTimesteps);
            setCollectiveIo(CollectiveConstant);
            // ---end of compute

            // Open a LOCAL handle for probing
            void *fh = nullptr;
            if (tecFileReaderOpen(filename.c_str(), &fh) != 0) {
                sendError("Failed to open base file: %s", filename.c_str());
                return false;
            }

            char *dataSetTitle = nullptr;
            tecDataSetGetTitle(fh, &dataSetTitle);
            // (use dataSetTitle if you like)
            if (dataSetTitle)
                tecStringFree(&dataSetTitle);

            // Variables listing
            int32_t NumVar = 0;
            tecDataSetGetNumVars(fh, &NumVar);
            std::ostringstream outputStream;
            for (int32_t var = 1; var <= NumVar; ++var) {
                char *name = nullptr;
                tecVarGetName(fh, var, &name);
                if (name) {
                    outputStream << name;
                    tecStringFree(&name);
                }
                if (var < NumVar)
                    outputStream << ',';
            }
            std::cerr << "variables: " << outputStream.str() << std::endl;

            // File type
            int32_t fileType = 0;
            tecFileGetType(fh, &fileType);
            if (fileType == 2) { // solution-only
                std::cerr << "Tecplot file contains no grid, only solution data." << std::endl;
                tecFileReaderClose(&fh); // FIX: close before return
                return false;
            }

            // Optional: geometries notice
            int32_t numGeoms = 0;
            tecGeomGetNumGeoms(fh, &numGeoms);
            if (numGeoms > 0) {
                std::cerr << "Tecplot file contains geometries, which are not supported by this module." << std::endl;
            }

            // Ensure all zones in the base file are structured/ordered
            int32_t numZones = 0;
            tecDataSetGetNumZones(fh, &numZones);
            for (int32_t inputZone = 1; inputZone <= numZones; ++inputZone) {
                int32_t zoneType = -1;
                tecZoneGetType(fh, inputZone, &zoneType);
                if (zoneType != 0) {
                    std::cerr << "Tecplot module currently supports only structured (ordered) zones." << std::endl;
                    tecFileReaderClose(&fh); // FIX: close before return
                    return false;
                }
            }


            m_indicesCombinedVariables =
                setFieldChoices(fh); // build UI choices once & cache grouped indices (parallel-safe)


            tecFileReaderClose(&fh); // normal close
        } catch (const std::exception &e) {
            std::cerr << "failed to probe " << filename << ": " << e.what() << '\n';
            setPartitions(0);
            return false;
        }
    }
    return true;
}

bool ReadSubzoneTecplot::prepareRead()
{
    time1 = MPI_Wtime(); // Record start time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at prepareRead: " << time1 << std::endl;
    return true;
}

bool ReadSubzoneTecplot::finishRead()
{
    //double time2 = MPI_Wtime(); // Record end time for debugging MPI issues
    //std::cout << "ReadSubzoneTecplot MPI_Wtime at finishRead: " << time2 << std::endl;
    //std::cout << "ReadSubzoneTecplot total time in seconds: " << time2 - time1 << std::endl;
    return true;
}
// Check zone type for each zone
// 0 = ordered, 1 = line segment, 2 = triangle, 3 = quadrilateral, 4 = tetraheddron, 5 = brick,
// 6 = polygon, 7 = polyhedron, 8  = Mixed(not used)
Byte ReadSubzoneTecplot::tecToVistleType(int tecType)
{
    switch (tecType) {
    //case 0:
    //    return StructuredGrid;
    case 1:
        return UnstructuredGrid::BAR;
    case 2:
        return UnstructuredGrid::TRIANGLE;
    case 3:
        return UnstructuredGrid::QUAD;
    case 4:
        return UnstructuredGrid::TETRAHEDRON;
    case 5:
        return UnstructuredGrid::HEXAHEDRON;
    case 6:
        return UnstructuredGrid::POLYGON;
    case 7:
        return UnstructuredGrid::POLYHEDRON;
    default:
        std::stringstream msg;
        msg << "The tec data type with the encoding " << tecType << " is not supported.";

        std::cerr << msg.str() << std::endl;
        throw exception(msg.str());
    }
}

// Helper that reads the values of a variables with index var from a tecplot file with the specified fh for
// the given inputZone and returns a pointer to a Vec<Scalar, 1> containing the values in its x() array.
Vec<Scalar, 1>::ptr ReadSubzoneTecplot::readVariables(void *fh, int64_t numValues, const int32_t inputZone, int32_t var)
{
    int32_t varType;
    tecZoneVarGetType(fh, inputZone, var, &varType);
    Vec<Scalar, 1>::ptr field(new Vec<Scalar, 1>(numValues));
    switch ((FieldDataType_e)varType) {
    case FieldDataType_Float: {
        std::vector<float> values(numValues);
        tecZoneVarGetFloatValues(fh, inputZone, var, 1, numValues, &values[0]);
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Double: {
        std::vector<double> values(numValues);
        tecZoneVarGetDoubleValues(fh, inputZone, var, 1, numValues, &values[0]);
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Int32: {
        std::vector<int32_t> values(numValues);
        tecZoneVarGetInt32Values(fh, inputZone, var, 1, numValues, &values[0]);
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Int16: {
        std::vector<int16_t> values(numValues);
        tecZoneVarGetInt16Values(fh, inputZone, var, 1, numValues, &values[0]);
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    } break;
    case FieldDataType_Byte: {
        std::vector<uint8_t> values(numValues);
        tecZoneVarGetUInt8Values(fh, inputZone, var, 1, numValues, &values[0]);
        for (size_t i = 0; i < numValues; i++) {
            field->x()[i] = values[i];
        }
    }
    } // close switch

    return field;
}


// Builds a structured grid from the first three variables in the tecplot file, which are assumed to be the
// coordinates x, y, z. The function reads the number of vertices in each direction and
// the coordinates from the tecplot file, and returns a pointer to a StructuredGrid object
StructuredGrid::ptr ReadSubzoneTecplot::createStructuredGrid(void *fh, int32_t inputZone)
{
    // Define grid:
    // get size x, y, z of all zones
    int64_t m_numVert_x = 0;
    int64_t m_numVert_y = 0;
    int64_t m_numVert_z = 0;
    int32_t numZones = 0;
    tecDataSetGetNumZones(fh, &numZones);

    tecZoneGetIJK(fh, inputZone, &m_numVert_x, &m_numVert_y, &m_numVert_z);
    StructuredGrid::ptr str_grid = std::make_shared<StructuredGrid>(m_numVert_x, m_numVert_y, m_numVert_z);

    auto &xCoords = str_grid->x();
    auto &yCoords = str_grid->y();
    auto &zCoords = str_grid->z();

    int32_t startIndex = 1;

    // TODO: change for more than structured gird (zoneType=0) -> test data needed
    // Read the number of values for each variable

    int64_t n = 0;

    // Unknown on the fly processes leaded to errors while parallel processing
    // therefore I switched back to the copying - System monitor shows that memory is not the problem on my computer


    // // X
    // tecZoneVarGetNumValues(fh, inputZone, 1, &n);
    // if (!readVarIntoScalarArray(fh, inputZone, 1, xCoords.data(), n))
    //     std::cerr << "Failed to read X coordinates\n";

    // // Y
    // tecZoneVarGetNumValues(fh, inputZone, 2, &n);
    // if (!readVarIntoScalarArray(fh, inputZone, 2, yCoords.data(), n))
    //     std::cerr << "Failed to read Y coordinates\n";

    // // Z
    // tecZoneVarGetNumValues(fh, inputZone, 3, &n);
    // if (!readVarIntoScalarArray(fh, inputZone, 3, zCoords.data(), n))
    //     std::cerr << "Failed to read Z coordinates\n";

    // X
    int64_t numValues = 0;
    int32_t var = 1;
    tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    auto fx = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fx->x().begin(), fx->x().end(), xCoords.begin());

    // Y
    var = 2;
    tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    auto fy = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fy->x().begin(), fy->x().end(), yCoords.begin());

    // Z
    var = 3;
    tecZoneVarGetNumValues(fh, inputZone, var, &numValues);
    auto fz = readVariables(fh, (int32_t)numValues, inputZone, var);
    std::copy(fz->x().begin(), fz->x().end(), zCoords.begin());


    return str_grid;
}

// Combines the three input vectors x, y, z into a single Vec<Scalar, 3> object.
template<typename T>
Vec<Scalar, 3>::ptr ReadSubzoneTecplot::combineVarstoOneOutput(std::vector<T> x, std::vector<T> y, std::vector<T> z,
                                                               int32_t numValues)
{
    Vec<Scalar, 3>::ptr result(new Vec<Scalar, 3>(numValues));
    // Ensure all vectors have the same size
    if (x.size() != y.size() || y.size() != z.size()) {
        throw std::invalid_argument("Input vectors must have the same size");
    }

    for (size_t i = 0; i < x.size(); i++) {
        result->x()[i] = x[i];
        result->y()[i] = y[i];
        result->z()[i] = z[i];
    }
    return result;
}

// Finds variables with the same name in the variable list and ouputs their indices in a map.
// The map key is the variable name without the last character, and the value is a vector
// containing the indices of the variables that match this prefix.
std::unordered_map<std::string, std::vector<int>>
ReadSubzoneTecplot::findSimilarStrings(const std::vector<std::string> &strings)
{
    std::unordered_map<std::string, std::vector<int>> prefixMap;

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
        }
    }

    return prefixMap;
}

// Sets the variables of the dataset that the user can choose from in the menu.
std::unordered_map<std::string, std::vector<int>> ReadSubzoneTecplot::setFieldChoices(void *fh)
{
    std::vector<std::string> choices{Reader::InvalidChoice};

    int32_t NumVar = 0;
    tecDataSetGetNumVars(fh, &NumVar);

    // 1) Add all scalar variable names (skip coords 1..3) to choices
    std::vector<std::string> varNames(NumVar + 1);
    for (int32_t v = 1; v <= NumVar; ++v) {
        char *nm = nullptr;
        tecVarGetName(fh, v, &nm);
        varNames[v] = nm ? std::string(nm) : std::string();
        if (nm)
            tecStringFree(&nm);
        if (v >= 4 && !varNames[v].empty()) {
            choices.push_back(varNames[v]);
        }
    }

    // 2) Build combined vector groups by prefix (strip trailing X/Y/Z)
    //    Only add groups where all three components exist.
    struct Triple {
        int ix = -1, iy = -1, iz = -1;
    };
    std::unordered_map<std::string, Triple> triples;

    auto ends_with = [](const std::string &s, char c) {
        return !s.empty() && s.back() == c;
    };

    for (int32_t v = 4; v <= NumVar; ++v) {
        const auto &nm = varNames[v];
        if (nm.size() < 2)
            continue;
        char last = nm.back();
        if (last != 'X' && last != 'Y' && last != 'Z')
            continue;
        std::string pref = nm.substr(0, nm.size() - 1);
        auto &t = triples[pref];
        if (last == 'X')
            t.ix = v;
        if (last == 'Y')
            t.iy = v;
        if (last == 'Z')
            t.iz = v;
    }

    std::unordered_map<std::string, std::vector<int>> groups;
    for (auto &kv: triples) {
        const auto &pref = kv.first;
        const auto &t = kv.second;
        if (t.ix > 0 && t.iy > 0 && t.iz > 0) {
            groups[pref] = {t.ix, t.iy, t.iz};
            choices.push_back(pref); // expose combined vector as a choice
            //std::cout << "Grouped vector: " << pref << " -> {" << t.ix << "," << t.iy << "," << t.iz << "}\n";
        }
    }

    // 3) Set the choices for each output port once
    for (int i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_fieldChoice[i], choices);
    }

    // (optional) keep a copy
    m_varChoices = choices;

    return groups;
}


// Returns the index of the variable with the given name in the tecplot file.
// If the variable is a combined variable, it returns the indices of the combined variable.
// If the variable is not found, it returns a vector with -1.
std::vector<int>
ReadSubzoneTecplot::getIndexOfTecVar(const std::string &varName,
                                     const std::unordered_map<std::string, std::vector<int>> &indicesCombinedVariables,
                                     void *fh) const
{
    std::vector<int> result;
    // get number of variables
    int32_t NumVar = 0;
    tecDataSetGetNumVars(fh, &NumVar);
    for (int32_t var = 1; var <= NumVar; ++var) {
        char *name = NULL;
        tecVarGetName(fh, var, &name);
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
                    //std::cout << "Found combined variable: " << combinedName << " with indices: " << group.second[0]
                    //          << ", " << group.second[1] << ", " << group.second[2] << std::endl;
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
//checks if choice is a valid input
bool ReadSubzoneTecplot::inspectDir()
{
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
            //sendInfo("Locating files in %s", dir.string().c_str());
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
                //sendInfo("Loading file %s", fName.c_str());
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
    //sendInfo("Directory contains %d timesteps", numFiles);
    if (numFiles == 0)
        return false;
    return true;
}

// orderSolutionTimes and getTimestepForSolutionTime can be used to order the solution times in the fileList
// if the times are not given in the file names.
std::unordered_map<int, double> ReadSubzoneTecplot::orderSolutionTimes(std::vector<std::string> fileList)
{
    std::vector<std::pair<double, int>> solutionTimeTimestepPairs;

    // Collect all solution times and their corresponding timesteps
    for (const auto &file: fileList) {
        try {
            void *fh = nullptr;
            tecFileReaderOpen(file.c_str(), &fh);

            int32_t numZones = 0;
            tecDataSetGetNumZones(fh, &numZones);

            for (int32_t zone = 1; zone <= numZones; ++zone) {
                double solutionTime = 0.0;
                tecZoneGetSolutionTime(fh, zone, &solutionTime);
                solutionTimeTimestepPairs.emplace_back(solutionTime, solutionTimeTimestepPairs.size());
            }

            tecFileReaderClose(&fh);
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

int ReadSubzoneTecplot::getTimestepForSolutionTime(std::unordered_map<int, double> &orderedSolutionTimes,
                                                   double solutionTime)
{
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
    if (timestep < 0 || timestep >= numFiles) {
        //std::cout << "Constant timestep: " << timestep << std::endl;
        return true;
    } else {
        const std::string &filename = fileList[timestep];
        try {
            void *fh = nullptr;
            const int open_rc = tecFileReaderOpen(filename.c_str(), &fh);
            if (open_rc != 0 || !fh) {
                sendError("Failed to open file %s (rc=%d)", filename.c_str(), open_rc);
                return false;
            }
            //sendInfo("Reading file %s for timestep %d", filename.c_str(), timestep);
            // Read grids of all zones:
            int32_t numZones = 0;
            tecDataSetGetNumZones(fh, &numZones);
            int32_t zone = block + 1; // zone numbers start with 1, not 0

            //Modified: new chunk ---skip blocks beyond this file's zone count
            if (zone < 1 || zone > numZones) {
                tecFileReaderClose(&fh);
                return true; // nothing to output for this (timestep, block)
            }
            // -

            StructuredGrid::ptr strGrid = NULL;
            strGrid = ReadSubzoneTecplot::createStructuredGrid(fh, zone);
            auto solutionTime = 0.0;
            tecZoneGetSolutionTime(fh, zone, &solutionTime);
            //int step = getTimestepForSolutionTime(solutionTimes, solutionTime);
            strGrid->setTimestep(timestep);

            int32_t loc = 0;
            tecZoneVarGetValueLocation(fh, zone, 1, &loc);
            //std::cout << "reading zone number " << zone << " of " << numZones << " zones" << std::endl;
            //std::cout << "timestep: " << timestep << std::endl;
            //std::cout << "solution time: " << solutionTime << std::endl;
            token.applyMeta(strGrid);
            token.addObject(m_grid, strGrid);

            //Define options of variable ports
            //auto indices = setFieldChoices(fh);

            // check if solution is included in the file
            int32_t fileType;
            if (tecFileGetType(fh, &fileType) != 1 && !(m_indicesCombinedVariables.empty())) {
                int32_t NumVar = 0;
                tecDataSetGetNumVars(fh, &NumVar);
                char *varName = NULL;
                int64_t numValues;
                for (int32_t var = 0; var < NumPorts; var++) { // loop over output ports

                    std::string name = m_fieldChoice[var]->getValue();
                    if (!emptyValue(m_fieldChoice[var])) {
                        //std::cout << "Reading variable: " << name << " on port: " << var << std::endl;
                        std::vector<int> varInFile = getIndexOfTecVar(name, m_indicesCombinedVariables, fh);

                        // guard against invalid indices
                        if (varInFile.empty() || varInFile[0] < 1) {
                            std::cerr << "Skipping '" << name << "' (not present in this file/zone)\n";
                            continue;
                        }


                        if (tecVarGetName(fh, varInFile[0], &varName) != 0)
                            varName = nullptr;


                        // varInFile is filled already; you also did: tecVarGetName(fh, varInFile[0], &varName);

                        if (varInFile.size() == 1) {
                            tecZoneVarGetNumValues(fh, zone, varInFile[0], &numValues);

                            Vec<Scalar, 1>::ptr field = readVariables(fh, numValues, zone, varInFile[0]);
                            //std::cout << "Variable name in file: " << (varName ? varName : "(null)") << std::endl;

                            if (field) {
                                // 0 = nodal (vertex), 1 = cell-centered (element)
                                int32_t loc = 0;
                                tecZoneVarGetValueLocation(fh, zone, varInFile[0], &loc);

                                field->addAttribute(vistle::attribute::Species, name);
                                field->setMapping(loc == 0 ? vistle::DataBase::Element : vistle::DataBase::Vertex);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                std::cout << "Accessing m_fieldsOut at index: " << var << std::endl;
                                token.addObject(m_fieldsOut[var], field);
                            }

                            if (varName)
                                tecStringFree(&varName); // free in all paths

                        } else if (varInFile.size() > 1) {
                            // combined vector (X,Y,Z)

                            if (varInFile.size() < 3 || varInFile[0] < 1 || varInFile[1] < 1 || varInFile[2] < 1) {
                                std::cerr << "Skipping vector '" << name
                                          << "' (components missing in this file/zone)\n";
                                continue;
                            }


                            //std::cout << "Reading combined variable: " << name << " on port: " << var << std::endl;

                            tecZoneVarGetNumValues(fh, zone, varInFile[0], &numValues);
                            const int startIndex = 1;

                            std::vector<float> xValues(numValues), yValues(numValues), zValues(numValues);
                            tecZoneVarGetFloatValues(fh, zone, varInFile[0], startIndex, numValues, xValues.data());
                            tecZoneVarGetFloatValues(fh, zone, varInFile[1], startIndex, numValues, yValues.data());
                            tecZoneVarGetFloatValues(fh, zone, varInFile[2], startIndex, numValues, zValues.data());

                            Vec<Scalar, 3>::ptr field = combineVarstoOneOutput(xValues, yValues, zValues, numValues);
                            if (field) {
                                // decide mapping from location of first component (all three *should* match)
                                int32_t locX = 0, locY = 0, locZ = 0;
                                tecZoneVarGetValueLocation(fh, zone, varInFile[0], &locX);
                                tecZoneVarGetValueLocation(fh, zone, varInFile[1], &locY);
                                tecZoneVarGetValueLocation(fh, zone, varInFile[2], &locZ);
                                if (locX != locY || locX != locZ) {
                                    std::cerr << "Warning: vector components have mixed locations (" << locX << ","
                                              << locY << "," << locZ << "), using first.\n";
                                }

                                field->addAttribute(vistle::attribute::Species, name);
                                field->setMapping(locX == 0 ? vistle::DataBase::Element : vistle::DataBase::Vertex);
                                field->setGrid(strGrid);
                                token.applyMeta(field);
                                token.addObject(m_fieldsOut[var], field);
                            }

                            if (varName)
                                tecStringFree(&varName); // free here too
                        }

                    } else {
                        //std::cout << "Skipping variable: " << m_fieldChoice[var]->getValue() << " on position: " << var
                        //          << std::endl;
                        continue; // skip if the variable is not selected
                    }
                } //close for loop over ports
            } // close if fileType != 1
            else {
                std::cerr << "Tecplot does not contain solution variables but just a grid. " << std::endl;
            }
            tecFileReaderClose(&fh);
        } //close try
        catch (const std::exception &e) {
            sendError("Failed to read file %s: %s", filename.c_str(), e.what());
            return false;
        }
        return true;
    }
}
