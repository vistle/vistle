// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    EnGoldgeoAsc
//
// Description: Abstraction of EnSight Gold geometry Files
//
// Initial version: 08.04.2003
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 / 2003 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Changes:
//

#ifndef READENSIGHT_GEOGOLDASCII_H
#define READENSIGHT_GEOGOLDASCII_H

#include "EnFile.h"

#include <string>
#include <vector>

class GeoGoldAscii: public EnFile {
public:
    // creates file-rep. and opens the file
    GeoGoldAscii(ReadEnsight *mod, const std::string &name);
    ~GeoGoldAscii();

    // read the file
    vistle::Object::ptr read(int timestep, int block, EnPart *part) override;

    // get part info
    bool parseForParts() override;

private:
    // read header
    bool readHeader(FILE *in);

    // read EnSight part information
    bool readPart(FILE *in, EnPart &actPart);

    // read part connectivities (EnSight Gold only)
    bool readPartConn(FILE *in, EnPart &actPart);

    // read bounding box (EnSight Gold)
    bool readBB(FILE *in);

    int lineCnt_; // actual linecount
    int actPartNumber_;
};
#endif
