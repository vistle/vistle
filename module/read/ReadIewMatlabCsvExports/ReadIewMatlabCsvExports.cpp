#include "ReadIewMatlabCsvExports.h"
#include <vistle/core/scalar.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>
#include <vistle/util/filesystem.h>


using namespace vistle;
namespace fs = vistle::filesystem;

MODULE_MAIN(Transversalflussmaschine)
DEFINE_ENUM_WITH_STRING_CONVERSIONS(Format, (ScalarFile)(VectorFiles))
Transversalflussmaschine::Transversalflussmaschine(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    setParallelizationMode(Reader::ParallelizationMode::Serial);
    m_filePathParam = addStringParameter("filename", "csv file path", "", Parameter::ExistingFilename);
    observeParameter(m_filePathParam);
    setParameterFilters("filename", "csv Files (*.csv)");
    m_gridPort = createOutputPort("grid_out", "grid");
    m_dataPort = createOutputPort("data_out", "scalar data");
    m_format = addIntParameter("format", "Scalar expects single file, Vector expects 3 files ending with _x.csv, ...",
                               ScalarFile, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_format, Format);
    observeParameter(m_format);
}

Transversalflussmaschine::~Transversalflussmaschine()
{}

constexpr size_t numCoords = 3;

bool checkFormat(const std::array<std::unique_ptr<rapidcsv::Document>, 3> &data)
{
    for (size_t i = 0; i < data.size(); i++) {
        auto next = (i + 1) / data.size();
        if (data[i] && data[next] &&
            (data[i]->GetColumnCount() != data[next]->GetColumnCount() ||
             data[i]->GetRowCount() != data[next]->GetRowCount())) {
            return false;
        }
    }
    return true;
}

bool Transversalflussmaschine::examine(const Parameter *param)
{
    auto &filepath = m_filePathParam->getValue();

    if (!filepath.empty()) {
        std::vector<std::string> filepaths;
        fs::path fp{filepath};
        auto ext = fp.extension().string();
        auto stem = filepath.substr(0, filepath.size() - ext.size());
        m_species = fp.filename().string();
        switch (m_format->getValue()) {
        case ScalarFile:
            filepaths.push_back(filepath);
            break;

        case VectorFiles: {
            filepaths.push_back(stem + "_x" + ext);
            filepaths.push_back(stem + "_y" + ext);
            filepaths.push_back(stem + "_z" + ext);
            break;
        }
        }

        rapidcsv::LabelParams lp(0, 0);
        rapidcsv::SeparatorParams sp(';');
        rapidcsv::ConverterParams cp;
        rapidcsv::LineReaderParams lrp(false, '#', true);

        for (size_t i = 0; i < filepaths.size(); i++) {
            if (!fs::exists(filepaths[i])) {
                if (i == 0) {
                    sendError("file " + filepaths[i] + " does not exist");
                    return false;
                } else {
                    sendWarning("file " + filepaths[i] + " does not exist ->setting to 0");
                    m_data[i] = nullptr;
                }
            }
            m_data[i] = std::make_unique<rapidcsv::Document>(filepaths[i], lp, sp, cp, lrp);
        }
        auto connectivityFileName = stem + "_connectivity" + ext;

        if (fs::exists(connectivityFileName)) {
            rapidcsv::LabelParams lp2(-1, -1);
            m_connectivity = std::make_unique<rapidcsv::Document>(connectivityFileName, lp2, sp, cp, lrp);
        } else
            sendWarning("No connectivity file found under " + connectivityFileName);


        if (!checkFormat(m_data)) {
            sendError("Vector files don't have the same number of timesteps and vertecies");
            return false;
        }
        setTimesteps(m_data[0]->GetColumnCount() - numCoords);
        setPartitions(1);
    }
    return true;
}

bool Transversalflussmaschine::read(Token &token, int timestep, int block)
{
    if (timestep == -1 && m_data[0]) {
        std::array<std::vector<float>, 3> dataVertices = {
            m_data[0]->GetColumn<float>("x"), m_data[0]->GetColumn<float>("y"), m_data[0]->GetColumn<float>("z")};
        size_t numVertices = dataVertices[0].size();
        std::array<Scalar *, 3> vertices;
        if (!m_connectivity) {
            auto points = std::make_shared<Points>(numVertices);
            vertices = {points->x().begin(), points->y().begin(), points->z().begin()};
            m_grid = points;
        } else {
            constexpr auto cornersPerElement = UnstructuredGrid::NumVertices[UnstructuredGrid::TETRAHEDRON];
            std::array<std::vector<size_t>, cornersPerElement> connectivity;
            for (size_t i = 0; i < cornersPerElement; i++) {
                connectivity[i] = m_connectivity->GetRow<size_t>(i);
            }

            size_t numElements = connectivity[0].size();
            size_t numCorners = cornersPerElement * numElements;
            UnstructuredGrid::ptr grid = std::make_shared<UnstructuredGrid>(numElements, numCorners, numVertices);
            vertices = {grid->x().begin(), grid->y().begin(), grid->z().begin()};
            m_grid = grid;
            auto tl = grid->tl().data();
            auto el = grid->el().data();
            auto cl = grid->cl().data();

            for (size_t i = 0; i < numElements; i++) {
                tl[i] = UnstructuredGrid::TETRAHEDRON;
                el[i] = cornersPerElement * i;

                for (size_t j = 0; j < cornersPerElement; j++) {
                    cl[4 * i + j] = connectivity[j][i];
                }
            }
            el[numElements] = cornersPerElement * numElements;
        }


        for (size_t i = 0; i < 3; i++) {
            std::copy(dataVertices[i].begin(), dataVertices[i].end(), vertices[i]);
        }


        token.applyMeta(m_grid);
        token.addObject(m_gridPort, m_grid);
    } else {
        if (!m_grid)
            return false;
        vistle::DataBase::ptr obj;

        switch (m_format->getValue()) {
        case ScalarFile: {
            auto data = m_data[0]->GetColumn<float>(timestep + numCoords);
            auto scal = std::make_shared<Vec<Scalar>>(data.size());
            std::copy(data.begin(), data.end(), scal->x().begin());
            obj = scal;

            break;
        }
        case VectorFiles: {
            auto size = m_data[0]->GetColumn<float>(timestep + numCoords).size();
            auto vec = std::make_shared<Vec<Scalar, 3>>(size, 0);
            for (size_t i = 0; i < 3; i++) {
                if (m_data[i]) {
                    auto data = m_data[i]->GetColumn<float>(timestep + numCoords);
                    std::copy(data.begin(), data.end(), vec->d()->x[i]->begin());
                }
            }
            obj = vec;
            break;
        }
        }

        obj->setMapping(vistle::DataBase::Vertex);
        obj->setGrid(m_grid);
        obj->addAttribute(attribute::Species, m_species);
        obj->setTimestep(timestep);
        token.applyMeta(obj);
        token.addObject(m_dataPort, obj);
    }
    return true;
}
