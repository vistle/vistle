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
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
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

MODULE_MAIN(ReadVtk)

using namespace vistle;

const std::string Invalid("(NONE)");

const double ConstantTime = -std::numeric_limits<double>::max();

struct VtkFile {
    std::string filename;
    std::string part;
    std::string index;
    int partNum = -1;
    int pieces = 1;
    double realtime = ConstantTime;
    vtkDataObject *dataset = nullptr;
    std::vector<std::string> pointfields;
    std::vector<std::string> cellfields;
};

bool isCollectionFile(const std::string &fn)
{
    constexpr const char *collectionEndings[3] = {".pvd", ".vtm", ".pvtu"};
    for (const auto ending: collectionEndings)
        if (boost::algorithm::ends_with(fn, ending))
            return true;

    return false;
}

template<class Reader>
VtkFile readFile(const std::string &filename, int piece = -1, bool ghost = false, bool onlyMeta = false)
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
    reader->SetFileName(filename.c_str());
    reader->UpdateInformation();
    int numPieces = 1;
#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION > 7
    if (auto unstr = vtkXMLUnstructuredDataReader::SafeDownCast(reader)) {
        numPieces = unstr->GetNumberOfPieces();
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

    if (onlyMeta) {
        if (reader->GetOutput()) {
            reader->GetOutput()->Register(reader);
            result.dataset = reader->GetOutput();
        }
        return result;
    }

#if (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION >= 1) || VTK_MAJOR_VERSION > 7
    if (piece >= 0 && numPieces > 1) {
        auto info = vtkSmartPointer<vtkInformation>::New();
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
        info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghost ? 1 : 0);
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


VtkFile getDataSet(const std::string &filename, int piece = -1, bool ghost = false, bool onlyMeta = false)
{
    VtkFile fileinfo;
    bool triedLegacy = false;
    if (boost::algorithm::ends_with(filename, ".vtk")) {
        fileinfo = readFile<vtkDataSetReader>(filename, piece, ghost, onlyMeta);
        triedLegacy = true;
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLUnstructuredGridReader>(filename, piece, ghost, onlyMeta);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLMultiBlockDataReader>(filename, piece, ghost, onlyMeta);
    }
    if (!fileinfo.dataset) {
        fileinfo = readFile<vtkXMLGenericDataObjectReader>(filename, piece, ghost, onlyMeta);
    }
    if (!triedLegacy && !fileinfo.dataset) {
        fileinfo = readFile<vtkDataSetReader>(filename, piece, ghost, onlyMeta);
    }
    return fileinfo;
}

VtkFile getDataSetMeta(const std::string &filename)
{
    return getDataSet(filename, -1, false, true);
}

int getNumPieces(const std::string &filename)
{
    return getDataSetMeta(filename).pieces;
}

struct ReadVtkData {
    std::vector<double> times;
    std::map<double, std::vector<VtkFile>> timesteps;
};


std::map<double, std::vector<VtkFile>> readXmlCollection(const std::string &filename, bool piecesAsBlocks = false)
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
    createOutputPort("grid_out");
    m_filename = addStringParameter("filename", "name of VTK file", "", Parameter::ExistingFilename);
    setParameterFilters(m_filename,
                        "PVD Files (*.pvd)/XML VTK Files (*.vti *.vtp *.vtr *.vts *.vtu *.pvtu)/Legacy VTK Files "
                        "(*.vtk)/XML VTK Multiblock Data (*.vtm)/All Files (*)");
    m_readPieces = addIntParameter("read_pieces", "create block for every piece in an unstructured grid", true,
                                   Parameter::Boolean);
    m_ghostCells = addIntParameter("create_ghost_cells", "create ghost cells for multi-piece unstructured grids", true,
                                   Parameter::Boolean);

    for (int i = 0; i < NumPorts; ++i) {
        std::stringstream spara;
        spara << "point_field_" << i;
        m_pointDataChoice[i] = addStringParameter(spara.str(), "point data field", "", Parameter::Choice);

        std::stringstream sport;
        sport << "point_data" << i;
        m_pointPort[i] = createOutputPort(sport.str(), "vertex data");
    }
    for (int i = 0; i < NumPorts; ++i) {
        std::stringstream spara;
        spara << "cell_field_" << i;
        m_cellDataChoice[i] = addStringParameter(spara.str(), "cell data field", "", Parameter::Choice);

        std::stringstream sport;
        sport << "cell_data" << i;
        m_cellPort[i] = createOutputPort(sport.str(), "cell data");
    }

    setParallelizationMode(ParallelizeTimeAndBlocks);
    observeParameter(m_filename);
    observeParameter(m_readPieces);
}

ReadVtk::~ReadVtk()
{}

bool ReadVtk::examine(const Parameter *param)
{
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

bool ReadVtk::read(Reader::Token &token, int timestep, int block)
{
    const bool readPieces = m_readPieces->getValue();
    const bool ghostCells = m_ghostCells->getValue();

    Meta m;
    m.setBlock(block);
    m.setNumBlocks(token.meta().numBlocks());
    m.setTimeStep(timestep);
    m.setNumTimesteps(token.meta().numTimesteps());

    if (m_d->timesteps.empty()) {
        const std::string filename = m_filename->getValue();
        if (readPieces) {
            return load(token, filename, m, block, ghostCells);
        } else {
            return load(token, filename);
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
                    if (!load(token, f.filename, m, readPieces ? block - b : -1, ghostCells, f.part))
                        return false;
                }

                b += f.pieces;
            }
        }
    }

    return true;
}

bool ReadVtk::load(Token &token, const std::string &filename, const Meta &meta, int piece, bool ghost,
                   const std::string &part) const
{
    auto ds_pieces = getDataSet(filename, piece, ghost);
    auto dobj = ds_pieces.dataset;
    if (!dobj) {
        sendError("could not read data set '%s'", filename.c_str());
        return false;
    }

    auto grid = vistle::vtk::toGrid(dobj, checkConvexity());
    if (grid) {
        if (!part.empty())
            grid->addAttribute("_part", part);
    }
    token.applyMeta(grid);
    token.addObject("grid_out", grid);

    vtkFieldData *fieldData = dobj->GetFieldData();
    vtkDataSetAttributes *pointData = nullptr;
    vtkDataSetAttributes *cellData = nullptr;
    if (auto ds = vtkDataSet::SafeDownCast(dobj)) {
        pointData = ds->GetPointData();
        cellData = ds->GetCellData();
    }
    for (int i = 0; i < NumPorts; ++i) {
        if (cellData && m_cellDataChoice[i]->getValue() != Invalid) {
            auto field = vistle::vtk::getField(cellData, m_cellDataChoice[i]->getValue(), grid);
            if (field) {
                field->addAttribute("_species", m_cellDataChoice[i]->getValue());
                field->setMapping(DataBase::Element);
                if (!part.empty())
                    field->addAttribute("_part", part);
            }
            token.applyMeta(field);
            token.addObject(m_cellPort[i], field);
        }

        if (pointData && m_pointDataChoice[i]->getValue() != Invalid) {
            auto field = vistle::vtk::getField(pointData, m_pointDataChoice[i]->getValue(), grid);
            if (!field) {
                field = vistle::vtk::getField(fieldData, m_pointDataChoice[i]->getValue(), grid);
            }
            if (field) {
                field->addAttribute("_species", m_pointDataChoice[i]->getValue());
                field->setMapping(DataBase::Vertex);
                if (!part.empty())
                    field->addAttribute("_part", part);
            }
            token.applyMeta(field);
            token.addObject(m_pointPort[i], field);
        }
    }

    return true;
}
