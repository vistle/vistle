#include "ReadEwMatlabCsvExports.h"
#include <vistle/core/scalar.h>
#include <vistle/core/points.h>
#include <vistle/core/unstr.h>
#include <boost/filesystem.hpp>

using namespace vistle;

MODULE_MAIN(Transversalflussmaschine)

Transversalflussmaschine::Transversalflussmaschine(const std::string &name, int moduleID, mpi::communicator comm)
: Reader(name, moduleID, comm)
{
    setParallelizationMode(Reader::ParallelizationMode::Serial);
    m_filePathParam = addStringParameter("filename", "csv file path", "", Parameter::ExistingFilename);
    observeParameter(m_filePathParam);
    setParameterFilters("filename", "csv Files (*.csv)/All Files (*)");
    m_gridPort = createOutputPort("grid_out", "grid");
    m_dataPort = createOutputPort("data_out", "scalar data");
}

Transversalflussmaschine::~Transversalflussmaschine()
{}

constexpr size_t numCoords = 3;

bool Transversalflussmaschine::examine(const Parameter *param)
{
    auto &filename = m_filePathParam->getValue();
    if (!filename.empty()) {
        rapidcsv::LabelParams lp(0, 0);
        rapidcsv::SeparatorParams sp(';');
        rapidcsv::ConverterParams cp;
        rapidcsv::LineReaderParams lrp(false, '#', true);

        if (!boost::filesystem::exists(filename)) {
            sendError("file " + filename + " does not exist");
            return false;
        }
        m_data = std::make_unique<rapidcsv::Document>(filename, lp, sp, cp, lrp);

        auto ext = boost::filesystem::path{filename}.extension().string();

        auto connectivityFileName = filename.substr(0, filename.size() - ext.size()) + "_connectivity" + ext;

        if (boost::filesystem::exists(connectivityFileName)) {
            rapidcsv::LabelParams lp2(-1, -1);
            m_connectivity = std::make_unique<rapidcsv::Document>(connectivityFileName, lp2, sp, cp, lrp);
        }

        else
            sendWarning("No connectivity file found under " + connectivityFileName);


        setTimesteps(m_data->GetColumnCount() - numCoords);
        setPartitions(1);
    }
    return true;
}


bool Transversalflussmaschine::read(Token &token, int timestep, int block)
{
    if (timestep == -1 && m_data) {
        std::array<std::vector<float>, 3> dataVertices = {m_data->GetColumn<float>("x"), m_data->GetColumn<float>("y"),
                                                          m_data->GetColumn<float>("z")};
        size_t numVertices = dataVertices[0].size();
        std::array<Scalar *, 3> vertices;
        if (!m_connectivity) {
            auto points = make_ptr<Points>(numVertices);
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
            UnstructuredGrid::ptr grid = make_ptr<UnstructuredGrid>(numElements, numCorners, numVertices);
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
        auto data = m_data->GetColumn<float>(timestep + numCoords);
        Vec<Scalar>::ptr scal(new Vec<Scalar>(data.size()));
        std::copy(data.begin(), data.end(), scal->x().begin());
        scal->setMapping(vistle::DataBase::Vertex);
        scal->setGrid(m_grid);
        scal->addAttribute("_species", "Stromdichte");
        scal->setTimestep(timestep);
        token.applyMeta(scal);
        token.addObject(m_dataPort, scal);
    }
    return true;
}
