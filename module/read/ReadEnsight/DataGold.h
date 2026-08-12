#ifndef VISTLE_READENSIGHT_DATAGOLD_H
#define VISTLE_READENSIGHT_DATAGOLD_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    DataGold
//
// Description: Abstraction of a EnSight Gold Data File
//             (text and binary format, per-vertex and per-element data)
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

class DataGold: public EnFile {
public:
    DataGold(ReadEnsight *mod, const std::string &name, int dim, bool perVertex, CaseFile::BinType binType);
    ~DataGold() override;

    bool parseForParts() override;
    vistle::Object::ptr read(int timestep, int block, EnPart *part, const EnPart *refPart) override;

private:
    enum class NextLine { End, Element, Part };
    // Read the next line of interest from a data file.
    // A "part" line (NextLine::Part) yields the part number in `partNrRead` and
    // the position of the part line in `partStart`; the file is then left
    // positioned right after the part number. An element type line
    // (NextLine::Element) is returned in `line`. NextLine::End signals the end
    // of the file or of the first time step.
    // END/BEGIN TIME STEP lines (and the description line that follows them)
    // and the line that follows text mentioning "particles" are skipped.
    NextLine nextDataLine(FILE *in, std::string &line, std::streamoff &partStart, int &partNrRead);
    bool perVertex_ = false;
};

#endif
