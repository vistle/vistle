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
    static const int NumFields = 2;
    static const int NumVolVert = NumFields;
    static const int NumVolElem = NumFields;
    static const int NumSurfVert = 1;
    static const int NumSurfElem = 1;

public:
    ReadEnsight(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadEnsight() override;

    bool examine(const vistle::Parameter *param) override;
    bool prepareRead() override;
    bool prepareTimestep(int timestep) override;
    bool read(Token &token, int timestep = -1, int block = -1) override;
    bool finishRead() override;

    std::vector<PartList> globalParts_;
    bool hasPartWithDim(int dim) const;

    bool byteSwap() const;

private:
    // write a list of parts to the map editor (info channel)
    bool createPartlists(int timestep, bool quick = false);
    bool makeGeoFiles();

    std::vector<std::pair<vistle::Port *, std::string>> getActiveFields(EnFile::ReadType what);

    vistle::StringParameter *m_casefile = nullptr;
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
    CaseFile m_case;
    vistle::coRestraint m_selectedParts;

    std::vector<std::string> m_geoFiles;
    std::vector<std::string> m_fieldFiles[NumFields];
};
#endif
