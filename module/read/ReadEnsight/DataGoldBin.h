#ifndef VISTLE_READENSIGHT_DATAGOLDBIN_H
#define VISTLE_READENSIGHT_DATAGOLDBIN_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    DataFileGold
//
// Description: Abstraction of a EnSight Gold ASCII Data File
//
// Initial version: 07.04.2003
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002/2003 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Changes:
//

#include "EnFile.h"

#include <string>

class ReadEnsight;

class DataGoldBin: public EnFile {
public:
    DataGoldBin(ReadEnsight *mod, const std::string &name, int dim, bool perVertex, CaseFile::BinType binType);
    ~DataGoldBin();

    bool parseForParts() override;
    vistle::Object::ptr read(int timestep, int block, EnPart *part) override;

private:
    bool perVertex_ = false;
};

#endif
