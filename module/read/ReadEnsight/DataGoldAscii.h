#ifndef VISTLE_READENSIGHT_DATAGOLDASCII_H
#define VISTLE_READENSIGHT_DATAGOLDASCII_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    DataGoldAscii
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

#include <vistle/core/object.h>

#include <string>

class DataGoldAscii: public EnFile {
public:
    DataGoldAscii(ReadEnsight *mod, const std::string &name, int dim, bool perVertex);
    ~DataGoldAscii();

    bool parseForParts() override;
    vistle::Object::ptr read(int timestep, int block, EnPart *part) override;

private:
    size_t lineCnt_; // actual linecount
    bool perVertex_ = true;
};

#endif
