/***************************************************************************
                          tecplotfile.h  -  description
                             -------------------
    begin                : Wed Jul 26 2006
	copyright			: (C) 2006-2014 IAG, University of Stuttgart
	email				: acco@iag.uni-stuttgart.de
 ***************************************************************************/

#ifndef TECFILE_H
#define TECFILE_H

#include "mesh.h"

#include <vector>
#include <memory>

class MeshBase;
struct MeshPts;

typedef std::vector<MeshBase *> MeshBaseVec;
typedef std::vector<MeshPts *> MeshPtsVec;

class TecplotFile {
public:
    TecplotFile(std::string const &iFile);
    ~TecplotFile();
    const std::vector<std::string> &Variables() const;
    size_t NumZones() const;
    MeshBaseVec Read(std::string const &iZoneRindList = "");
    MeshBase *ReadZone(size_t idx, std::string const &iZoneRindList = "");
    void SkipZone(size_t idx);
    MeshPtsVec ReadInnerPts();

private:
    bool checkDataSetType();
    struct Impl;
    struct Zone;
    std::string mTitle;
    int mNumVar;
    std::vector<std::string> mVarNames;
    std::vector<Zone> mZones;
    std::unique_ptr<Impl> pimpl;
};

MeshBaseVec Read(std::string const &iFile, std::string const &iZoneRindList);

#endif
