#ifndef VISTLE_READENSIGHT_GEOGOLD_H
#define VISTLE_READENSIGHT_GEOGOLD_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    GeoGold
//
// Description: Abstraction of an EnSight Gold geometry file
//             (text and binary format)
//
// Initial version: 08.08.2003
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 / 2003 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "EnFile.h"
#include "EnPart.h"

#include <string>
#include <vector>

class GeoGold: public EnFile {
public:
    // creates file-rep. and opens the file
    GeoGold(ReadEnsight *mod, const std::string &name, CaseFile::BinType binType = CaseFile::CBIN);
    ~GeoGold() override;

protected:
    bool parseForParts() override;
    vistle::Object::ptr read(int timestep, int block, EnPart *part, const EnPart *refPart = nullptr) override;

    // read bounding box
    bool readBoundingBox(FILE *in);

    // read header (multi-timestep check, node and element id)
    bool readHeader(FILE *in);

    // read EnSight part information (coordinates and ids)
    bool readPartCoords(FILE *in, EnPart &actPart, const EnPart *refPart);
    // read part connectivities (EnSight Gold only)
    bool readPartConn(FILE *in, EnPart &actPart);

    // read the count block following an element type line: number of elements,
    // element ids (skipped if given), per-element face/point counts (numFaces,
    // nfaced/nsided) and per-face point counts (numPoints, nfaced only)
    void readElementCounts(FILE *in, const EnElement &elem, size_t &numElements, std::vector<unsigned> &numFaces,
                           std::vector<std::vector<unsigned>> &numPoints);

    // Build a vistle grid object from the data in an EnPart.
    vistle::Object::ptr buildGrid(EnPart *part);

    // state used by readPartConn / readPartCoords
    bool m_partFound = false;
};
#endif
