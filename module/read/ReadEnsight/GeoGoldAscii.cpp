// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                           (C)2002 / 2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class EnGoldgeoASC                     ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 08.04.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "GeoGoldAscii.h"
#include "ReadEnsight.h"

#include <vector>
#include <string>
#include <iostream>

#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>

#include <boost/algorithm/string.hpp>
using boost::algorithm::trim_copy;

using namespace vistle;

#define CERR std::cerr << "GeoGoldAscii::" << __func__ << ": "

// helper converts char buf containing num ints of length int_leng to int-array arr
template<typename T>
static void atoiArr(size_t int_leng, char *buf, T *arr, size_t num)
{
    if ((buf != nullptr) && (arr != nullptr)) {
        unsigned cnt = 0, i = 0;
        std::string str(buf);
        //cerr << "En6GeoASC::atoiArr(..) STR: " << str << std::endl;
        std::string::iterator it = str.begin();
        std::string chunk;
        while (it != str.end()) {
            char x = *it;
            if (x != ' ')
                chunk += x;
            if (((x == ' ') && (!chunk.empty())) || (cnt >= int_leng)) {
                arr[i] = atol(chunk.c_str());
                //cerr << "chunk: " << chunk << " int: " << i << " val: " << arr[i] << std::endl;
                cnt = 0;
                // chunk.clear() does not wor on linux
                chunk.erase(chunk.begin(), chunk.end());
                i++;
            }
            ++it;
            ++cnt;
        }
        // this warning should never occur
        if (i > num)
            CERR << "WARNING: found more numbers than expected!" << std::endl;
        // chunk will not be empty
        if (!chunk.empty())
            arr[i] = atol(chunk.c_str());
        //cerr << "chunk: " << chunk << " int: " << i << " val: " << arr[i] << std::endl;
    }
}

GeoGoldAscii::GeoGoldAscii(ReadEnsight *mod, const std::string &name): EnFile(mod, name, CaseFile::NOBIN), lineCnt_(0)
{}

// read the file
vistle::Object::ptr GeoGoldAscii::read(int timestep, int block, EnPart *part)
{
    Object::ptr out;

    auto in = open();
    if (!in) {
        return out;
    }

    // read header
    if (!readHeader(in)) {
        return out;
    }

    // read bounding box
    if (!readBB(in)) {
        return out;
    }

    ens->sendInfo("reading part#%d/%d:  %d", part->getPartNum(), (int)partList_->size(), block);
    // read parts and connectivity

    fseek(in, part->startPos(), SEEK_SET);
    CERR << "reading part: " << part << " at " << part->startPos() << std::endl;
    if (!readPart(in, *part)) {
        ens->sendWarning("reading part#%d failed, skipping", part->getPartNum());
        return out;
    }

    if (!readPartConn(in, *part)) {
        ens->sendWarning("reading connectivity for part#%d failed, skipping", part->getPartNum());
        return out;
    }
    CERR << "part: " << part << std::endl;
    if (part->numEleRead3d() > 0 && part->numConnRead3d() > 0 && part->x3d_ && part->y3d_ && part->z3d_) {
        auto unstr = std::make_shared<UnstructuredGrid>(0, 0, 0);
        unstr->d()->el = part->el3d_;
        unstr->d()->cl = part->cl3d_;
        unstr->d()->tl = part->tl3d_;
        unstr->d()->x[0] = part->x3d_;
        unstr->d()->x[1] = part->y3d_;
        unstr->d()->x[2] = part->z3d_;
        out = unstr;
    } else if (part->numEleRead2d() > 0 && part->numConnRead2d() > 0 && part->x3d_ && part->y3d_ && part->z3d_) {
        auto poly = std::make_shared<Polygons>(0, 0, 0);
        poly->d()->el = part->el2d_;
        poly->d()->cl = part->cl2d_;
        poly->d()->x[0] = part->x3d_;
        poly->d()->x[1] = part->y3d_;
        poly->d()->x[2] = part->z3d_;
        out = poly;
    } else {
        std::stringstream str;
        str << "cannot handle: " << part << std::endl;
        ens->sendWarning(str.str());
    }
    if (out) {
        CERR << "read: " << *out << std::endl;
    }

    return out;
}


// get Bounding Box section in EnSight GOLD (only)
bool GeoGoldAscii::readBB(FILE *in)
{
    char buf[lineLen];
    // 2 lines description - ignore it
    fgets(buf, lineLen, in);
    ++lineCnt_;
    std::string line(buf);
    if (line.find("extents") != std::string::npos) {
        //CERR << "extents section found" << std::endl;
        fgets(buf, lineLen, in);
        ++lineCnt_;
        fgets(buf, lineLen, in);
        ++lineCnt_;
        fgets(buf, lineLen, in);
        ++lineCnt_;
    } else {
        // rewind one line
        fseek(in, -5L, SEEK_CUR);
    }

    return true;
}

// read header
bool GeoGoldAscii::readHeader(FILE *in)
{
    char buf[lineLen];
    // 2 lines description - ignore it
    fgets(buf, lineLen, in);
    ++lineCnt_;
    fgets(buf, lineLen, in);
    ++lineCnt_;
    // node id
    fgets(buf, lineLen, in);
    ++lineCnt_;
    std::string tmp(buf);
    char tok[20];
    strcpy(tok, " id");
    size_t beg(tmp.find(tok));

    if (beg == std::string::npos) {
        CERR << "ERROR node-id not found" << std::endl;
        return false;
    }
    // " id"  has length 3
    beg += 4;
    bool ret = true;
    std::string cmpStr(trim_copy(tmp.substr(beg)));
    if (cmpStr == std::string("off")) {
        nodeId_ = EnFile::OFF;
    } else if (cmpStr == std::string("given")) {
        nodeId_ = EnFile::GIVEN;
    } else if (cmpStr == std::string("assign")) {
        nodeId_ = EnFile::ASSIGN;
    } else if (cmpStr == std::string("ignore")) {
        nodeId_ = EnFile::EN_IGNORE;
    } else {
        nodeId_ = EnFile::UNKNOWN;
        CERR << "NODE-ID Error" << std::endl;
        ret = false;
    }

    // element id
    fgets(buf, lineLen, in);
    ++lineCnt_;
    tmp = std::string(buf);

    beg = tmp.find(tok);

    if (beg == std::string::npos) {
        CERR << "ERROR element-id not found" << std::endl;
        return false;
    }

    beg += 4;
    cmpStr = trim_copy(tmp.substr(beg));
    if (cmpStr == std::string("off")) {
        elementId_ = EnFile::OFF;
    } else if (cmpStr == std::string("given")) {
        elementId_ = EnFile::GIVEN;
    } else if (cmpStr == std::string("assign")) {
        elementId_ = EnFile::ASSIGN;
    } else if (cmpStr == std::string("ignore")) {
        elementId_ = EnFile::EN_IGNORE;
    } else {
        elementId_ = EnFile::UNKNOWN;
        CERR << "ELEMENT-ID Error" << std::endl;
        ret = false;
    }


    return ret;
}

bool GeoGoldAscii::readPart(FILE *in, EnPart &actPart)
{
    char buf[lineLen];
    int noRead(0);
    // 2 lines description - ignore it
    fgets(buf, lineLen, in);
    ++lineCnt_;
    std::string line(buf);
    CERR << lineCnt_ << " : " << line << std::endl;
    if (line.find("part") == std::string::npos) {
        CERR << lineCnt_ << " NO part header found" << std::endl;
        return false;
    }

    // part No
    fgets(buf, lineLen, in);
    ++lineCnt_;
    int partNo = -1;
    noRead = sscanf(buf, "%d", &partNo);
    actPartNumber_ = partNo;
    actPart.setPartNum(partNo);
    //CERR << "got part No: " << partNo << std::endl;
    if (noRead != 1) {
        CERR << "Error reading part No" << std::endl;
        return false;
    }

    // description line
    fgets(buf, lineLen, in);
    ++lineCnt_;
    actPart.setComment(buf);
    // coordinates token
    fgets(buf, lineLen, in);
    ++lineCnt_;
    line = std::string(buf);
    if (line.find("coordinates") == std::string::npos) {
        CERR << "coordinates key not found" << std::endl;
        return false;
    }
    // number of coordinates
    fgets(buf, lineLen, in);
    ++lineCnt_;
    size_t nc(0);
    noRead = sscanf(buf, "%zu", &nc);
    //CERR << "got No. of coordinates (per part): " << nc << std::endl;
    if (noRead != 1) {
        CERR << "Error reading no of coordinates" << std::endl;
        return false;
    }
    actPart.setNumCoords(nc);
    // allocate memory for the coordinate list per part
    actPart.x3d_.construct(nc);
    actPart.y3d_.construct(nc);
    actPart.z3d_.construct(nc);
    // id's or coordinates
    vistle::Scalar *x = actPart.x3d_->data();
    vistle::Scalar *y = actPart.y3d_->data();
    vistle::Scalar *z = actPart.z3d_->data();

    // id's or coordinates
    float val;
    switch (nodeId_) {
    case OFF:
    case ASSIGN:
    case EN_IGNORE:
        // read x-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            x[i] = val;
        }
        // read y-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            y[i] = val;
        }
        // read z-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            z[i] = val;
        }
        break;
    case GIVEN:
        // index array
        // the index array is currently ignored
        int iVal;
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%d", &iVal);
            if (noRead != 1) {
                CERR << "Error reading index array" << std::endl;
                return false;
            }
        }
        // read x-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            x[i] = val;
        }
        // read y-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            y[i] = val;
        }
        // read z-values
        for (size_t i = 0; i < nc; ++i) {
            fgets(buf, lineLen, in);
            ++lineCnt_;
            noRead = sscanf(buf, "%e", &val);
            if (noRead != 1) {
                CERR << "Error reading coordinates" << std::endl;
                return false;
            }
            z[i] = val;
        }
        break;
    default:
        CERR << "Error interpreting nodeId " << nodeId_ << std::endl;
        return false;
        break;
    }

    return true;
}

bool GeoGoldAscii::readPartConn(FILE *in, EnPart &actPart)
{
    int &partNo(actPartNumber_);

    char buf[lineLen];
    int currElePtr3d = 0, currElePtr2d = 0;

    unsigned locArr[21];
    unsigned cornIn[20], cornOut[20];

    size_t numElements;

    int statistic[30];
    int rstatistic[30][30];
    for (int ii = 0; ii < 30; ++ii) {
        statistic[ii] = 0;
        for (int jj = 0; jj < 9; ++jj)
            rstatistic[ii][jj] = 0;
    }
    int degCells(0);

    actPart.clearElements();
    actPart.el2d_.construct();
    actPart.el2d_->push_back(0);
    actPart.cl2d_.construct();
    actPart.tl2d_.construct();
    auto &eleLst2d = *actPart.el2d_;
    auto &cornLst2d = *actPart.cl2d_;
    auto &typeLst2d = *actPart.tl2d_;

    actPart.el3d_.construct();
    actPart.el3d_->push_back(0);
    actPart.cl3d_.construct();
    actPart.tl3d_.construct();
    auto &eleLst3d = *actPart.el3d_;
    auto &cornLst3d = *actPart.cl3d_;
    auto &typeLst3d = *actPart.tl3d_;

    // we don't know a priori how many EnSight elements we can expect here therefore we have to read
    // until we find a new 'part'
    while (!feof(in)) {
        fgets(buf, lineLen, in);
        ++lineCnt_;
        std::string tmp(buf);

        if (tmp.find("part") != std::string::npos) {
            // we have read one line more than needed
            fseek(in, -5L, SEEK_CUR);
            break;
        }
        // scan for element type
        std::string elementType(tmp);
        EnElement elem(elementType);
        // we have a valid EnSight element
        if (elem.valid()) {
            std::vector<int> blanklist;
            // get number of elements
            fgets(buf, lineLen, in);
            ++lineCnt_;
            numElements = atol(buf);
            //CERR << elementType << "  " <<  numElements <<  std::endl;
            size_t nc = elem.getNumberOfCorners();
            size_t ncc = elem.getNumberOfVistleCorners();
            int covType = elem.getCovType();

            // read elements id's
            switch (elementId_) {
            case OFF:
            case EN_IGNORE:
            case ASSIGN:
                break;
            case GIVEN:
                for (size_t i = 0; i < numElements; ++i) {
                    fgets(buf, lineLen, in);
                    ++lineCnt_;
                }
                break;
            default:
                CERR << "Error interpreting elementId " << elementId_ << std::endl;
                return false;
                break;
            }

            for (size_t i = 0; i < numElements; ++i) {
                fgets(buf, lineLen, in);
                ++lineCnt_;
                // an integer always has 10 figures (see EnSight docu EnGold)
                atoiArr(10, buf, locArr, nc);
                // remap indices (EnSight elements may have a different numbering scheme than Vistle)
                //  prepare arrays
                for (size_t j = 0; j < nc; ++j)
                    cornIn[j] = locArr[j] - 1;
                // we add the element to the list of points if it has more than one
                // distinct point
                bool degen = !elem.hasDistinctCorners(cornIn);
                if (degen) {
                    blanklist.push_back(-1);
                    degCells++;
                } else {
                    blanklist.push_back(1);
                    // do the remapping
                    elem.remap(cornIn, cornOut);
                    // assign element list
                    if (elem.getDim() == EnElement::D2) {
                        for (size_t j = 0; j < ncc; ++j)
                            cornLst2d.push_back(cornOut[j]);
                        typeLst2d.push_back(covType);
                        currElePtr2d += ncc;
                        eleLst2d.push_back(currElePtr2d);
                    } else if (elem.getDim() == EnElement::D3) {
                        for (size_t j = 0; j < ncc; ++j)
                            cornLst3d.push_back(cornOut[j]);
                        typeLst3d.push_back(covType);
                        currElePtr3d += ncc;
                        eleLst3d.push_back(currElePtr3d);
                    }
                }
            }
            elem.setBlanklist(std::move(blanklist));
            actPart.addElement(elem, numElements);
        }
    }

    // create arrays explicitly
    actPart.setNumEleRead2d(eleLst2d.size() - 1);
    actPart.setNumEleRead3d(eleLst3d.size() - 1);
    actPart.setNumConnRead2d(cornLst2d.size());
    actPart.setNumConnRead3d(cornLst3d.size());

    //CERR << degCells << " degenerated cells found" << std::endl;

    if (degCells > 0) {
        CERR << "WRONG ELEMENT STATISTICS" << std::endl;
        std::cerr << "-------------------------------------------" << std::endl;
        for (int ii = 2; ii < 9; ++ii) {
            std::cerr << ii << " | " << statistic[ii];
            for (int jj = 1; jj < 9; ++jj)
                std::cerr << " || " << rstatistic[ii][jj];
            std::cerr << std::endl;
        }

        sprintf(buf, " -> found %d fully degenerated cells in part %d", degCells, partNo);
        ens->sendInfo("%s", buf);
    }
    return true;
}

//
// Destructor
//
GeoGoldAscii::~GeoGoldAscii()
{}

//
// set up a list of parts
//
bool GeoGoldAscii::parseForParts()
{
    if (!isOpen()) {
        return false;
    }
    auto &in = in_;

    if (!readHeader(in)) {
        return false;
    }

    if (!readBB(in)) {
        return false;
    }

    char buf[lineLen];
    EnPart *actPart(nullptr);
    while (!feof(in)) {
        long filePos = ftell(in);
        fgets(buf, lineLen, in);
        ++lineCnt_;
        std::string tmp(buf);
        int actPartNr;

        // scan for part token
        // read comment and print part line
        size_t id = tmp.find("part");
        if (id != std::string::npos) {
            long startPos = filePos;
            // part line found
            // get part number
            fgets(buf, lineLen, in);
            ++lineCnt_;
            actPartNr = atol(buf);

            // comment line we need it for the table output
            fgets(buf, lineLen, in);
            ++lineCnt_;
            std::string comment(buf);

            // coordinates token
            fgets(buf, lineLen, in);
            ++lineCnt_;
            std::string coordTok(buf);
            id = coordTok.find("coordinates");
            size_t numCoords(0);
            if (id != std::string::npos) {
                // number of coordinates
                fgets(buf, lineLen, in);
                ++lineCnt_;
                numCoords = atol(buf);
                //CERR << "numCoords " << numCoords << std::endl;
                switch (nodeId_) {
                case GIVEN: // 10+1 + 3*(12+1)
                    fseek(in, 50 * numCoords, SEEK_CUR);
                    break;
                default: // 39 = 3 * (12 + 1)
                    fseek(in, 39 * numCoords, SEEK_CUR);
                }
            }

            id = std::string::npos;

            // add part to the partList
            // BE AWARE that you have to add a part to the list of parts AFTER the part is
            // COMPLETELY build up
            if (actPart != nullptr) {
                partList_->emplace_back(std::move(*actPart));
                delete actPart;
            }
            actPart = new EnPart(actPartNr, comment);
            actPart->setNumCoords(numCoords);
            actPart->setStartPos(startPos);
        }
        // scan for element type
        std::string elementType(tmp);
        EnElement elem(elementType);

        // we have a valid EnSight element
        if (elem.valid()) {
            // get number of elements
            fgets(buf, lineLen, in);
            ++lineCnt_;
            size_t numElements = atol(buf);
            size_t nc(elem.getNumberOfCorners());
            switch (elementId_) {
            case GIVEN:
                fseek(in, ((nc * 10) + 12) * numElements, SEEK_CUR);
                break;
            default:
                fseek(in, ((nc * 10) + 1) * numElements, SEEK_CUR);
            }

            // add element info to the part
            // we set the number of 2/3D elements to 1 if
            // we find the corresponding element type that's suuficient to mark
            // the dimension of the cuurent part. The real numbers will be set
            // during the read phase
            if (elem.getDim() == EnElement::D2) {
                if (actPart != nullptr)
                    actPart->setNumEleRead2d(1);
            }
            if (elem.getDim() == EnElement::D3) {
                if (actPart != nullptr)
                    actPart->setNumEleRead3d(1);
            }

            if (actPart != nullptr)
                actPart->addElement(elem, numElements, false);
        }
    }

    // add last part to the list of parts
    if (actPart != nullptr) {
        partList_->emplace_back(std::move(*actPart));
        delete actPart;
    }

    //CERR << "FINISHED " << std::endl;
    return true;
}
