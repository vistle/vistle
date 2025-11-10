#include <boost/algorithm/string/predicate.hpp>

#include <vistle/core/lines.h>
#include <vistle/core/object.h>
#include <vistle/core/points.h>
#include <vistle/core/polygons.h>
#include <vistle/core/rectilineargrid.h>
#include <vistle/core/structuredgrid.h>
#include <vistle/core/uniformgrid.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>

#include <vistle/util/filesystem.h>

#include <vtkCellData.h>
#include <vtkCompositeDataIterator.h>
#include <vtkCompositeDataPipeline.h>
#include <vtkCompositeDataSet.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>
#include <vtkGenericDataSet.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkRectilinearGrid.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataReader.h>
#if VTK_MAJOR_VERSION >= 9
#include <vtkHDFReader.h>
#endif
#include <vtkVersion.h>
#if VTK_MAJOR_VERSION < 5
#include <vtkIdType.h>
#endif
#include <vtkDataSetAttributes.h>
#include <vtkSmartPointer.h>

#include "ReadVtk.h"

#include <vistle/vtk/vtktovistle.h>


#ifdef HAVE_TINYXML2
#include <tinyxml2.h>
namespace xml = tinyxml2;
#endif

using vistle::Parameter;

MODULE_MAIN(ReadVtk)
using vistle::Reader;
using vistle::DataBase;
using vistle::Parameter;

const std::string Invalid = Reader::InvalidChoice;

bool isCollectionFile(const std::string &fn)
{
    constexpr const char *collectionEndings[] = {".pvd", ".vtm", ".pvti", ".pvtp", ".pvtr", ".pvts", ".pvtu"};
    for (const auto ending: collectionEndings)
        if (boost::algorithm::ends_with(fn, ending))
            return true;

    return false;
}

template<class VO>
std::vector<std::string> getFields(VO *dsa)
{
    std::vector<std::string> fields;
    if (!dsa)
        return fields;
    int na = dsa->GetNumberOfArrays();
    for (int i = 0; i < na; ++i) {
        fields.push_back(dsa->GetArrayName(i));
        //cerr << "field " << i << ": " << fields[i] << endl;
    }
    return fields;
}

template<class Reader>
VtkFile readFile(const std::string &filename, int piece = -1, const ReadOptions &options = ReadOptions())
{
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
#if (VTK_MAJOR_VERSION == 9 && VTK_MINOR_VERSION >= 2) || VTK_MAJOR_VERSION > 9
    if (auto hdfreader = vtkHDFReader::SafeDownCast(reader)) {
        hdfreader->SetUseCache(options.useCache);
    }
#endif
    reader->SetFileName(filename.c_str());
    reader->UpdateInformation();
    int numPieces = 1;
#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION > 7
    if (auto unstr = vtkXMLUnstructuredDataReader::SafeDownCast(reader)) {
        numPieces = unstr->GetNumberOfPieces();
    }
    if (auto poly = vtkXMLPolyDataReader::SafeDownCast(reader)) {
        numPieces = poly->GetNumberOfPieces();
    }
#endif
    result.pieces = numPieces;

    if (auto mbreader = vtkXMLMultiBlockDataReader::SafeDownCast(reader)) {
        const int np = mbreader->GetNumberOfPointArrays();
        for (int i = 0; i < np; ++i)
            result.pointfields.push_back(mbreader->GetPointArrayName(i));
        const int nc = mbreader->GetNumberOfCellArrays();
        for (int i = 0; i < nc; ++i)
            result.cellfields.push_back(mbreader->GetCellArrayName(i));
    } else if (auto xmlreader = vtkXMLReader::SafeDownCast(reader)) {
        const int np = xmlreader->GetNumberOfPointArrays();
        for (int i = 0; i < np; ++i)
            result.pointfields.push_back(xmlreader->GetPointArrayName(i));
        const int nc = xmlreader->GetNumberOfCellArrays();
        for (int i = 0; i < nc; ++i)
            result.cellfields.push_back(xmlreader->GetCellArrayName(i));
    } else if (auto dsreader = vtkDataReader::SafeDownCast(reader)) {
        const int ns = dsreader->GetNumberOfScalarsInFile();
        for (int i = 0; i < ns; ++i)
            result.pointfields.push_back(dsreader->GetScalarsNameInFile(i));
        const int nv = dsreader->GetNumberOfVectorsInFile();
        for (int i = 0; i < nv; ++i)
            result.pointfields.push_back(dsreader->GetVectorsNameInFile(i));
    }

    if (options.onlyMeta) {
        if (reader->GetOutput()) {
            reader->GetOutput()->Register(reader);

            if (result.pointfields.empty() && result.cellfields.empty()) {
                reader->Update();
                result.dataset = reader->GetOutput();
                if (auto ds = vtkDataSet::SafeDownCast(result.dataset)) {
                    result.pointfields = getFields<vtkFieldData>(ds->GetPointData());
                    if (result.pointfields.empty())
                        result.pointfields = getFields<vtkFieldData>(ds->GetFieldData());
                    result.cellfields = getFields<vtkFieldData>(ds->GetCellData());
                }
            } else {
                result.dataset = reader->GetOutput();
            }
        }
        return result;
    }

#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION > 7
    if (piece >= 0 && numPieces > 1) {
        auto info = vtkSmartPointer<vtkInformation>::New();
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), options.ghost ? 1 : 0);
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


VtkFile getDataSet(const std::string &filename, int piece = -1, const ReadOptions &options = ReadOptions())
{
    VtkFile fileinfo;
    bool triedLegacy = false;
    if (boost::algorithm::ends_with(filename, ".vtk")) {
        fileinfo = readFile<vtkDataSetReader>(filename, piece, options);
        triedLegacy = true;
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLUnstructuredGridReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLStructuredGridReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLRectilinearGridReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLImageDataReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLPolyDataReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLMultiBlockDataReader>(filename, piece, options);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLGenericDataObjectReader>(filename, piece, options);
    }
#if VTK_MAJOR_VERSION >= 9
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkHDFReader>(filename, piece, options);
    }
#endif
    if (!triedLegacy && !fileinfo.dataset) {
        fileinfo = readFile<vtkDataSetReader>(filename, piece, options);
    }
    return fileinfo;
}

VtkFile ReadVtk::getDataSetMeta(const std::string &filename)
{
    auto &fileinfo = m_files[filename];
    if (!fileinfo.dataset) {
        ReadOptions options;
        options.onlyMeta = true;
        fileinfo = getDataSet(filename, -1, options);
    }
    return fileinfo;
}

int ReadVtk::getNumPieces(const std::string &filename)
{
    return getDataSetMeta(filename).pieces;
}

struct ReadVtkData {
    std::vector<double> times;
    std::map<double, std::vector<VtkFile>> timesteps;
};


std::map<double, std::vector<VtkFile>> ReadVtk::readXmlCollection(const std::string &filename, bool piecesAsBlocks)
{
    std::map<double, std::vector<VtkFile>> timesteps;
    const std::array<std::string, 3> types{"Collection", "vtkMultiBlockDataSet", "PUnstructuredGrid"};

#ifdef HAVE_TINYXML2
    vistle::filesystem::path pvdpath(filename);
    if (!vistle::filesystem::is_regular_file(pvdpath)) {
        std::cerr << "readXmlCollection: " << filename << " is not a regular file" << std::endl;
        return timesteps;
    }
    xml::XMLDocument doc;
    doc.LoadFile(filename.c_str());
    auto VTKFile = doc.FirstChildElement("VTKFile");
    if (!VTKFile) {
        std::cerr << "readXmlCollection: did not find VTKFile element" << std::endl;
        return timesteps;
    }
    const char *typep = VTKFile->Attribute("type");
    if (!typep) {
        std::cerr << "readXmlCollection: VTKFile does not contain type" << std::endl;
        return timesteps;
    }

    const std::string type(typep);
    auto it = std::find(types.begin(), types.end(), type);
    if (it == types.end()) {
        std::cerr << "readXmlCollection: VTKFile not of required type:";
        for (auto &t: types)
            std::cerr << " '" << t << "'";
        std::cerr << std::endl;
        return timesteps;
    }

    auto Collection = VTKFile->FirstChildElement(type.c_str());
    if (!Collection) {
        std::cerr << "readXmlCollection: did not find Collection element" << std::endl;
        return timesteps;
    }
    if (auto DataSet = Collection->FirstChildElement("DataSet")) {
        do {
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
            const char *index = DataSet->Attribute("index");
            if (index && !part) {
                p = atoi(index);
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
                if (index)
                    vtkFile.index = index;
                vtkFile.realtime = time;
                if (piecesAsBlocks) {
                    vtkFile.pieces = getNumPieces(vtkFile.filename);
                }

                auto &timestep = timesteps[time];
                timestep.push_back(vtkFile);
            }


        } while ((DataSet = DataSet->NextSiblingElement("DataSet")));
    } else if (auto DataSet = Collection->FirstChildElement("Piece")) {
        do {
            if (const char *file = DataSet->Attribute("Source")) {
                VtkFile vtkFile;
                vistle::filesystem::path fp(file);
                if (fp.is_relative()) {
                    auto p = pvdpath.parent_path();
                    p /= fp;
                    vtkFile.filename = p.string();
                } else {
                    vtkFile.filename = file;
                }
                timesteps[ConstantTime].push_back(vtkFile);
            }
        } while ((DataSet = DataSet->NextSiblingElement("Piece")));
    }
#endif

    std::cerr << "readXmlCollection(" << filename << "): num timesteps: " << timesteps.size()
              << ", num blocks: " << timesteps.begin()->second.size() << std::endl;

    return timesteps;
}

void ReadVtk::setChoices(const VtkFile &fileinfo)
{
    std::vector<std::string> cellFields({Invalid});
    std::copy(fileinfo.cellfields.begin(), fileinfo.cellfields.end(), std::back_inserter(cellFields));
    for (int i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_cellDataChoice[i], cellFields);
    }
    std::vector<std::string> pointFields({Invalid});
    std::copy(fileinfo.pointfields.begin(), fileinfo.pointfields.end(), std::back_inserter(pointFields));
    for (int i = 0; i < NumPorts; ++i) {
        setParameterChoices(m_pointDataChoice[i], pointFields);
    }
}

ReadVtk::ReadVtk(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    auto gout = createOutputPort("grid_out", "grid or geometry");
    m_filename = addStringParameter("filename", "name of VTK file", "", Parameter::ExistingFilename);
    setParameterFilters(m_filename, "PVD Files (*.pvd)/"
                                    "XML VTK Files (*.vti *.vtp *.vtr *.vts *.vtu)/"
                                    "Parallel XML VTK Files(*.pvti *.pvtp *.pvtr *.pvts *.pvtu)/"
                                    "XML VTK Multiblock Data (*.vtm)/"
#if VTK_MAJOR_VERSION >= 9
                                    "VTK HDF Files (*.vtkhdf *.hdf)/"
#endif
                                    "Legacy VTK Files (*.vtk)");
    linkPortAndParameter(gout, m_filename);
    m_readPieces = addIntParameter("read_pieces", "create block for every piece in an unstructured grid", false,
                                   Parameter::Boolean);
    m_ghostCells = addIntParameter("create_ghost_cells", "create ghost cells for multi-piece unstructured grids", true,
                                   Parameter::Boolean);
    m_useCache = addIntParameter(
        "use_cache", "cache for improved performance for transient data on stationary grids (VTK HDF format only)",
        true, Parameter::Boolean);

    for (int i = 0; i < NumPorts; ++i) {
        std::stringstream spara;
        spara << "point_field_" << i;
        m_pointDataChoice[i] = addStringParameter(spara.str(), "point data field", "", Parameter::Choice);

        std::stringstream sport;
        sport << "point_data" << i;
        m_pointPort[i] = createOutputPort(sport.str(), "vertex data");

        linkPortAndParameter(m_pointPort[i], m_pointDataChoice[i]);
    }
    for (int i = 0; i < NumPorts; ++i) {
        std::stringstream spara;
        spara << "cell_field_" << i;
        m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);

        std::stringstream sport;
        sport << "cell_data" << i;
        m_cellPort[i] = createOutputPort(sport.str(), "cell data");

        linkPortAndParameter(m_cellPort[i], m_cellDataChoice[i]);
    }

    setParallelizationMode(ParallelizeTimeAndBlocks);
    observeParameter(m_filename);
    observeParameter(m_readPieces);
}

bool ReadVtk::examine(const vistle::Parameter *param)
{
    if (param == nullptr || param == m_filename || param == m_readPieces) {
        m_files.clear();
    }

    bool readPieces = m_readPieces->getValue();

    int maxNumPieces = 0;
    const std::string filename = m_filename->getValue();
    if (isCollectionFile(filename)) {
#ifndef HAVE_TINYXML2
        sendError("not compiled against TinyXML2 - no support for parsing XML collection files");
#endif

        auto timesteps = readXmlCollection(m_filename->getValue(), readPieces);
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

    if (isCollectionFile(filename)) {
#ifndef HAVE_TINYXML2
        sendError("not compiled against TinyXML2 - no support for parsing XML collection files");
#endif

        m_d->timesteps = readXmlCollection(filename, readPieces);
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

bool ReadVtk::finishRead()
{
    delete m_d;
    m_d = nullptr;

    return true;
}

bool ReadVtk::read(Token &token, int timestep, int block)
{
    const bool readPieces = m_readPieces->getValue();
    const bool ghostCells = m_ghostCells->getValue();
    const bool useCache = m_useCache->getValue();

    ReadOptions options;
    options.ghost = ghostCells;
    options.useCache = useCache;

    vistle::Meta m;
    m.setBlock(block);
    m.setNumBlocks(token.meta().numBlocks());
    m.setTimeStep(timestep);
    m.setNumTimesteps(token.meta().numTimesteps());

    if (m_d->timesteps.empty()) {
        const std::string filename = m_filename->getValue();
        if (readPieces) {
            return load(token, filename, options, m, block);
        } else {
            return load(token, filename, options);
        }
    } else {
        bool constant = true;
        double t = ConstantTime;
        if (timestep >= 0) {
            if (size_t(timestep) >= m_d->times.size())
                return false;

            constant = false;
            t = m_d->times[timestep];
        }

        //std::cerr << "Reading t=" << timestep << " (#=" << token.meta().numTimesteps() << ") , block=" << block << " (#=" << token.meta().numBlocks() << ")" << std::endl;

        auto it = m_d->timesteps.find(t);
        if (it != m_d->timesteps.end()) {
            int b = 0;
            for (auto f: it->second) {
                if (!constant)
                    m.setRealTime(f.realtime);

                if (b <= block && block < b + f.pieces) {
                    if (!load(token, f.filename, options, m, readPieces ? block - b : -1, f.part))
                        return false;
                }

                b += f.pieces;
            }
        }
    }

    return true;
}

bool ReadVtk::load(Token &token, const std::string &filename, const ReadOptions &options, const vistle::Meta &meta,
                   int piece, const std::string &part) const
{
    auto ds_pieces = getDataSet(filename, piece, options);
    auto dobj = ds_pieces.dataset;
    if (!dobj) {
        sendError("could not read data set '%s'", filename.c_str());
        return false;
    }

    std::string diagnostics;
    auto grid = vistle::vtk::toGrid(dobj, &diagnostics);
    if (grid) {
        if (!part.empty())
            grid->addAttribute(vistle::attribute::Part, part);
    }
    token.applyMeta(grid);
    token.addObject("grid_out", grid);
    if (!diagnostics.empty()) {
        sendWarning("grid: %s", diagnostics.c_str());
        diagnostics.clear();
    }

    vtkFieldData *fieldData = dobj->GetFieldData();
    vtkDataSetAttributes *pointData = nullptr;
    vtkDataSetAttributes *cellData = nullptr;
    if (auto ds = vtkDataSet::SafeDownCast(dobj)) {
        pointData = ds->GetPointData();
        cellData = ds->GetCellData();
    }
    for (int i = 0; i < NumPorts; ++i) {
        if (cellData && m_cellDataChoice[i]->getValue() != Invalid) {
            std::string name = m_cellDataChoice[i]->getValue();
            auto field = vistle::vtk::getField(cellData, name, grid, &diagnostics);
            if (!diagnostics.empty()) {
                sendWarning("field %s: %s", name.c_str(), diagnostics.c_str());
                diagnostics.clear();
            }
            if (field) {
                field->describe(name, id());
                field->setMapping(vistle::DataBase::Element);
                field->setGrid(grid);
                if (!part.empty())
                    field->addAttribute(vistle::attribute::Part, part);
            }
            token.applyMeta(field);
            token.addObject(m_cellPort[i], field);
        }

        if (pointData && m_pointDataChoice[i]->getValue() != Invalid) {
            std::string name = m_pointDataChoice[i]->getValue();
            auto field = vistle::vtk::getField(pointData, name, grid, &diagnostics);
            if (!diagnostics.empty()) {
                sendWarning("field %s: %s", name.c_str(), diagnostics.c_str());
                diagnostics.clear();
            }
            if (!field) {
                field = vistle::vtk::getField(fieldData, name, grid, &diagnostics);
                if (!diagnostics.empty()) {
                    sendWarning("field %s: %s", name.c_str(), diagnostics.c_str());
                    diagnostics.clear();
                }
            }
            if (field) {
                field->describe(name, id());
                field->setMapping(vistle::DataBase::Vertex);
                field->setGrid(grid);
                if (!part.empty())
                    field->addAttribute(vistle::attribute::Part, part);
            }
            token.applyMeta(field);
            token.addObject(m_pointPort[i], field);
        }
    }

    return true;
}
