#ifndef VISTLE_READENSIGHT_READENSIGHT_H
#define VISTLE_READENSIGHT_READENSIGHT_H

#include <vistle/module/reader.h>
#include <vistle/util/coRestraint.h>

#include "CaseFile.h"
#include "EnPart.h"
#include "EnFile.h"

#include <string>
#include <vector>

class ReadEnsight: public vistle::Reader {
    static const int NumVolVert = 2;
    static const int NumVolElem = 2;
    static const int NumSurfVert = 1;
    static const int NumSurfElem = 1;
    static const int NumCurveVert = 1;
    static const int NumCurveElem = 1;
    static const int NumPointVert = 1;
    static const int NumPointElem = 1;

public:
    ReadEnsight(const std::string &name, int moduleID, mpi::communicator comm);

    bool examine(const vistle::Parameter *param) override;
    bool prepareRead() override;
    bool prepareTimestep(int timestep) override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool finishRead() override;

    PartList m_earlyParts; // saves parts and their offsets into the geometry file for the first timestep, if requested
    PartList m_masterParts; // only  geometry parts, no fields
    std::vector<PartList> m_globalParts; // all parts for all timesteps
    bool hasPartWithDim(int dim) const;

    bool byteSwap() const;

private:
    // collect selected and connected fields of one port/choice group
    void collectFields(vistle::Port **ports, vistle::StringParameter **choices, int n,
                       std::vector<std::pair<vistle::Port *, std::string>> &fields);
    // set choices and read-only state of one choice group
    void setFieldChoices(vistle::StringParameter **choices, int n, const std::vector<std::string> &cs, bool readOnly);

    // write a list of parts to the map editor (info channel)
    bool createPartlists(int timestep, bool onlyGeo = false);
    void sendPartsToInfo(const PartList &partList) const;
    bool makeGeoFiles();

    std::vector<std::pair<vistle::Port *, std::string>> getActiveFields(EnFile::ReadType what);

    vistle::StringParameter *m_casefile = nullptr;
    vistle::IntParameter *m_caseVerbose = nullptr;
    vistle::StringParameter *m_partSelection = nullptr;
    vistle::IntParameter *m_earlyPartList = nullptr;
    vistle::IntParameter *m_dataBigEndianParam = nullptr;
    bool m_dataBigEndian = false;
    vistle::Port *m_grid = nullptr;
    vistle::Port *m_vol_vert[NumVolVert];
    vistle::StringParameter *m_vol_vert_choice[NumVolVert];
    vistle::Port *m_vol_elem[NumVolElem];
    vistle::StringParameter *m_vol_elem_choice[NumVolElem];
    vistle::Port *m_surf = nullptr;
    vistle::Port *m_surf_vert[NumSurfVert];
    vistle::StringParameter *m_surf_vert_choice[NumSurfVert];
    vistle::Port *m_surf_elem[NumSurfElem];
    vistle::StringParameter *m_surf_elem_choice[NumSurfElem];
    vistle::Port *m_curve = nullptr;
    vistle::Port *m_curve_vert[NumCurveVert];
    vistle::StringParameter *m_curve_vert_choice[NumCurveVert];
    vistle::Port *m_curve_elem[NumCurveElem];
    vistle::StringParameter *m_curve_elem_choice[NumCurveElem];
    vistle::Port *m_points = nullptr;
    vistle::Port *m_point_vert[NumPointVert];
    vistle::StringParameter *m_point_vert_choice[NumPointVert];
    vistle::Port *m_point_elem[NumPointElem];
    vistle::StringParameter *m_point_elem_choice[NumPointElem];
    CaseFile m_case;
    vistle::coRestraint m_selectedParts;

    std::vector<std::string> m_geoFiles;
    std::map<int, vistle::Object::const_ptr> m_constantGeo;
};
#endif
