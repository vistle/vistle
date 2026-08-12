#include "ReadEnsight.h"
#include <vistle/core/parameter.h>
#include <vistle/util/filesystem.h>
#include <vistle/util/byteswap.h>
#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>

#include "CaseFile.h"
#include "EnFile.h"
#include "CaseParserDriver.h"
#include "vistle/module/reader.h"
#include <iostream>

namespace {
const std::string NONE{vistle::Reader::InvalidChoice};
}

using namespace vistle;

MODULE_MAIN(ReadEnsight)

ReadEnsight::ReadEnsight(const std::string &name, int moduleID, mpi::communicator comm): Reader(name, moduleID, comm)
{
    m_casefile = addStringParameter("casefile", "EnSight case file", "", Parameter::ExistingFilename);
    setParameterFilters(m_casefile, "Case Files (*.case *.CASE *.encas)");

    m_partSelection = addStringParameter("parts", "select parts", "all", Parameter::Restraint);

    m_grid = createOutputPort("grid_out", "geometry");
    for (int port = 0; port < NumVolVert; ++port) {
        m_vol_vert[port] = createOutputPort("vol_vert_field" + std::to_string(port) + "_out",
                                            "per-vertex volume field " + std::to_string(port));
        m_vol_vert_choice[port] = addStringParameter("vol_vert_field" + std::to_string(port),
                                                     "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_vol_vert_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_vol_vert[port], m_vol_vert_choice[port]);
    }
    for (int port = 0; port < NumVolElem; ++port) {
        m_vol_elem[port] = createOutputPort("vol_elem_field" + std::to_string(port) + "_out",
                                            "per-element volume field " + std::to_string(port));
        m_vol_elem_choice[port] = addStringParameter("vol_elem_field" + std::to_string(port),
                                                     "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_vol_elem_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_vol_elem[port], m_vol_elem_choice[port]);
    }

    m_surf = createOutputPort("surface_out", "surface");
    for (int port = 0; port < NumSurfVert; ++port) {
        m_surf_vert[port] = createOutputPort("surface_vert_field" + std::to_string(port) + "_out",
                                             "per-vertex surface field " + std::to_string(port));
        m_surf_vert_choice[port] = addStringParameter("surf_vert_field" + std::to_string(port),
                                                      "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_surf_vert_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_surf_vert[port], m_surf_vert_choice[port]);
    }
    for (int port = 0; port < NumSurfElem; ++port) {
        m_surf_elem[port] = createOutputPort("surf_elem_field" + std::to_string(port) + "_out",
                                             "per-element surface field " + std::to_string(port));
        m_surf_elem_choice[port] = addStringParameter("surf_elem_field" + std::to_string(port),
                                                      "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_surf_elem_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_surf_elem[port], m_surf_elem_choice[port]);
    }
    m_curve = createOutputPort("curve_out", "curve");
    for (int port = 0; port < NumCurveVert; ++port) {
        m_curve_vert[port] = createOutputPort("curve_vert_field" + std::to_string(port) + "_out",
                                              "per-vertex curve field " + std::to_string(port));
        m_curve_vert_choice[port] = addStringParameter("curve_vert_field" + std::to_string(port),
                                                       "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_curve_vert_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_curve_vert[port], m_curve_vert_choice[port]);
    }
    for (int port = 0; port < NumCurveElem; ++port) {
        m_curve_elem[port] = createOutputPort("curve_elem_field" + std::to_string(port) + "_out",
                                              "per-element curve field " + std::to_string(port));
        m_curve_elem_choice[port] = addStringParameter("curve_elem_field" + std::to_string(port),
                                                       "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_curve_elem_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_curve_elem[port], m_curve_elem_choice[port]);
    }
    m_points = createOutputPort("points_out", "points");
    for (int port = 0; port < NumPointVert; ++port) {
        m_point_vert[port] = createOutputPort("point_vert_field" + std::to_string(port) + "_out",
                                              "per-vertex point field " + std::to_string(port));
        m_point_vert_choice[port] = addStringParameter("point_vert_field" + std::to_string(port),
                                                       "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_point_vert_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_point_vert[port], m_point_vert_choice[port]);
    }
    for (int port = 0; port < NumPointElem; ++port) {
        m_point_elem[port] = createOutputPort("point_elem_field" + std::to_string(port) + "_out",
                                              "per-element point field " + std::to_string(port));
        m_point_elem_choice[port] = addStringParameter("point_elem_field" + std::to_string(port),
                                                       "field " + std::to_string(port), NONE, Parameter::Choice);
        setParameterChoices(m_point_elem_choice[port], std::vector<std::string>({NONE}));
        linkPortAndParameter(m_point_elem[port], m_point_elem_choice[port]);
    }

    setCurrentParameterGroup("Details");
    m_earlyPartList =
        addIntParameter("early_partlist", "create part list before reading geometry", true, Parameter::Boolean);
    m_caseVerbose = addIntParameter("verbose_parser", "enable parser debug output", false, Parameter::Boolean);
    m_dataBigEndianParam =
        addIntParameter("file_big_endian", "file is in big endian format", m_dataBigEndian, Parameter::Boolean);
    setParameterReadOnly(m_dataBigEndianParam, true);

    observeParameter(m_casefile);
    observeParameter(m_earlyPartList);

    setParallelizationMode(ParallelizeTimeAndBlocksAfterStatic);
}

bool ReadEnsight::examine(const vistle::Parameter *param)
{
    m_earlyParts.clear();
    m_masterParts.clear();
    m_globalParts.clear();

    if (!param || param == m_casefile || param == m_earlyPartList) {
        std::string file = m_casefile->getValue();
        filesystem::path path(file);
        {
            m_case = CaseFile();
            CaseParserDriver parser(file);
            // uncomment this line to see more debug output
            parser.setVerbose(m_caseVerbose->getValue() != 0);
            if (!parser.isOpen() || !parser.parse()) {
                std::string err = parser.lastError();
                sendError("Cannot parse case file %s: %s", file.c_str(), err.c_str());
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

        setFieldChoices(m_vol_vert_choice, NumVolVert, vert_choices, false);
        setFieldChoices(m_vol_elem_choice, NumVolElem, elem_choices, false);
        setFieldChoices(m_surf_vert_choice, NumSurfVert, vert_choices, false);
        setFieldChoices(m_surf_elem_choice, NumSurfElem, elem_choices, false);
        setFieldChoices(m_curve_vert_choice, NumCurveVert, vert_choices, false);
        setFieldChoices(m_curve_elem_choice, NumCurveElem, elem_choices, false);
        setFieldChoices(m_point_vert_choice, NumPointVert, vert_choices, false);
        setFieldChoices(m_point_elem_choice, NumPointElem, elem_choices, false);

        auto times = m_case.getAllRealTimes();
        setTimesteps(times.size());

        if (m_earlyPartList->getValue() != 0) {
            makeGeoFiles();
            size_t ntimes = times.size();
            if (ntimes == 0)
                ntimes = 1;
            m_globalParts.resize(ntimes);
            if (!createPartlists(-1, true)) {
                m_globalParts.clear();
                return false;
            }
            m_earlyParts = m_globalParts[0];
            sendPartsToInfo(m_earlyParts);

            if (!hasPartWithDim(3)) {
                setFieldChoices(m_vol_vert_choice, NumVolVert, vert_choices, true);
                setFieldChoices(m_vol_elem_choice, NumVolElem, elem_choices, true);
            }
            if (!hasPartWithDim(2)) {
                setFieldChoices(m_surf_vert_choice, NumSurfVert, vert_choices, true);
                setFieldChoices(m_surf_elem_choice, NumSurfElem, elem_choices, true);
            }
            if (!hasPartWithDim(1)) {
                setFieldChoices(m_curve_vert_choice, NumCurveVert, vert_choices, true);
                setFieldChoices(m_curve_elem_choice, NumCurveElem, elem_choices, true);
            }
            if (!hasPartWithDim(0)) {
                setFieldChoices(m_point_vert_choice, NumPointVert, vert_choices, true);
                setFieldChoices(m_point_elem_choice, NumPointElem, elem_choices, true);
            }
        }
    }

    return true;
}

void ReadEnsight::sendPartsToInfo(const PartList &partList) const
{
    std::string info;
    info += EnPart::partInfoHeader();

    int block(0);
    for (auto pos = partList.begin(); pos != partList.end(); pos++) {
        info += pos->partInfoString(block);
        ++block;
    }
    info += EnPart::partInfoFooter();
    sendInfo(info);
}

bool ReadEnsight::makeGeoFiles()
{
    // set filenames
    std::string geoFileName = m_case.getGeoFileNm();
    if (geoFileName.empty()) {
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

    size_t ntimes = times.size();
    if (ntimes == 0)
        ntimes = 1;
    m_globalParts.clear();
    m_globalParts.resize(ntimes);
    m_globalParts[0] = m_earlyParts;
    m_masterParts = m_earlyParts;

    if (!createPartlists(-1)) {
        return false;
    }
    sendPartsToInfo(m_globalParts[0]);

    m_selectedParts.clear();
    m_selectedParts.add(m_partSelection->getValue());

    size_t numParts(m_globalParts[0].size());
    size_t numActiveParts = 0;
    std::array<size_t, 4> numActive{};
    for (size_t i = 0; i < numParts; i++) {
        const auto &p = m_globalParts[0][i];
        auto partNum = p.getPartNum();
        bool active = m_selectedParts(partNum);
        if (active) {
            ++numActiveParts;
            for (int d = 0; d < 4; ++d) {
                if (p.getTotNumEle(d) > 0)
                    ++numActive[d];
            }
        }
    }
    setPartitions(numActiveParts);

    std::array<std::pair<EnFile::ReadType, std::string>, 4> dims{
        {{EnFile::POINT, "point"}, {EnFile::CURVE, "curve"}, {EnFile::SURFACE, "surface"}, {EnFile::VOLUME, "volume"}}};

    for (int d = 0; d < 4; ++d) {
        if (numActive[d] == 0) {
            if (!getActiveFields(dims[d].first).empty()) {
                sendWarning("%s/%dD ports connected, but no %dD parts selected", dims[d].second.c_str(), d, d);
            }
        }
    }

    setParallelizationMode(m_case.hasChangingGeometryPerPart() ? ParallelizeBlocks
                                                               : ParallelizeTimeAndBlocksAfterStatic);

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

    if (what & EnFile::VOLUME) {
        collectFields(m_vol_vert, m_vol_vert_choice, NumVolVert, fields);
        collectFields(m_vol_elem, m_vol_elem_choice, NumVolElem, fields);
    }
    if (what & EnFile::SURFACE) {
        collectFields(m_surf_vert, m_surf_vert_choice, NumSurfVert, fields);
        collectFields(m_surf_elem, m_surf_elem_choice, NumSurfElem, fields);
    }
    if (what & EnFile::CURVE) {
        collectFields(m_curve_vert, m_curve_vert_choice, NumCurveVert, fields);
        collectFields(m_curve_elem, m_curve_elem_choice, NumCurveElem, fields);
    }
    if (what & EnFile::POINT) {
        collectFields(m_point_vert, m_point_vert_choice, NumPointVert, fields);
        collectFields(m_point_elem, m_point_elem_choice, NumPointElem, fields);
    }

    std::cerr << "Reading " << fields.size() << " fields (enabled dims=" << what << ")" << std::endl;
    for (const auto &f: fields) {
        std::cerr << "  " << f.first->getName() << ": " << f.second << std::endl;
    }

    return fields;
}

void ReadEnsight::collectFields(Port **ports, StringParameter **choices, int n,
                                std::vector<std::pair<Port *, std::string>> &fields)
{
    for (int i = 0; i < n; ++i) {
        auto choice = choices[i]->getValue();
        if (choice.empty() || choice == NONE)
            continue;
        if (!ports[i]->isConnected())
            continue;
        fields.emplace_back(ports[i], choice);
    }
}

void ReadEnsight::setFieldChoices(StringParameter **choices, int n, const std::vector<std::string> &cs, bool readOnly)
{
    for (int i = 0; i < n; ++i) {
        setParameterChoices(choices[i], cs);
        setParameterReadOnly(choices[i], readOnly);
    }
}

bool ReadEnsight::read(Reader::Token &token, int timestep, int block)
{
    if (timestep < 0 && numTimesteps() == m_geoFiles.size()) {
        return true;
    }

    std::string geofile;
    if (timestep < 0) {
        assert(m_geoFiles.size() == 1);
        geofile = m_geoFiles[0];
    } else if (timestep >= 0 && numTimesteps() == m_geoFiles.size()) {
        geofile = m_geoFiles[timestep];
    } else {
        assert(m_geoFiles.size() == 1);
    }
    std::cerr << "reading t=" << timestep << ", block=" << block << ":"
              << " geo=" << geofile << std::endl;

    PartList *curPartList = &m_globalParts[0];
    if (timestep >= 0) {
        assert(timestep < m_globalParts.size());
        curPartList = &m_globalParts[timestep];
    }

    auto findBlock = [this](PartList &parts, int block) {
        int curNum = 0;
        for (size_t idx = 0; idx < parts.size(); ++idx) {
            auto &p = parts[idx];
            auto partNum = p.getPartNum();
            bool active = m_selectedParts(partNum);
            if (active) {
                if (curNum == block) {
                    return std::make_pair(&p, int(idx));
                }
                ++curNum;
            }
        }
        return std::make_pair(static_cast<EnPart *>(nullptr), -1);
    };

    auto [curPart, curPartIdx] = findBlock(*curPartList, block);
    if (!curPart) {
        sendError("part for timestep %d, block %d not found", timestep, block);
        return false;
    }

    EnPart *refPart = nullptr;
    if (m_case.hasChangingGeometryPerPart() && timestep > 0) {
        PartList *prevList = &m_masterParts;
        if (timestep > 0) {
            assert(timestep < m_globalParts.size());
            prevList = &m_globalParts[timestep - 1];
        }
        refPart = findBlock(*prevList, block).first;
    }

    Object::const_ptr geo;
    if (!geofile.empty()) {
        auto file = EnFile::createGeometryFile(this, m_case, geofile);
        if (!file) {
            sendError("failed to open geo %s", geofile.c_str());
            return false;
        }
        std::cerr << "successfully opened geo " << geofile << std::endl;
        auto binType = file->binType();
        m_case.setBinType(binType);

        file->setPartList(curPartList);
        Object::ptr grid = file->read(timestep, block, curPart, refPart);
        if (!grid) {
            sendError("failed to read geometry from %s", geofile.c_str());
            return false;
        }
        std::cerr << "read geo " << geofile << ":" << curPart->partInfoString(-1) << std::endl;

        token.applyMeta(grid);
        grid->addAttribute(attribute::Part, curPart->comment());

        if (auto unstr = UnstructuredGrid::as(grid)) {
            token.addObject(m_grid, unstr);
        } else if (auto poly = Polygons::as(grid)) {
            token.addObject(m_surf, poly);
        } else if (auto lines = Lines::as(grid)) {
            token.addObject(m_curve, lines);
        } else if (auto pts = Points::as(grid)) {
            token.addObject(m_points, pts);
        }

        if (timestep < 0) {
            m_constantGeo[block] = grid;
            m_masterParts[curPartIdx] = *curPart; // now has blankslists; update master
        }
        geo = grid;
    } else {
        geo = m_constantGeo[block];
    }

    EnFile::ReadType what = EnFile::NOTHING;
    if (auto unstr = UnstructuredGrid::as(geo)) {
        what = EnFile::VOLUME;
    } else if (auto poly = Polygons::as(geo)) {
        what = EnFile::SURFACE;
    } else if (Lines::as(geo)) {
        what = EnFile::CURVE;
    } else if (Points::as(geo)) {
        what = EnFile::POINT;
    } else {
        std::cerr << "no grid for data t=" << timestep << ", block=" << block << std::endl;
        return true;
    }

    if (timestep < 0 && numTimesteps() > 0) {
        return true;
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
            continue;
        }
        file->setPartList(curPartList);
        auto data = file->read(timestep, block, curPart);
        file.reset();
        if (data) {
            data->addAttribute(attribute::Part, curPart->comment());
            token.applyMeta(data);
            if (auto db = DataBase::as(data)) {
                db->describe(field, id());
                auto di = m_case.getDataItem(field);
                if (di) {
                    db->setMapping(di->perVertex() ? DataBase::Vertex : DataBase::Element);
                }
                db->setGrid(geo);
                token.addObject(port, db);
                std::cerr << "read data: " << *db << std::endl;
            }
        }
    }

    return true;
}

bool ReadEnsight::finishRead()
{
    m_constantGeo.clear();
    m_globalParts.clear();
    m_masterParts.clear();
    if (m_earlyPartList->getValue() == 0) {
        m_earlyParts.clear();
    } else {
        m_globalParts.push_back(m_earlyParts);
        auto times = m_case.getAllRealTimes();
        m_globalParts.resize(times.size());
    }
    return true;
}

// Create a list of all EnSight parts ( of the first timestep in time dependent data )
// This is the master part
// Write a table of all parts in the master part list  to the info channel
bool ReadEnsight::createPartlists(int timestep, bool onlyGeo)
{
    // we read only the first geometry file and assume that for moving geometries
    // the split-up in parts does not change

    size_t idx = timestep < 0 ? 0 : size_t(timestep);

    if (m_geoFiles.empty()) {
        return false;
    }

    bool earlyPartList = m_earlyPartList->getValue() != 0;

    assert(m_globalParts.size() > idx);
    auto &parts = m_globalParts[idx];

    if (onlyGeo || idx > 0 || !earlyPartList) {
        if (m_geoFiles.size() > idx) {
            auto &fName = m_geoFiles[idx];
            // create file object
            auto enf = EnFile::createGeometryFile(this, m_case, fName);
            if (!enf) {
                sendError("Could not open geometry file %s", fName.c_str());
                return false;
            }

            parts.clear();
            enf->setPartList(&parts);
            // this creates the table and extracts the part infos
            if (!enf->parseForParts()) {
                parts.clear();

                if (timestep >= 0 || !enf->mayBeCorrupt()) {
                    sendError("Could not parse geometry file %s", fName.c_str());
                    return false;
                }

                sendInfo("Could not parse geometry file %s - retrying with changed endianness", fName.c_str());
                m_dataBigEndian = !m_dataBigEndian;
                enf = EnFile::createGeometryFile(this, m_case, fName);
                if (!enf) {
                    sendError("Could not open geometry file %s", fName.c_str());
                    m_dataBigEndian = false;
                    return false;
                }
                enf->setPartList(&parts);
                if (!enf->parseForParts()) {
                    sendError("Could not find any parts in geometry file %s", fName.c_str());
                    parts.clear();
                    m_dataBigEndian = false;
                    return false;
                }
                setParameter(m_dataBigEndianParam, Integer(m_dataBigEndian));
            }
            assert(m_dataBigEndian == m_dataBigEndianParam->getValue());

            if (enf->mayBeCorrupt()) {
                sendError("Could not parse geometry file %s", fName.c_str());
                parts.clear();
                return false;
            }

            auto binType = enf->binType();
            m_case.setBinType(binType);

            if (timestep < 0) {
                m_masterParts = parts;
            }
        } else {
            // geometry reused from first timestep
            parts = m_masterParts;
        }
    }

    if (onlyGeo) {
        return true;
    }

    auto fields = getActiveFields(EnFile::ALL);
    std::set<std::string> processed;
    for (size_t i = 0; i < fields.size(); ++i) {
        auto &field = fields[i].second;
        if (processed.find(field) != processed.end()) {
            continue;
        }

        auto file = EnFile::createDataFile(this, m_case, field, timestep);
        if (!file) {
            sendError("Could not open file for field %s", field.c_str());
            return false;
        }
        file->setPartList(&parts);
        if (!file->parseForParts()) {
            std::string fname = file->name();
            sendError("Could not parse field file %s for field %s", fname.c_str(), field.c_str());
            parts.clear();
            return false;
        }
        //file->sendPartsToInfo();

        processed.insert(field);
    }

    return true;
}

bool ReadEnsight::hasPartWithDim(int dim) const
{
    for (const auto &pl: m_globalParts) {
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
