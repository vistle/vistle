/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

#include "ReadCSV.h"

#include <vistle/core/unstr.h>
#include <vistle/core/points.h>
#include <vistle/util/filesystem.h>

#include <fstream>

#include <thread>
#include <chrono>

#include "fast-cpp-csv-parser/csv.h"

using namespace vistle;

MODULE_MAIN(ReadCSV)
DEFINE_ENUM_WITH_STRING_CONVERSIONS(LayerMode, (NONE)(X)(Y)(Z))

ReadCSV::ReadCSV(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    createOutputPort("points_out", "unordered points");
    m_directory = addStringParameter("directory", "directory with CSV files", "", Parameter::ExistingDirectory);
    setParameterFilters(m_directory, "CSV files (*.csv)");
    m_filename = addIntParameter("filename", "Name of file", 0, Parameter::Choice);
    observeParameter(m_directory);
    observeParameter(m_filename);

    m_layerMode = addIntParameter("layerMode", "layer mode", 0, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_layerMode, LayerMode);
    m_layerOffset = addFloatParameter("layerOffset", "layer offset", 0);
    m_layersInOneObject = addIntParameter("layersInOneObject", "layers in one object", false, Parameter::Boolean);
    observeParameter(m_layersInOneObject);
    static const std::array<std::string, NUM_COORD_FIELDS> coordNames = {"x", "y", "z"};
    for (size_t i = 0; i < NUM_COORD_FIELDS; i++) {
        m_selectionParams[i] =
            addIntParameter(coordNames[i] + "Name", "Name of " + coordNames[i] + " column", 0, Parameter::Choice);
    }

    for (size_t i = 0; i < NUM_DATA_FIELDS; i++) {
        m_selectionParams[i + NUM_COORD_FIELDS] =
            addIntParameter("dataName" + std::to_string(i),
                            "Name of data column outputted at data_out_" + std::to_string(i), 0, Parameter::Choice);
        createOutputPort("data_out" + std::to_string(i), "data on points from column dataName" + std::to_string(i));
    }

    setParallelizationMode(ParallelizationMode::ParallelizeBlocks);

    // m_mode = addStringParameter("mode", "Mode", "singleFile", Parameter::Choice);
    // setParameterChoices(m_mode, {"singleFile", "multipleFiles"});
    // observeParameter(m_mode);
}

constexpr std::array<char, 3> delimiters = {',', ';', '\t'};

char determineDelimiter(const std::string &line)
{
    std::unordered_map<char, int> delimiterCount;

    for (char delimiter: delimiters) {
        delimiterCount[delimiter] = std::count(line.begin(), line.end(), delimiter);
    }

    auto maxElement = std::max_element(
        delimiterCount.begin(), delimiterCount.end(),
        [](const std::pair<char, int> &a, const std::pair<char, int> &b) { return a.second < b.second; });

    if (maxElement->second > 0) {
        return maxElement->first;
    }
    std::cerr << "Could not determine delimiter" << std::endl;
    return delimiters[0];
}

void splitLine(std::vector<std::string> &tokensIn, const std::string &line, char delimiter)
{
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokensIn.push_back(token);
    }
}

std::string getFilename(const std::string &directory, vistle::Integer index)
{
    vistle::filesystem::path dir(directory);
    for (auto &p: vistle::filesystem::directory_iterator(dir)) {
        if (p.path().extension() == ".csv" || p.path().extension() == ".CSV") {
            if (index == 0) {
                return p.path().string();
            }
            --index;
        }
    }
    return "";
}

bool ReadCSV::examine(const Parameter *param)
{
    vistle::filesystem::path dir(m_directory->getValue());
    if (param == m_directory || param == nullptr) {
        if (!vistle::filesystem::exists(dir)) {
            sendError("Directory " + m_directory->getValue() + " does not exist");
            return true;
        }
        //get all csv files in directory
        std::vector<std::string> files;
        files.push_back("all files");
        for (auto &p: vistle::filesystem::directory_iterator(dir)) {
            if (p.path().extension() == ".csv" || p.path().extension() == ".CSV") {
                files.push_back(p.path().filename().string());
            }
        }
        setParameterChoices(m_filename, files);
    }
    if (param == m_filename || param == nullptr) {
        if (!m_layersInOneObject->getValue())
            setPartitions(std::max(m_filename->choices().size() - 1, size_t(1)));
        std::fstream f(getFilename(m_directory->getValue(), std::max(vistle::Integer(0), m_filename->getValue() - 1)));
        if (!f.is_open()) {
            sendError("Could not open file " + m_directory->getValue());
            std::cerr << "returning early 2" << std::endl;

            return true;
        }
        std::string line;
        while (line.empty()) {
            std::getline(f, line);
        }
        m_delimiter = determineDelimiter(line);
        m_choices = std::vector<std::string>{"NONE"};
        splitLine(m_choices, line, m_delimiter);

        for (auto p: m_selectionParams) {
            setParameterChoices(p, m_choices);
            std::cerr << "choices for " << p->getName() << std::endl;
        }
    }
    if (param == m_layersInOneObject) {
        if (m_layersInOneObject->getValue()) {
            setPartitions(1);
        } else {
            setPartitions(std::max(m_filename->choices().size() - 1, size_t(1)));
        }
    }
    return true;
}

template<size_t N>
struct num {
    static const constexpr auto value = N;
};

template<class F, size_t... Is>
void for_(F func, std::index_sequence<Is...>)
{
    (func(num<Is>{}), ...);
}

template<size_t N, typename F>
void for_(F func)
{
    for_(func, std::make_index_sequence<N>());
}

template<size_t N, char Delimiter>
using CSVReader = io::CSVReader<N, io::trim_chars<' ', '\t'>, io::no_quote_escape<Delimiter>>;

template<size_t N, size_t... I, char Delimiter>
void read_header(CSVReader<N, Delimiter> &reader, const std::array<const char *, N> &arr, std::index_sequence<I...>)
{
    reader.read_header(io::ignore_missing_column | io::ignore_extra_column, arr[I]...);
}

template<size_t N, char Delimiter>
void read_header(CSVReader<N, Delimiter> &reader, const std::array<const char *, N> &arr)
{
    read_header(reader, arr, std::make_index_sequence<N>{});
}

template<size_t N, size_t... I, char Delimiter>
bool read_row(CSVReader<N, Delimiter> &reader, std::array<float, N> &arr, std::index_sequence<I...>)
{
    return reader.read_row(arr[I]...);
}

template<size_t N, char Delimiter>
bool read_row(CSVReader<N, Delimiter> &reader, std::array<float, N> &arr)
{
    return read_row(reader, arr, std::make_index_sequence<N>{});
}

vistle::Vector3 layerOffset(LayerMode mode, float offset)
{
    switch (mode) {
    case LayerMode::X:
        return Vector3(offset, 0, 0);
    case LayerMode::Y:
        return Vector3(0, offset, 0);
    case LayerMode::Z:
        return Vector3(0, 0, offset);
    default:
        return Vector3(0, 0, 0);
    }
}

template<char Delimiter>
void ReadCSV::readLayer(size_t layer, vistle::Points::ptr &points,
                        std::array<vistle::Vec<Scalar>::ptr, NUM_DATA_FIELDS> &dataFields)
{
    CSVReader<NUM_FIELDS, Delimiter> in(getFilename(m_directory->getValue(), layer));
    std::array<const char *, NUM_FIELDS> colNames;
    for (size_t i = 0; i < NUM_FIELDS; i++) {
        colNames[i] = m_choices[m_selectionParams[i]->getValue()].c_str();
    }
    read_header(in, colNames);
    std::array<float, NUM_FIELDS> csvData;
    csvData.fill(0);
    auto lo = layerOffset(static_cast<LayerMode>(m_layerMode->getValue()), m_layerOffset->getValue());
    while (read_row(in, csvData)) {
        points->x().push_back(csvData[0] + lo[0] * layer);
        points->y().push_back(csvData[1] + lo[1] * layer);
        if (getCoordSelection(2) != 0) { //3D data
            points->z().push_back(csvData[2] + lo[2] * layer);
        } else {
            points->z().push_back(lo[2] * layer);
        }
        for (size_t i = 0; i < NUM_DATA_FIELDS; i++) {
            if (dataFields[i]) {
                dataFields[i]->x().push_back(csvData[i + NUM_COORD_FIELDS]);
            }
        }
    }
}

template<char Delimiter>
bool ReadCSV::readFile(Token &token, int timestep, int block)
{
    if (block < 0 && !m_layersInOneObject->getValue()) {
        return true;
    }
    sendInfo("Reading block %d, %s", block, getFilename(m_directory->getValue(), block).c_str());
    vistle::Points::ptr points = make_ptr<vistle::Points>((size_t)0);

    std::array<vistle::Vec<Scalar>::ptr, NUM_DATA_FIELDS> dataObjects;
    for (size_t i = 0; i < NUM_DATA_FIELDS; i++) {
        if (getDataSelection(i) != 0) {
            dataObjects[i] = make_ptr<vistle::Vec<Scalar>>((size_t)0);
        }
    }
    if (m_layersInOneObject->getValue()) {
        for (size_t i = 0; i < m_filename->choices().size() - 1; i++) {
            readLayer<Delimiter>(i, points, dataObjects);
        }
    } else {
        readLayer<Delimiter>(block, points, dataObjects);
    }
    points->setBlock(block);
    points->setTimestep(timestep);

    token.applyMeta(points);
    token.addObject("points_out", points);
    for (size_t i = 0; i < NUM_DATA_FIELDS; i++) {
        if (dataObjects[i]) {
            dataObjects[i]->setMapping(DataBase::Vertex);
            dataObjects[i]->setGrid(points);
            dataObjects[i]->setBlock(block);
            dataObjects[i]->setTimestep(timestep);

            dataObjects[i]->addAttribute("_species", m_choices[getDataSelection(i)]);
            token.applyMeta(dataObjects[i]);
            token.addObject("data_out" + std::to_string(i), dataObjects[i]);
        }
    }
    return true;
}

bool ReadCSV::read(Token &token, int timestep, int block)
{
    if (getCoordSelection(0) == 0 || getCoordSelection(1) == 0) {
        sendError("At Least x and y column must be selected");
        return false;
    }
    bool retval = false;
    for_<3>([&](auto i) {
        if (!retval && m_delimiter == delimiters[i.value]) {
            retval |= readFile<delimiters[i.value]>(token, timestep, block);
        }
        return false;
    });
    return retval;
}

vistle::Integer ReadCSV::getCoordSelection(size_t i)
{
    return m_selectionParams[i]->getValue();
}

vistle::Integer ReadCSV::getDataSelection(size_t i)
{
    return m_selectionParams[i + NUM_COORD_FIELDS]->getValue();
}
