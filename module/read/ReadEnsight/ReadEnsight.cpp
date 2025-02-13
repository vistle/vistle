#include "ReadEnsight.h"
#include <vistle/core/parameter.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/byteswap.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>

#include "CaseFile.h"
#include "EnFile.h"
#include "CaseParserDriver.h"
#include <iostream>

namespace {
const std::string NONE("(none)");
}

using namespace vistle;

MODULE_MAIN(ReadEnsight)

ReadEnsight::ReadEnsight(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    m_casefile = addStringParameter("casefile", "EnSight case file", "", Parameter::ExistingFilename);
    setParameterFilters(m_casefile, "Case Files (*.case *.CASE *.encas)");

    m_partSelection = addStringParameter("parts", "select parts", "all");

    m_grid = createOutputPort("grid_out", "geometry");
    for (int port = 0; port < NumVolVert; ++port) {
        m_vol_vert[port] = createOutputPort("vol_vert_field" + std::to_string(port) + "_out",
                                            "per-vertex volume field " + std::to_string(port));
        m_vol_vert_choice[port] = addStringParameter("vol_vert_field" + std::to_string(port),
                                                     "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_vol_vert_choice[port], std::vector<std::string>({NONE}));
    }
    for (int port = 0; port < NumVolElem; ++port) {
        m_vol_elem[port] = createOutputPort("vol_elem_field" + std::to_string(port) + "_out",
                                            "per-element volume field " + std::to_string(port));
        m_vol_elem_choice[port] = addStringParameter("vol_elem_field" + std::to_string(port),
                                                     "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_vol_elem_choice[port], std::vector<std::string>({NONE}));
    }

    m_surf = createOutputPort("surface_out", "surface");
    for (int port = 0; port < NumSurfVert; ++port) {
        m_surf_vert[port] = createOutputPort("surface_vert_field" + std::to_string(port) + "_out",
                                             "per-vertex surface field " + std::to_string(port));
        m_surf_vert_choice[port] = addStringParameter("surf_vert_field" + std::to_string(port),
                                                      "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_surf_vert_choice[port], std::vector<std::string>({NONE}));
    }
    for (int port = 0; port < NumSurfElem; ++port) {
        m_surf_elem[port] = createOutputPort("surf_elem_field" + std::to_string(port) + "_out",
                                             "per-element surface field " + std::to_string(port));
        m_surf_elem_choice[port] = addStringParameter("surf_elem_field" + std::to_string(port),
                                                      "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_surf_elem_choice[port], std::vector<std::string>({NONE}));
    }

    m_earlyPartList =
        addIntParameter("early_partlist", "create part list before reading geometry", true, Parameter::Boolean);
    m_dataBigEndianParam =
        addIntParameter("file_big_endian", "file is in big endian format", m_dataBigEndian, Parameter::Boolean);
    setParameterReadOnly(m_dataBigEndianParam, true);

    observeParameter(m_casefile);
    observeParameter(m_earlyPartList);

    setParallelizationMode(ParallelizeTimeAndBlocks);
}

ReadEnsight::~ReadEnsight()
{}


bool ReadEnsight::examine(const vistle::Parameter *param)
{
    if (!param || param == m_casefile || param == m_earlyPartList) {
        std::string file = m_casefile->getValue();
        filesystem::path path(file);
        {
            m_case = CaseFile();
            CaseParserDriver parser(file);
            // uncomment this line to see more debug output
            //parser.setVerbose(true);
            if (!parser.isOpen() || !parser.parse()) {
                std::string err = parser.lastError();
                sendWarning("Cannot parse case file %s: %s", file.c_str(), err.c_str());
                return false;
            }

            m_case = parser.getCaseObj();
        }
        m_case.setFullFilename(file);
        if (m_case.empty()) {
            sendInfo("EnSight case %s is empty", file.c_str());
            return false;
        }

        if (m_case.getVersion() != CaseFile::gold) {
            sendInfo("EnSight case %s is not in Gold format: actual version %d, expected %d", file.c_str(),
                     m_case.getVersion(), CaseFile::gold);
            return false;
        }

        // Get data fields
        const auto &dl = m_case.getDataIts();

        // lists for Choice Labels
        std::vector<std::string> vert_choices({NONE}), elem_choices({NONE});

        // fill in all species for the appropriate Ports/Choices
        for (auto iter = dl.begin(); iter != dl.end(); ++iter) {
            auto &it = iter->second;
            // fill choice parameter of out-port for scalar data
            // that is token  DPORT1 DPORT2
            switch (it.getType()) {
            case DataItem::scalar:
            case DataItem::vector:
                if (it.perVertex())
                    vert_choices.push_back(it.getDesc());
                else
                    elem_choices.push_back(it.getDesc());
                break;
            case DataItem::tensor:
                // fill in ports for tensor data
                break;
            }
        }

        for (int i = 0; i < NumVolVert; ++i) {
            setParameterChoices(m_vol_vert_choice[i], vert_choices);
            setParameterReadOnly(m_vol_vert_choice[i], false);
        }
        for (int i = 0; i < NumVolElem; ++i) {
            setParameterChoices(m_vol_elem_choice[i], elem_choices);
            setParameterReadOnly(m_vol_elem_choice[i], false);
        }
        for (int i = 0; i < NumSurfVert; ++i) {
            setParameterChoices(m_surf_vert_choice[i], vert_choices);
            setParameterReadOnly(m_surf_vert_choice[i], false);
        }
        for (int i = 0; i < NumSurfElem; ++i) {
            setParameterChoices(m_surf_elem_choice[i], elem_choices);
            setParameterReadOnly(m_surf_elem_choice[i], false);
        }

        auto times = m_case.getAllRealTimes();
        setTimesteps(times.size());

        if (m_earlyPartList->getValue() != 0) {
            makeGeoFiles();
            size_t ntimes = times.size();
            if (ntimes == 0)
                ntimes = 1;
            globalParts_.resize(ntimes);
            if (!createPartlists(-1, true)) {
                return false;
            }

            if (!hasPartWithDim(3)) {
                for (int i = 0; i < NumVolVert; ++i) {
                    setParameterReadOnly(m_vol_vert_choice[i], true);
                }
                for (int i = 0; i < NumVolElem; ++i) {
                    setParameterReadOnly(m_vol_elem_choice[i], true);
                }
            }
            if (!hasPartWithDim(2)) {
                for (int i = 0; i < NumSurfVert; ++i) {
                    setParameterReadOnly(m_surf_vert_choice[i], true);
                }
                for (int i = 0; i < NumSurfElem; ++i) {
                    setParameterReadOnly(m_surf_elem_choice[i], true);
                }
            }
        }
    }

    return true;
}

bool ReadEnsight::makeGeoFiles()
{
    // set filenames
    std::string geoFileName = m_case.getGeoFileNm();
    if (geoFileName.length() < 5) {
        // FIXME?
        return false;
    }
    std::vector<std::string> allGeoFiles(m_case.makeFileNames(geoFileName, m_case.getGeoTimeSet()));
    size_t numTs(allGeoFiles.size());
    if (numTs == 0)
        allGeoFiles.push_back(m_case.getGeoFileNm());
    m_geoFiles = allGeoFiles;

    std::cerr << "geofiles (" << allGeoFiles.size() << " total):";
    for (auto f: allGeoFiles) {
        std::cerr << " " << f;
    }
    std::cerr << std::endl;

    return true;
}

bool ReadEnsight::prepareRead()
{
    auto times = m_case.getAllRealTimes();
    setTimesteps(times.size());
    sendInfo("Reading dataset with %d timesteps", (int)times.size());

    if (!makeGeoFiles()) {
        return false;
    }

    if (m_earlyPartList->getValue() == 0) {
        globalParts_.clear();
        size_t ntimes = times.size();
        if (ntimes == 0)
            ntimes = 1;
        globalParts_.resize(ntimes);
    }
    assert((times.size() == 0 && globalParts_.size() == 1) || globalParts_.size() == times.size());

    if (!createPartlists(-1)) {
        return false;
    }

    m_selectedParts.add(m_partSelection->getValue());

    size_t numParts(globalParts_[0].size());
    size_t numActiveParts = 0;
    for (size_t i = 0; i < numParts; i++) {
        bool active = m_selectedParts(i);
        if (active) {
            ++numActiveParts;
        }
    }

    setPartitions(numActiveParts);

    return true;
}

bool ReadEnsight::prepareTimestep(int timestep)
{
    if (timestep <= 0) {
        // part list already created in prepareRead()
        return true;
    }

    return createPartlists(timestep);
}

std::vector<std::pair<vistle::Port *, std::string>> ReadEnsight::getActiveFields(EnFile::ReadType what)
{
    std::vector<std::pair<vistle::Port *, std::string>> fields;

    std::vector<Port *> ports;
    std::vector<std::string> choices;

    if (what == EnFile::VOLUME || what == EnFile::VOLUME_AND_SURFACE) {
        for (int i = 0; i < NumVolVert; ++i) {
            auto field = m_vol_vert_choice[i];
            auto choice = field->getValue();
            if (choice.empty() || choice == NONE) {
                continue;
            }
            ports.push_back(m_vol_vert[i]);
            choices.push_back(choice);
            fields.emplace_back(m_vol_vert[i], choice);
        }
        for (int i = 0; i < NumVolElem; ++i) {
            auto field = m_vol_elem_choice[i];
            auto choice = field->getValue();
            if (choice.empty() || choice == NONE) {
                continue;
            }
            ports.push_back(m_vol_elem[i]);
            choices.push_back(choice);
            fields.emplace_back(m_vol_elem[i], choice);
        }
    } else if (what == EnFile::SURFACE || what == EnFile::VOLUME_AND_SURFACE) {
        for (int i = 0; i < NumSurfVert; ++i) {
            auto field = m_surf_vert_choice[i];
            auto choice = field->getValue();
            if (choice.empty() || choice == NONE) {
                continue;
            }
            ports.push_back(m_surf_vert[i]);
            choices.push_back(choice);
            fields.emplace_back(m_surf_vert[i], choice);
        }
        for (int i = 0; i < NumSurfElem; ++i) {
            auto field = m_surf_elem_choice[i];
            auto choice = field->getValue();
            if (choice.empty() || choice == NONE) {
                continue;
            }
            ports.push_back(m_surf_elem[i]);
            choices.push_back(choice);
            fields.emplace_back(m_surf_elem[i], choice);
        }
    }

    assert(ports.size() == choices.size());
    std::cerr << "Reading " << ports.size() << " fields" << std::endl;
    for (size_t i = 0; i < ports.size(); ++i) {
        std::cerr << "  " << ports[i]->getName() << ": " << choices[i] << std::endl;
    }

    return fields;
}

bool ReadEnsight::read(Reader::Token &token, int timestep, int block)
{
    if (m_geoFiles.size() > 1 && timestep < 0) {
        return true;
    }
    std::cerr << "reading t=" << timestep << ", block=" << block << ":" << std::endl;

    std::string geofile;
    if (timestep >= 0 && timestep < m_geoFiles.size()) {
        geofile = m_geoFiles[timestep];
    } else if (timestep < 0 && m_geoFiles.size() == 1) {
        geofile = m_geoFiles[0];
    } else {
        sendError("no geometry file name for timestep %d", timestep);
        return false;
    }
    if (geofile.empty()) {
        sendError("empty geometry file name for timestep %d", timestep);
        return false;
    }
    std::cerr << " geo=" << geofile << std::endl;

    PartList *curPartList = &globalParts_[0];
    if (timestep >= 0 && timestep < globalParts_.size()) {
        curPartList = &globalParts_[timestep];
    }

    EnPart curPart;
    int curNum = 0;
    int count = 0;
    for (const auto &p: *curPartList) {
        bool active = m_selectedParts(count);
        ++count;
        if (active) {
            if (curNum == block) {
                curPart = p;
                std::cerr << " reading for part: " << curPart << std::endl;
                break;
            }
            ++curNum;
        }
    }

    Object::ptr grid;
    auto what = EnFile::VOLUME_AND_SURFACE;
    {
        auto file = EnFile::createGeometryFile(this, m_case, geofile);
        if (!file) {
            sendError("failed to open geo %s", geofile.c_str());
            return false;
        }
        std::cerr << "successfully opened geo " << geofile << std::endl;
        auto binType = file->binType();
        m_case.setBinType(binType);

        file->setPartList(curPartList);
        grid = file->read(timestep, block, &curPart);
    }
    if (!grid) {
        sendError("failed to read geometry from %s", geofile.c_str());
        return false;
    }

    token.applyMeta(grid);
    grid->addAttribute("_part", curPart.comment());
    if (auto unstr = UnstructuredGrid::as(grid)) {
        token.addObject(m_grid, unstr);
        what = EnFile::VOLUME;
    } else if (auto poly = Polygons::as(grid)) {
        token.addObject(m_surf, poly);
        what = EnFile::SURFACE;
    }

    auto fields = getActiveFields(what);
    for (size_t i = 0; i < fields.size(); ++i) {
        auto &port = fields[i].first;
        auto &field = fields[i].second;

        auto file = EnFile::createDataFile(this, m_case, field, timestep);
        if (file) {
            std::cerr << "successfully opened field " << field << " for timestep " << timestep << ", outputting to "
                      << port->getName() << std::endl;
        } else {
            std::cerr << "failed to open field " << field << " for timestep " << timestep << ", outputting to "
                      << port->getName() << std::endl;
            return false;
        }
        file->setPartList(curPartList);
        auto data = file->read(timestep, block, &curPart);
        file.reset();
        if (data) {
            data->addAttribute("_part", curPart.comment());
            token.applyMeta(data);
            data->addAttribute("_species", field);
            if (auto db = DataBase::as(data)) {
                db->setGrid(grid);
                token.addObject(port, db);
                std::cerr << "read data: " << *db << std::endl;
            }
        }
    }

    return true;
}

bool ReadEnsight::finishRead()
{
    if (m_earlyPartList->getValue() == 0) {
        globalParts_.clear();
    }
    return true;
}

// Create a list of all EnSight parts ( of the first timestep in time dependent data )
// This is the master part
// Write a table of all parts in the master part list  to the info channel
bool ReadEnsight::createPartlists(int timestep, bool quick)
{
    // we read only the first geometry file and assume that for moving geometries
    // the split-up in parts does not change

    size_t idx = timestep < 0 ? 0 : size_t(timestep);

    if (m_geoFiles.empty()) {
        return false;
    }

    if (m_geoFiles.size() <= idx) {
        return true;
    }

    if (quick || timestep >= 0 || m_earlyPartList->getValue() == 0) {
        auto &fName = m_geoFiles[idx];
        // create file object
        auto enf = EnFile::createGeometryFile(this, m_case, fName);
        if (!enf) {
            sendError("Could not open geometry file %s", fName.c_str());
            return false;
        }

        assert(globalParts_.size() > idx);
        auto &parts = globalParts_[idx];

        enf->setPartList(&parts);
        // this creates the table and extracts the part infos
        if (!enf->parseForParts()) {
            sendError("Could not parse geometry file %s", fName.c_str());
            parts.clear();
            return false;
        }

        if (enf->fileMayBeCorrupt_) {
            if (timestep < 0) {
                sendInfo("GeoFile %s may be corrupt - retrying with changed endianess", fName.c_str());
                m_dataBigEndian = !m_dataBigEndian;
                parts.clear();
                enf = EnFile::createGeometryFile(this, m_case, fName);
                enf->setPartList(&parts);
                if (!enf->parseForParts()) {
                    sendError("GeoFile %s seems to be corrupt", fName.c_str());
                    parts.clear();
                    return false;
                }
                if (enf->fileMayBeCorrupt_) {
                    m_dataBigEndian = !m_dataBigEndian;
                } else {
                    setParameter(m_dataBigEndianParam, Integer(m_dataBigEndian));
                }
            }
        }
        assert(m_dataBigEndian == m_dataBigEndianParam->getValue());

        if (enf->fileMayBeCorrupt_) {
            sendError("GeoFile %s seems to be corrupt!! Check case file or geo file", fName.c_str());
            parts.clear();
            return false;
        }

        auto binType = enf->binType();
        m_case.setBinType(binType);

        if (timestep < 0) {
            enf->sendPartsToInfo();
        }
    }

    if (quick) {
        return true;
    }

    auto &parts = globalParts_[idx];
    auto fields = getActiveFields(EnFile::VOLUME_AND_SURFACE);
    std::set<std::string> processed;
    for (size_t i = 0; i < fields.size(); ++i) {
        auto &field = fields[i].second;
        if (processed.find(field) != processed.end()) {
            continue;
        }

        auto file = EnFile::createDataFile(this, m_case, field, timestep);
        if (!file) {
            sendError("Could not open field file %s", field.c_str());
            return false;
        }
        file->setPartList(&parts);
        if (file->parseForParts()) {
            file->sendPartsToInfo();
        }

        processed.insert(field);
    }

    return true;
}

bool ReadEnsight::hasPartWithDim(int dim) const
{
    for (const auto &pl: globalParts_) {
        if (::hasPartWithDim(pl, dim)) {
            return true;
        }
    }

    return false;
}

bool ReadEnsight::byteSwap() const
{
    return m_dataBigEndian ^ (vistle::host_endian == vistle::big_endian);
}
