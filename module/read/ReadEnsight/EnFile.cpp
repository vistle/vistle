// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                  (C)2001 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class       EnFile                    ++
// ++                                           DataCont                  ++
// ++                                                                     ++
// ++ Author of initial version:  Ralf Mikulla (rm@vircinity.com)         ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 05.06.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "GeoGoldAscii.h"
#include "GeoGoldBin.h"
#include "DataGoldAscii.h"
#include "DataGoldBin.h"
#include "EnPart.h"
#include "EnFile.h"
#include "EnElement.h"
#include "ReadEnsight.h"

#include "ByteSwap.h"

#include <iostream>
#include <cfloat>
#include <boost/algorithm/string.hpp>

#ifdef _WIN32
int fseek(FILE *file, long offset, int whence)
{
    return _fseeki64(file, offset, whence);
}
#endif

//#define DEBUG

#define CERR std::cerr << "EnFile::" << __func__ << ": "

CaseFile::BinType EnFile::guessBinType(ReadEnsight *mod, const std::string &filename)
{
    CaseFile::BinType ret = CaseFile::UNKNOWN;

    FILE *in = fopen(filename.c_str(), "rb");
    if (!in) {
        CERR << "fopen failed for " << filename << std::endl;
        //return CaseFile::CBIN;
        return CaseFile::UNKNOWN;
    }
    std::vector<char> buf;
    for (int i = 0; i < 80; ++i) {
        int c = fgetc(in);
        if (c > 0 && c < 255 && isprint(c)) {
            buf.push_back(c);
        }
    }
    fclose(in);

    std::string firstLine(buf.begin(), buf.end());
    std::transform(firstLine.begin(), firstLine.end(), firstLine.begin(), tolower);

    if ((firstLine.find("c binary") != std::string::npos) || (firstLine.find("cbinary") != std::string::npos)) {
        ret = CaseFile::CBIN;
    } else if ((firstLine.find("fortran binary") != std::string::npos) ||
               (firstLine.find("fortranbinary") != std::string::npos)) {
        ret = CaseFile::FBIN;
    } else {
        ret = CaseFile::NOBIN;
    }

    return ret;
}

bool strCaseEndsWith(const std::string &str, const std::string &suffix)
{
    if (str.length() >= suffix.length()) {
        for (size_t i = 0; i < suffix.size(); i++) {
            if (std::tolower(str[str.length() - suffix.length() + i]) != std::tolower(suffix[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}


//////////////////////////// base class ////////////////////////////

// static member to create an EnSight geometry file
// use the information given in the case file
std::unique_ptr<EnFile> EnFile::createGeometryFile(ReadEnsight *mod, const CaseFile &c, const std::string &filename)
{
    // file type
    CaseFile::BinType binType = guessBinType(mod, filename);
    // EnSight version
    int version(c.getVersion());

    if (strCaseEndsWith(filename, ".mgeo")) {
        // EnSight 6
        if (version == CaseFile::v6) {
            mod->sendError("EnSight v6 is not supported");
            return nullptr;
        }
    } else if (version == CaseFile::v6) {
        // EnSight 6
        mod->sendError("EnSight v6 is not supported");
        return nullptr;
    } else if (version == CaseFile::gold) {
        // EnSight GOLD
        switch (binType) {
        case CaseFile::CBIN:
            //mod->sendInfo("reading EnSight Gold C binary");
            return std::make_unique<GeoGoldBin>(mod, filename, binType);
        case CaseFile::FBIN:
            //mod->sendInfo("reading EnSight Gold Fortran binary");
            return std::make_unique<GeoGoldBin>(mod, filename, binType);
        case CaseFile::NOBIN:
            //mod->sendInfo("reading EnSight Gold ASCII");
            return std::make_unique<GeoGoldAscii>(mod, filename);
        case CaseFile::UNKNOWN:
            mod->sendError("unknown format");
            break;
        }
    }
    return nullptr;
}

std::unique_ptr<EnFile> EnFile::createDataFile(ReadEnsight *mod, const CaseFile &c, const std::string &field,
                                               int timestep)
{
    // EnSight version
    int version(c.getVersion());
    if (version != CaseFile::gold) {
        // EnSight 6 and older not supported, only EnSight Gold
        return nullptr;
    }

    auto item = c.getDataItem(field);
    if (!item) {
        return nullptr;
    }

    auto fn = item->getFileName();
    fn = c.getDir() + "/" + boost::trim_copy(fn);
    auto allFieldFiles = c.makeFileNames(fn, c.getTimeSet(field));
    if (allFieldFiles.empty())
        allFieldFiles.push_back(fn);

    int dim = 0;
    switch (item->getType()) {
    case DataItem::scalar:
        dim = 1;
        break;
    case DataItem::vector:
        dim = 3;
        break;
    case DataItem::tensor:
        dim = 0;
        break;
    }

    if (dim != 1 && dim != 3) {
        return nullptr;
    }

    int idx = timestep;
    if (idx < 0)
        idx = 0;
    if (allFieldFiles.size() == 1) {
        idx = 0;
    }
    if (idx >= allFieldFiles.size()) {
        return nullptr;
    }
    CERR << "field: file=" << fn;
    std::cerr << " (" << allFieldFiles.size() << " total):";
    for (auto f: allFieldFiles) {
        std::cerr << " " << f;
    }
    std::cerr << std::endl;

    // file type
    auto filename = allFieldFiles[idx];
    CERR << "opening data file " << filename << std::endl;
    //auto binType = guessBinType(mod, filename);

    switch (c.getBinType()) {
    case CaseFile::CBIN:
    case CaseFile::FBIN:
        CERR << "reading CBIN/FBIN" << std::endl;
        return std::make_unique<DataGoldBin>(mod, filename, dim, item->perVertex(), c.getBinType());
    case CaseFile::NOBIN:
        CERR << "reading ASCII" << std::endl;
        return std::make_unique<DataGoldAscii>(mod, filename, dim, item->perVertex());
    case CaseFile::UNKNOWN:
        CERR << "not reading UNKNOWN" << std::endl;
        break;
    }

    return nullptr;
}

EnFile::EnFile(ReadEnsight *mod, const std::string &name, const int dim, const CaseFile::BinType binType)
: fileMayBeCorrupt_(false), binType_(binType), byteSwap_(false), dim_(dim), ens(mod), name_(name), in_(open())
{
    if (mod)
        byteSwap_ = mod->byteSwap();
}

EnFile::EnFile(ReadEnsight *mod, const std::string &name, const CaseFile::BinType binType)
: fileMayBeCorrupt_(false), binType_(binType), byteSwap_(false), dim_(1), ens(mod), name_(name), in_(open())
{
    if (mod)
        byteSwap_ = mod->byteSwap();
}


EnFile::~EnFile()
{}

FP EnFile::open()
{
    assert(!name_.empty());
    FILE *fp = nullptr;
    if (binType_ != CaseFile::FBIN && binType_ != CaseFile::CBIN) { // reopen as ASCII else leave it in binary mode
        fp = fopen(name_.c_str(), "r");
    } else {
        fp = fopen(name_.c_str(), "rb");
    }
    if (!fp) {
        CERR << "fopen(" << name_ << ") failed" << std::endl;
    }
    return FP(fp);
}


bool EnFile::isOpen()
{
    return in_.get() != nullptr;
}

CaseFile::BinType EnFile::binType()
{
    assert(binType_ != CaseFile::UNKNOWN);
    return binType_;
}

// helper skip n floats or doubles
void EnFile::skipFloat(FILE *in, size_t n)
{
    filePos_ = ftell(in);

    if (binType_ == CaseFile::FBIN) { // check for block markers
        size_t ilen(getSizeRaw(in));

        fseek(in, ilen, SEEK_CUR);

        size_t olen(getSizeRaw(in));
        if ((ilen != olen)) {
            CERR << "ERROR: wrong number of elements in block, expected" << n << " but blockstart: " << ilen
                 << " blockend: " << olen << std::endl;
        }
    }

    // Read floats up to 4GB
    else {
        fseek(in, n * sizeof(float), SEEK_CUR);
    }
}

// helper skip n ints
void EnFile::skipInt(FILE *in, size_t n)
{
    filePos_ = ftell(in);

    if (binType_ == CaseFile::FBIN) { // check for block markers
        size_t ilen(getSizeRaw(in));
        fseek(in, n * 4, SEEK_CUR);

        size_t olen(getSizeRaw(in));
        if ((ilen != olen) || (ilen != n * 4)) {
            CERR << "ERROR: wrong number of elements in block, expected" << n << " but blockstart: " << ilen
                 << " blockend: " << olen << std::endl;
        }
    } else {
        fseek(in, n * sizeof(int), SEEK_CUR);
    }
}

// helper to read binary std::strings
std::string EnFile::getStr(FILE *in)
{
    const int strLen(80);
    char buf[strLen + 1];
    buf[80] = '\0';
    std::string ret;

    filePos_ = ftell(in);

    if (binType_ == CaseFile::FBIN) {
        size_t olen(0);
        size_t ilen(getSizeRaw(in));
        if (feof(in)) {
            //end of file reached
            return ret;
        } else if (feof(in)) {
            CERR << "ERROR: error during read of a fortran string" << std::endl;
            return ret;
        }
        //automatic byteorder detection
        if (ilen == 0) {
            olen = getSizeRaw(in);
            CERR << "WARNING: empty std::string" << std::endl;
            if (olen != 0) {
                CERR << "ERROR: not a fortran std::string " << std::endl;
            }
            return ret;
        }
        if (ilen != strLen) { // we have to do byteswaping here
            byteSwap_ = !byteSwap_;
            byteSwap(ilen);
        }
        if (ilen != strLen) {
            CERR << "ERROR: not a fortran std::string of length 80" << std::endl;
            return ret;
        }
        fread(buf, strLen, 1, in);
        if (feof(in)) {
            CERR << "ERROR: end of file during read of a fortran std::string of length 80" << std::endl;
            return ret;
        } else if (ferror(in)) {
            CERR << "ERROR: error during read of a fortran std::string of length 80" << std::endl;
            return ret;
        }
        olen = getSizeRaw(in);
        if (ilen != olen) {
            CERR << "ERROR: not a fortran std::string of length 80" << std::endl;
            return ret;
        }
    } else {
        fread(buf, strLen, 1, in);
        if (feof(in)) {
#ifdef DEBUG
            CERR << "ERROR: end of file during read of a C std::string of length 80" << std::endl;
#endif
            return ret;
        } else if (ferror(in)) {
            CERR << "ERROR: error during read of a C std::string of length 80" << std::endl;
            return ret;
        }
    }

    buf[80] = '\0';
    ret = buf;
    return ret;
}

template<typename T>
T EnFile::getValRaw(FILE *in)
{
    static_assert(sizeof(T) == 4, "T must be 4 bytes");
    T ret = 0;
    fread(&ret, sizeof(T), 1, in); // read a 4 byte integer
    if (feof(in)) {
        CERR << "ERROR: end of file during read a value of size " << sizeof(T) << std::endl;
        return 0;
    } else if (ferror(in)) {
        CERR << "ERROR: error during read a value of size " << sizeof(T) << std::endl;
        return 0;
    }
    if (byteSwap_) {
        byteSwap(ret);
    }
    return ret;
}

template<typename T>
T EnFile::getVal(FILE *in)
{
    filePos_ = ftell(in);

    if (binType_ == CaseFile::FBIN) {
        getSizeRaw(in);
        T ret = getValRaw<T>(in);
        getSizeRaw(in);
        return ret;
    }

    return getValRaw<T>(in);
}

int EnFile::getInt(FILE *in)
{
    return getVal<int>(in);
}

unsigned EnFile::getUInt(FILE *in)
{
    return getVal<unsigned>(in);
}

size_t EnFile::getSizeRaw(FILE *in)
{
    return getValRaw<unsigned>(in);
}

template<typename T>
bool EnFile::getValArrHelper(FILE *in, size_t n, T *arr)
{
    if (!arr) {
        fseek(in, n * sizeof(T), SEEK_CUR);
        return true;
    }

    fread(arr, sizeof(T), n, in);
    if (feof(in)) {
        CERR << "ERROR: end of file during read of an array of size " << n * sizeof(T) << std::endl;
        return false;
    } else if (ferror(in)) {
        CERR << "ERROR: error during read of an array of size " << n * sizeof(T) << std::endl;
        return false;
    }

    if (byteSwap_)
        byteSwap(arr, n);
    return true;
}

template<typename T>
T *EnFile::getValArr(FILE *in, size_t n, T *arr)
{
    filePos_ = ftell(in);

    if (n == 0)
        return arr;

    if (binType_ == CaseFile::FBIN) {
        if (in) {
            size_t ilen(getSizeRaw(in));
            getValArrHelper(in, n, arr);
            size_t olen(getSizeRaw(in));
            if ((ilen != olen) && (!feof(in))) {
                CERR << "length mismatch (fortran) " << std::endl;
            }
        } else {
            CERR << "stream not in good condition" << std::endl;
        }
    } else {
        getValArrHelper(in, n, arr);
    }

    return arr;
}

int *EnFile::getIntArr(FILE *in, size_t n, int *iarr)
{
    return getValArr(in, n, iarr);
}

unsigned *EnFile::getUIntArr(FILE *in, size_t n, unsigned *uarr)
{
    return getValArr(in, n, uarr);
}

void EnFile::setPartList(PartList *p)
{
    partList_ = p;
}

vistle::Scalar *EnFile::getFloatArr(FILE *in, size_t n, vistle::Scalar *farr)
{
    filePos_ = ftell(in);

    if (n == 0)
        return nullptr;

    // FIXME: Scalar
    if (farr == nullptr)
        return nullptr;

    if (binType_ == CaseFile::FBIN) {
        const size_t len(sizeof(float));
        char *buf = new char[n * len];
        bool eightBytePerFloat = false;

        size_t ilen(getSizeRaw(in));
        size_t olen(0);

        // we may have obtained double arrays
        // unfortionately ifstream.read will read only basic types
        // otherwise we would have saved one memcpy
        // There may be more elegant soutions for that.
        if (ilen / n == 8) {
            eightBytePerFloat = true;
            double *dummyArr = new double[n];
            delete[] buf;
            buf = new char[ilen];
            fread(buf, 8, n, in);
            olen = getSizeRaw(in);
            if (byteSwap_)
                byteSwap((uint64_t *)buf, n);
            memcpy(dummyArr, buf, ilen);
            CERR << "got 64-bit floats" << std::endl;
            if (farr != nullptr) {
                for (size_t i = 0; i < n; ++i)
                    farr[i] = (float)dummyArr[i];
            }
            delete[] dummyArr;
        } else {
            fread(buf, len, n, in);
            olen = getSizeRaw(in);
        }
        if ((ilen != olen) && (!feof(in))) {
            CERR << "length mismatch (fortran) " << ilen << "     " << olen << std::endl;
        }

        if (!eightBytePerFloat) {
            if (byteSwap_)
                byteSwap((uint32_t *)buf, n);
            if (farr != nullptr)
                memcpy(farr, buf, n * len);
        }
    } else {
        if (sizeof(*farr) == sizeof(float)) {
            getValArrHelper(in, n, farr);
        } else if (farr) {
            std::vector<float> buf(n);
            getValArrHelper(in, n, buf.data());
            std::copy(buf.begin(), buf.end(), farr);
        } else {
            getValArrHelper(in, n, (float *)nullptr);
        }
    }

    return farr;
}

long EnFile::filePos() const
{
    return filePos_;
}

std::string EnFile::name() const
{
    return name_;
}

// find a part by its part number
EnPart *EnFile::findPart(const int partNum) const
{
    if (partList_ != nullptr) {
        for (size_t i = 0; i < partList_->size(); ++i) {
            if ((*partList_)[i].getPartNum() == partNum)
                return &(*partList_)[i];
        }
    }
    return nullptr;
}

void EnFile::sendPartsToInfo()
{
    std::string info;
    info += EnPart::partInfoHeader();

    int cnt(0);
    if (partList_ != nullptr) {
        for (auto pos = partList_->begin(); pos != partList_->end(); pos++) {
            info += pos->partInfoString(cnt);
            cnt++;
        }
    }
    info += EnPart::partInfoFooter();
    ens->sendInfo(info);
}

bool EnFile::hasPartWithDim(int dim) const
{
    if (partList_ == nullptr) {
        return false;
    }

    for (const auto &part: *partList_) {
        if (part.hasDim(dim)) {
            return true;
        }
    }

    return false;
}
