// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS  EnFile
//
// Description: EnSight file representation ( base class)
//
// Initial version: 01.06.2002 by RM
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2001 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Changes:
//

#ifndef READENSIGHT_ENFILE_H
#define READENSIGHT_ENFILE_H

#include "EnElement.h"
#include "EnPart.h"
#include "CaseFile.h"

#include <string>
#include <memory>

#include <vistle/module/reader.h>

class ReadEnsight;

#ifdef _WIN32
int fseek(FILE *file, long offset, int whence);
#endif

class FP: public std::unique_ptr<FILE, decltype(&fclose)> {
public:
    FP(FILE *f): std::unique_ptr<FILE, decltype(&fclose)>(f, fclose) {}
    operator FILE *() const { return get(); }
};

const unsigned int lineLen(250);

//
// base class for EnSight geometry files
// provide general methods for reading geometry files
//
class EnFile {
public:
    enum DimType { DIM1D, DIM2D, DIM3D, GEOMETRY };
    enum IdType { UNKNOWN = -1, OFF, GIVEN, ASSIGN, EN_IGNORE };
    enum ReadType { NOTHING, SURFACE, VOLUME, VOLUME_AND_SURFACE };

    EnFile(ReadEnsight *mod, const std::string &name, const CaseFile::BinType binType = CaseFile::UNKNOWN);

    EnFile(ReadEnsight *mod, const std::string &name, const int dim,
           const CaseFile::BinType binType = CaseFile::UNKNOWN);

    virtual ~EnFile();

    bool isOpen();

    // check for binary file
    CaseFile::BinType binType();

    // read the file
    virtual vistle::Object::ptr read(int timestep, int block, EnPart *part) = 0;

    // Set the master part list. This is the list of all part in the geometry file
    // or the geo. file for the first timestep. This means we must still check the
    // length of the connection table

    virtual void setPartList(PartList *p);
    virtual bool parseForParts() = 0;

    void sendPartsToInfo();

    bool hasPartWithDim(int dim) const;

    static CaseFile::BinType guessBinType(ReadEnsight *mod, const std::string &filename);
    // use this function to create a geometry EnSight file representation
    // each EnSight geometry file has its factory to create associated
    // data files
    // you will have to change this method each time you enter a new type of
    // EnSight Geometry
    static std::unique_ptr<EnFile> createGeometryFile(ReadEnsight *mod, const CaseFile &c, const std::string &filename);
    static std::unique_ptr<EnFile> createDataFile(ReadEnsight *mod, const CaseFile &c, const std::string &field,
                                                  int timestep);

    bool fileMayBeCorrupt_ = true;

protected:
    FP open();

    // functions used for BINARY input
    virtual std::string getStr(FILE *in);

    // skip n ints
    void skipInt(FILE *in, size_t n);

    // skip n floats
    void skipFloat(FILE *in, size_t n);

    // get integer
    virtual int getInt(FILE *in);
    virtual unsigned getUInt(FILE *in);

    // get integer array
    virtual int *getIntArr(FILE *in, size_t n, int *iarr = nullptr);
    virtual unsigned *getUIntArr(FILE *in, size_t n, unsigned *uarr = nullptr);

    // get float array
    virtual vistle::Scalar *getFloatArr(FILE *in, size_t n, vistle::Scalar *farr = nullptr);

    // find a part by its part number
    virtual EnPart *findPart(int partNum) const;

    IdType nodeId_ = UNKNOWN;

    IdType elementId_ = UNKNOWN;

    CaseFile::BinType binType_;

    bool byteSwap_ = false;

    PartList *partList_ = nullptr;

    int dim_ = 1;

    // pointer to module for sending ui messages
    ReadEnsight *ens = nullptr;

    long filePos() const;
    std::string name() const;

    std::string name_;

    FP in_;

private:
    template<typename T>
    T getValRaw(FILE *in);
    template<typename T>
    T getVal(FILE *in);
    template<typename T>
    T *getValArr(FILE *in, size_t n, T *arr = nullptr);
    template<typename T>
    bool getValArrHelper(FILE *in, size_t n, T *uarr = nullptr);
    size_t getSizeRaw(FILE *in);

    long filePos_ = 0;
};
#endif
