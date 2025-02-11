// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                           (C)2002 / 2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class EnGoldgeoBIN                     ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 08.04.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "ReadEnsight.h"
#include "GeoGoldBin.h"
#include "ByteSwap.h"

#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>


#include <vector>

#include <boost/algorithm/string.hpp>
using boost::algorithm::trim_copy;

using namespace vistle;

//#define DEBUG

#define CERR std::cerr << "GeoGoldBin::" << __func__ << ": "

GeoGoldBin::GeoGoldBin(ReadEnsight *mod, const std::string &name, CaseFile::BinType binType)
: EnFile(mod, name, binType), m_numCoords(0)
{
#ifdef DEBUG
    CERR << "byteSwap_ = " << ((byteSwap_ == true) ? "true" : "false") << std::endl;
#endif
}

Object::ptr GeoGoldBin::read(int timeStep, int block, EnPart *part)
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

    partFound = false;
    ens->sendInfo("read part#%d/%d:  %d", part->getPartNum(), (int)partList_->size(), block);
    fseek(in, part->startPos(), SEEK_SET);
    if (!readPart(in, *part)) {
        ens->sendWarning("reading part#%d failed, skipping", part->getPartNum());
        return out;
    }
    if (!readPartConn(in, *part)) {
        ens->sendWarning("reading connectivity for part#%d failed, skipping", part->getPartNum());
        return out;
    }
#ifdef DEBUG
    CERR << "part: " << part << std::endl;
#endif
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

#ifdef DEBUG
    if (out) {
        CERR << "read: " << *out << std::endl;
    }
#endif

    return out;
}

// get Bounding Box section in EnSight GOLD (only)
bool GeoGoldBin::readBB(FILE *in)
{
    assert(in);
    // 2 lines description - ignore it
    std::string line(getStr(in));
    if (line.find("extents") != std::string::npos) {
        //CERR <<  "extents section found" << std::endl;
        // Vistle is not using the bounding box so far
        skipFloat(in, 6);
    } else {
        if (binType_ == CaseFile::FBIN) {
            fseek(in, -88, SEEK_CUR); // 4 + 80 + 4
        } else {
            fseek(in, -80, SEEK_CUR);
        }
    }
    return true;
}

// read header
bool GeoGoldBin::readHeader(FILE *in)
{
    // bin type
    getStr(in);
    // 2 lines description - ignore it or
    // we may have a multiple timestep - single file situation
    // we ignore it
    std::string checkTs(getStr(in));
    size_t tt(checkTs.find("BEGIN TIME STEP"));
    if (tt != std::string::npos) {
        ens->sendWarning(
            "found multiple timesteps in one file - ONLY THE FIRST TIMESTEP IS READ - ALL OTHERS ARE IGNORED");
        getStr(in);
    }

    getStr(in);
    // node id
    std::string tmp(getStr(in));
    size_t beg(tmp.find(" id"));

    if (beg == std::string::npos) {
        CERR << "ERROR node-id not found" << std::endl;
        return false;
    }
    bool ret = true;
    // " id"  has length 3
    beg += 4;
    std::string cmpStr(trim_copy(tmp.substr(beg, 10)));
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
    tmp = getStr(in);

    beg = tmp.find(" id");

    if (beg == std::string::npos) {
        CERR << "ERROR element-id not found" << std::endl;
        return false;
    }

    beg += 4;
    cmpStr = trim_copy(tmp.substr(beg, 10));
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

bool GeoGoldBin::readPart(FILE *in, EnPart &actPart)
{
    // 2 lines description - ignore it

    std::string line;
    if (!partFound) {
        line = getStr(in);
    }
    if (partFound || line.find("part") != std::string::npos) {
        // part No

        m_actPartNum = getInt(in);

#ifdef DEBUG
        CERR << "byteSwap_ = " << ((byteSwap_ == true) ? "true" : "false") << std::endl;
#endif

        actPart.setPartNum(m_actPartNum);
        //CERR << "got part No: " << m_actPartNum << std::endl;

        // description line
        actPart.setComment(getStr(in));
        // coordinates token
        line = getStr(in);
        if (line.find("coordinates") == std::string::npos) {
            if (line.find("block") != std::string::npos) {
                ens->sendWarning("%s", "found structured part - not implemented yet -");
                return false;
            }

            CERR << "coordinates key not found" << std::endl;
            return false;
        }
        // number of coordinates
        size_t nc(getUInt(in));
        m_numCoords = nc;
        actPart.setNumCoords(nc);
        actPart.x3d_.construct(nc);
        actPart.y3d_.construct(nc);
        actPart.z3d_.construct(nc);
        // id's or coordinates
        vistle::Scalar *x = actPart.x3d_->data();
        vistle::Scalar *y = actPart.y3d_->data();
        vistle::Scalar *z = actPart.z3d_->data();

        switch (nodeId_) {
        case GIVEN: {
            // index array
            auto iMap = new unsigned[nc];
            getUIntArr(in, nc, iMap);
            // workaround for broken EnSightFiles with Index map as floats
            float *tmpf = (float *)(iMap);
            if (nc > 2 && tmpf[0] == 1.0 && tmpf[1] == 2.0 && tmpf[2] == 3.0) {
                fprintf(stderr, "Broken EnSight File!!! ignoring index map\n");
            }
            // read x-values
            getFloatArr(in, nc, x);
            // read y-values
            getFloatArr(in, nc, y);
            // read z-values
            getFloatArr(in, nc, z);
            delete[] iMap;
            break;
        }
        default:
            // read x-values
            getFloatArr(in, nc, x);
            // read y-values
            getFloatArr(in, nc, y);
            // read z-values
            getFloatArr(in, nc, z);
        }
    } else
        CERR << "NO part header found" << std::endl;
    //CERR << "got " << m_numCoords << " coordinates"   << std::endl;
    return true;
}

bool GeoGoldBin::readPartConn(FILE *in, EnPart &actPart)
{
#ifdef DEBUG
    CERR << "comment: " << actPart.comment() << std::endl;
#endif

    int &partNo(m_actPartNum);

    char buf[lineLen];

    size_t currElePtr2d = 0, currElePtr3d = 0;
    unsigned cornIn[20], cornOut[20];
    size_t numElements;

    partFound = false;
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
    while ((!feof(in)) && (!partFound)) {
        std::string tmp(getStr(in));
        if (tmp.find("part") != std::string::npos) {
            partFound = true;
        }
        // scan for element type
        std::string elementType(tmp);
        EnElement elem(elementType);
        // we have a valid EnSight element
        if (elem.valid() && !partFound) {
            std::vector<int> blanklist;
            // get number of elements
            numElements = getUInt(in);
#ifdef DEBUG
            CERR << " read " << numElements << " elements" << std::endl;
#endif

            if (numElements > 0) {
                // skip elements id's
                if (elementId_ == GIVEN)
                    skipInt(in, numElements);

                // ------------------- NFACED ----------------------
                if (elem.getEnType() == EnElement::nfaced) {
                    // Read number of faces/points
                    std::vector<unsigned> numFaces(numElements);
                    std::vector<std::vector<unsigned>> numPoints(numElements);
                    getUIntArr(in, numElements, numFaces.data());
                    for (size_t i = 0; i < numElements; ++i) {
                        numPoints[i].resize(numFaces[i]);
                        getUIntArr(in, numFaces[i], numPoints[i].data());
                    }

                    // Read polyhedral elements (VARIANT 1)
                    for (size_t i = 0; i < numElements; ++i) {
                        typeLst3d.push_back(elem.getCovType());
                        for (size_t j = 0; j < numFaces[i]; ++j) {
                            size_t nc = numPoints[i][j];
                            std::vector<unsigned> locArr(nc);
                            getUIntArr(in, nc, locArr.data());
                            for (size_t k = 0; k < nc; ++k) {
                                cornLst3d.push_back(locArr[k] - 1);
                                currElePtr3d++;
                                if ((k != 0) && (locArr[k] == locArr[0])) {
                                    // The first point appears twice in the face and would destroy
                                    // our "first-point-again-ends-face"-definition. We explicitly
                                    // start a new face here by adding the point again.
                                    cornLst3d.push_back(locArr[k] - 1);
                                    currElePtr3d++;
                                }
                            }
                            cornLst3d.push_back(locArr[0] - 1);
                            currElePtr3d++; // add first point again to mark the end of the face
                        }
                        eleLst3d.push_back(currElePtr3d);
                        blanklist.push_back(1);
                    }
                }
                // ------------------- NFACED ----------------------

                // ------------------- NSIDED ----------------------
                else if (elem.getEnType() == EnElement::nsided) {
                    // Read number of points
                    std::vector<unsigned> numPoints(numElements);
                    getUIntArr(in, numElements, numPoints.data());
                    // Read elements
                    for (size_t i = 0; i < numElements; ++i) {
                        typeLst2d.push_back(elem.getCovType());
                        size_t nc = numPoints[i];
                        std::vector<unsigned> locArr(nc);
                        getUIntArr(in, nc, locArr.data());
                        for (size_t k = 0; k < nc; ++k) {
                            cornLst2d.push_back(locArr[k] - 1);
                        }
                        currElePtr2d += nc;
                        eleLst2d.push_back(currElePtr2d);
                        blanklist.push_back(1);
                    }
                }
                // ------------------- NSIDED ----------------------

                // ---------------- DEFAULT ELEMENT-----------------
                else {
                    size_t nc = elem.getNumberOfCorners();
                    size_t ncc = elem.getNumberOfVistleCorners();
                    int covType = elem.getCovType();
                    std::vector<unsigned> locArr(numElements * nc);
                    getUIntArr(in, numElements * nc, locArr.data());
                    size_t eleCnt(0);
                    for (size_t i = 0; i < numElements; ++i) {
                        // remap indices (EnSight elements may have a different element numbering scheme than Vistle)
                        //  prepare arrays
                        for (size_t j = 0; j < nc; ++j) {
                            size_t idx = eleCnt + j;
                            assert(locArr[idx] > 0);
                            cornIn[j] = locArr[idx] - 1;
                        }
                        eleCnt += nc;
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
                }
                // ---------------- DEFAULT ELEMENT-----------------
            }


#ifdef DEBUG
            CERR << "read " << numElements << " elements, #blanklist=" << blanklist.size() << std::endl;
#endif
            elem.setBlanklist(std::move(blanklist));
            actPart.addElement(elem, numElements);
        }
    }
    // we have read one line more than needed
    if (partFound) {
        //	in.sync();
        //	if ( binType_ == EnFile::FBIN) in.seekg(-88L, ios::cur);
        //	else in.seekg(-80L, ios::cur);
        //	in.sync();
    }

    actPart.setNumEleRead2d(eleLst2d.size() - 1);
    actPart.setNumEleRead3d(eleLst3d.size() - 1);
    actPart.setNumConnRead2d(cornLst2d.size());
    actPart.setNumConnRead3d(cornLst3d.size());

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

#ifdef DEBUG
    CERR << "done" << std::endl;
#endif

    return true;
}

//
// Destructor
//
GeoGoldBin::~GeoGoldBin()
{}

//
// set up a list of parts
//
bool GeoGoldBin::parseForParts()
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

    EnPart *actPart(nullptr);
    long startPos = 0;
    bool validElementFound = false;
    while (!feof(in)) {
        size_t nElem2D(0), nElem3D(0);
        std::string tmp(getStr(in));
        int actPartNr = 0;
        // scan for part token
        // read comment and print part line
        size_t id = tmp.find("part");
        if (id != std::string::npos) {
            startPos = filePos();
            // part line found
            // get part number
            actPartNr = getInt(in);

#ifdef DEBUG
            CERR << "byteSwap_ = " << ((byteSwap_ == true) ? "true" : "false") << std::endl;
#endif

            // comment line we need it for the table output
            std::string comment(getStr(in));
            // coordinates token
            std::string coordTok(getStr(in));
            id = coordTok.find("coordinates");
            size_t numCoords(0);
            if (id != std::string::npos) {
                // number of coordinates
                numCoords = getUInt(in);
#ifdef DEBUG
                CERR << "got " << numCoords << " coordinates for part " << comment << std::endl;
#endif
                switch (nodeId_) {
                case GIVEN:
                    skipInt(in, numCoords);
                    skipFloat(in, numCoords);
                    skipFloat(in, numCoords);
                    skipFloat(in, numCoords);
                    break;
                default:
                    skipFloat(in, numCoords);
                    skipFloat(in, numCoords);
                    skipFloat(in, numCoords);
                }
            }
            id = std::string::npos;
            // add part to the partList
            // BE AWARE that you have to add a part to the list of parts AFTER the part is
            // COMPLETELY built up
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
        if (actPart->comment().find("particles") != std::string::npos) {
            validElementFound = true;
        } else if (elem.valid()) {
            validElementFound = true;
            // get number of elements
            size_t numElements = getUInt(in);
            if (elementId_ == GIVEN)
                skipInt(in, numElements);

            if (elem.getEnType() == EnElement::nfaced) {
                size_t numFaces = 0;
                size_t numNodes = 0;
                for (size_t i = 0; i < numElements; i++)
                    numFaces += getUInt(in);
                for (size_t i = 0; i < numFaces; i++)
                    numNodes += getUInt(in);
                skipInt(in, numNodes);
            } else if (elem.getEnType() == EnElement::nsided) {
                size_t numPoints = 0;
                for (size_t i = 0; i < numElements; i++)
                    numPoints += getUInt(in);
                skipInt(in, numPoints);
            } else {
                skipInt(in, elem.getNumberOfCorners() * numElements);
            }

            // we set the number of 2/3D elements to 1 if
            // we find the corresponding element type that's sufficient to mark
            // the dimension of the current part. The real numbers will be set
            // during the read phase
            if (elem.getDim() == EnElement::D2) {
                nElem2D += numElements;
                actPart->setNumEleRead2d(nElem2D);
            }
            if (elem.getDim() == EnElement::D3) {
                nElem3D += numElements;
                actPart->setNumEleRead3d(nElem3D);
            }

            // add element info to the part
            actPart->addElement(elem, numElements, false);
        }
    }

    // add last part to the list of parts
    if (actPart != nullptr) {
        partList_->emplace_back(std::move(*actPart));
        delete actPart;
    }

    if (!validElementFound) {
        CERR << "WARNING never found a valid element SUSPICIOUS!!!!" << std::endl;
        fileMayBeCorrupt_ = true;
        return false;
    }

    return true;
}
