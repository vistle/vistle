#ifndef VISTLE_READENSIGHT_ENFILE_H
#define VISTLE_READENSIGHT_ENFILE_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS  EnFile
//
// Description: EnSight file representation (base class)
//
// Initial version: 01.06.2002 by RM
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2001 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "EnPart.h"
#include "CaseFile.h"

#include <string>
#include <memory>

#include <vistle/module/reader.h>
#include <vistle/util/fileio.h>

class ReadEnsight;

class FP: public std::unique_ptr<FILE, decltype(&fclose)> {
public:
    FP(FILE *f): std::unique_ptr<FILE, decltype(&fclose)>(f, fclose) {}
    operator FILE *() const { return get(); }
};

//
// base class for EnSight geometry and data files
// provide general methods for reading geometry and data files
//
class EnFile {
public:
    enum IdType { UNKNOWN = -1, OFF, GIVEN, ASSIGN, EN_IGNORE };
    // bit flags selecting which element dimensionalities are of interest
    enum ReadType : int { NOTHING = 0, SURFACE = 1, VOLUME = 2, CURVE = 4, POINT = 8, ALL = 0xF };

    EnFile(ReadEnsight *mod, const std::string &name, int dim = 1, const CaseFile::BinType binType = CaseFile::UNKNOWN);

    virtual ~EnFile();

    bool isOpen();

    bool mayBeCorrupt() const;

    // check for file type (ASCII, C binary, Fortran binary)
    CaseFile::BinType binType();

    // read the file
    virtual vistle::Object::ptr read(int timestep, int block, EnPart *part, const EnPart *refPart = nullptr) = 0;

    virtual bool parseForParts() = 0;

    void setPartList(PartList *p);

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

    std::string name() const;

    void enableChangingGeometryPerPart(bool changing);
    bool hasChangingGeometryPerPart() const;

protected:
    FP open();

    // functions used for BINARY input
    virtual std::string getStr(FILE *in);

    // get integer
    virtual int getInt(FILE *in);
    virtual unsigned getUInt(FILE *in);

    // get integer array
    virtual int *getIntArr(FILE *in, size_t n, int *iarr = nullptr);
    virtual unsigned *getUIntArr(FILE *in, size_t n, unsigned *uarr = nullptr);
    // skip n ints
    void skipInt(FILE *in, size_t n);

    // get float array
    virtual vistle::Scalar *getFloatArr(FILE *in, size_t n, vistle::Scalar *farr = nullptr);
    // skip n floats
    void skipFloat(FILE *in, size_t n);

    // true, if line introduces a part - mode returns how geometry changes compared to previous timestep
    bool parsePartLine(const std::string &line, EnPart::GeoMode *mode = nullptr) const;

    // parse an EnSight ID string ("off", "given", "assign", "ignore") into an IdType
    IdType parseIdType(const std::string &str) const;
    // find a part by its part number
    virtual EnPart *findPart(int partNum) const;

    IdType nodeId_ = UNKNOWN;

    IdType elementId_ = UNKNOWN;

    CaseFile::BinType binType_;

    bool fileMayBeCorrupt_ = true;

    bool byteSwap_ = false;

    PartList *partList_ = nullptr;

    int dim_ = 1;

    // pointer to module for sending ui messages
    ReadEnsight *ens = nullptr;

    ssize_t filePos() const;

    std::string name_;

    FP in_;

    std::string where() const;

    bool m_changingGeometryPerPart = false;

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

    // read one text-formatted value from an ASCII file
    bool readTextVal(FILE *in, int &val) const;
    bool readTextVal(FILE *in, unsigned &val) const;
    bool readTextVal(FILE *in, float &val) const;

    ssize_t filePos_ = 0;
};
#endif
