// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                           (C)2002 / 2003 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class GeoGold (text and binary)       ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 08.04.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "GeoGold.h"
#include "ReadEnsight.h"

#include <cassert>
#include <boost/algorithm/string/trim.hpp>

#include <vistle/core/unstr.h>
#include <vistle/core/polygons.h>
#include <vistle/core/lines.h>
#include <vistle/core/points.h>

using namespace vistle;
using boost::algorithm::trim_copy;

#define CERR std::cerr << "GeoGold::" << __func__ << ": " << where() << ": "

GeoGold::GeoGold(ReadEnsight *mod, const std::string &name, CaseFile::BinType binType): EnFile(mod, name, 1, binType)
{}

// set up a list of parts
bool GeoGold::parseForParts()
{
    if (!isOpen()) {
        return false;
    }
    auto &in = in_;

    if (!readHeader(in)) {
        return false;
    }

    std::unique_ptr<EnPart> actPart;
    ssize_t startPos = 0;
    bool validElementFound = false;
    std::array<size_t, 4> nElem;
    for (std::string tmp = getStr(in); !feof(in); tmp = getStr(in)) {
        // scan for part token
        // read comment and print part line
        EnPart::GeoMode geoMode = EnPart::INVALID;
        if (parsePartLine(tmp, &geoMode)) {
            startPos = filePos();

            if (actPart) {
                // add previous part
                partList_->emplace_back(std::move(*actPart));
                actPart.reset();
            }

            // part line found
            // get part number
            int actPartNr = getInt(in);

            // comment line we need it for the table output
            std::string comment(getStr(in));
            tmp = getStr(in);
            // coordinates token
            std::string coordTok(tmp);
            size_t id = coordTok.find("coordinates");
            ssize_t numCoords(-1);
            if (id != std::string::npos) {
                // number of coordinates
                numCoords = getUInt(in);
                if (nodeId_ == GIVEN || nodeId_ == EN_IGNORE)
                    skipInt(in, numCoords);
                skipFloat(in, numCoords);
                skipFloat(in, numCoords);
                skipFloat(in, numCoords);
            }

            actPart = std::make_unique<EnPart>(actPartNr, comment);
            actPart->setNumCoords(numCoords);
            actPart->setStartPos(startPos);
            actPart->setGeoMode(geoMode);
            actPart->setComment(comment);
            actPart->setPartNum(actPartNr);
            nElem[0] = nElem[1] = nElem[2] = nElem[3] = 0;
            if (id != std::string::npos) {
                continue;
            }
        }

        if (!actPart)
            continue;

        // scan for element type
        std::string elementType(tmp);
        EnElement elem(elementType);
        // we have a valid EnSight element
        if (actPart->comment().find("particles") != std::string::npos) {
            validElementFound = true;
        } else if (elem.valid()) {
            validElementFound = true;
            // get number of elements and count block, then skip the element data
            size_t numElements = 0;
            std::vector<unsigned> numFaces;
            std::vector<std::vector<unsigned>> numPoints;
            readElementCounts(in, elem, numElements, numFaces, numPoints);
            if (elem.getEnType() == EnElement::nfaced) {
                size_t numNodes = 0;
                for (const auto &face: numPoints)
                    numNodes += std::accumulate(face.begin(), face.end(), 0);
                skipInt(in, numNodes);
            } else if (elem.getEnType() == EnElement::nsided) {
                size_t numPts = 0;
                for (auto n: numFaces)
                    numPts += n;
                skipInt(in, numPts);
            } else {
                skipInt(in, elem.getNumberOfCorners() * numElements);
            }

            actPart->setNumEleRead(elem.getDim(), numElements);
            nElem[elem.getDim()] += numElements;

            // add element info to the part
            actPart->addElement(std::move(elem), numElements, false);
        }
    }

    // add last part to the list of parts
    if (actPart) {
        partList_->emplace_back(std::move(*actPart));
    }

    if (!validElementFound) {
        CERR << "WARNING: never found a valid element" << std::endl;
        fileMayBeCorrupt_ = true;
        return false;
    }

    return true;
}

// read the count block following an element type line: number of elements,
// element ids (skipped if given), per-element face/point counts in numFaces
// (nfaced: faces per element, nsided: points per element) and per-face point counts in numPoints
void GeoGold::readElementCounts(FILE *in, const EnElement &elem, size_t &numElements, std::vector<unsigned> &numFaces,
                                std::vector<std::vector<unsigned>> &numPoints)
{
    numElements = getUInt(in);
    numFaces.clear();
    numPoints.clear();

    if (numElements == 0)
        return;

    if (elementId_ == GIVEN || elementId_ == EN_IGNORE)
        skipInt(in, numElements);

    if (elem.getEnType() == EnElement::nfaced) {
        numFaces.resize(numElements);
        getUIntArr(in, numElements, numFaces.data());
        numPoints.resize(numElements);
        for (size_t i = 0; i < numElements; ++i) {
            numPoints[i].resize(numFaces[i]);
            getUIntArr(in, numFaces[i], numPoints[i].data());
        }
    } else if (elem.getEnType() == EnElement::nsided) {
        // points per element
        numFaces.resize(numElements);
        getUIntArr(in, numElements, numFaces.data());
    }
}

// read the element connectivity of a part (handles both ASCII and binary input)
bool GeoGold::readPartConn(FILE *in, EnPart &actPart)
{
    size_t currElePtr0d = 0, currElePtr1d = 0, currElePtr2d = 0, currElePtr3d = 0;
    unsigned cornIn[20], cornOut[20];

    size_t numElements;
    std::vector<unsigned> numFaces;
    std::vector<std::vector<unsigned>> numPoints;

    int degCells(0);

    actPart.clearElements();
    std::array<ShmVector<Index>, 4> el;
    std::array<ShmVector<Index>, 4> cl;
    std::array<ShmVector<Byte>, 4> tl;
    for (int d = 0; d < 4; ++d) {
        el[d] = actPart.el_[d];
        cl[d] = actPart.cl_[d];
        tl[d] = actPart.tl_[d];
    }

    m_partFound = false;
    // we don't know a priori how many EnSight elements we can expect here therefore we have to read
    // until we find a new 'part'
    while ((!feof(in)) && (!m_partFound)) {
        std::string tmp(getStr(in));
        if (parsePartLine(tmp)) {
            m_partFound = true;
            break;
        }
        // scan for element type
        std::string elementType(tmp);
        EnElement elem(elementType);

        // we have a valid EnSight element
        if (!elem.valid()) {
            continue;
        }

        std::vector<vistle::Byte> blanklist;
        // get number of elements, ids and count block
        readElementCounts(in, elem, numElements, numFaces, numPoints);
        if (numElements == 0) {
            continue;
        }

        if (elem.getEnType() == EnElement::nfaced) {
            // ------------------- NFACED ----------------------
            // Read polyhedral elements (VARIANT 1)
            for (size_t i = 0; i < numElements; ++i) {
                tl[3]->push_back(elem.getCovType());
                for (size_t j = 0; j < numFaces[i]; ++j) {
                    size_t nc = numPoints[i][j];
                    std::vector<unsigned> locArr(nc);
                    getUIntArr(in, nc, locArr.data());
                    for (size_t k = 0; k < nc; ++k) {
                        cl[3]->push_back(locArr[k] - 1);
                        currElePtr3d++;
                        if ((k != 0) && (locArr[k] == locArr[0])) {
                            // The first point appears twice in the face and would destroy
                            // our "first-point-again-ends-face"-definition. We explicitly
                            // start a new face here by adding the point again.
                            cl[3]->push_back(locArr[k] - 1);
                            currElePtr3d++;
                        }
                    }
                    cl[3]->push_back(locArr[0] - 1);
                    currElePtr3d++; // add first point again to mark the end of the face
                }
                el[3]->push_back(currElePtr3d);
                blanklist.push_back(1);
            }
        } else if (elem.getEnType() == EnElement::nsided) {
            // ------------------- NSIDED ----------------------
            // Read elements
            for (size_t i = 0; i < numElements; ++i) {
                tl[2]->push_back(elem.getCovType());
                size_t nc = numFaces[i];
                std::vector<unsigned> locArr(nc);
                getUIntArr(in, nc, locArr.data());
                for (size_t k = 0; k < nc; ++k) {
                    cl[2]->push_back(locArr[k] - 1);
                }
                currElePtr2d += nc;
                el[2]->push_back(currElePtr2d);
                blanklist.push_back(1);
            }
        } else {
            // ---------------- DEFAULT ELEMENT-----------------
            size_t nc = elem.getNumberOfCorners();
            assert(nc <= 20); // bound of cornIn / cornOut
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
                bool degen = elem.getDim() != EnElement::D0 && !elem.hasDistinctCorners(cornIn);
                if (degen) {
                    blanklist.push_back(0);
                    degCells++;
                } else {
                    blanklist.push_back(1);
                    // do the remapping
                    elem.remap(cornIn, cornOut);
                    if (elem.getDim() == EnElement::D2) {
                        for (size_t j = 0; j < ncc; ++j)
                            cl[2]->push_back(cornOut[j]);
                        tl[2]->push_back(covType);
                        currElePtr2d += ncc;
                        el[2]->push_back(currElePtr2d);
                    } else if (elem.getDim() == EnElement::D3) {
                        for (size_t j = 0; j < ncc; ++j)
                            cl[3]->push_back(cornOut[j]);
                        tl[3]->push_back(covType);
                        currElePtr3d += ncc;
                        el[3]->push_back(currElePtr3d);
                    } else if (elem.getDim() == EnElement::D1) {
                        // a bar with more than two nodes becomes one line segment per node pair
                        for (size_t j = 0; j + 1 < nc; ++j) {
                            cl[1]->push_back(cornIn[j]);
                            cl[1]->push_back(cornIn[j + 1]);
                            tl[1]->push_back(covType);
                        }
                        currElePtr1d += nc;
                        el[1]->push_back(currElePtr1d);
                    } else if (elem.getDim() == EnElement::D0) {
                        cl[0]->push_back(cornIn[0]);
                        tl[0]->push_back(covType);
                        currElePtr0d++;
                        el[0]->push_back(currElePtr0d);
                    }
                }
            }
        }

        elem.setBlanklist(std::move(blanklist));
        actPart.addElement(std::move(elem), numElements);
    }

    // create arrays explicitly
    for (int d = 0; d < 4; ++d) {
        actPart.setNumEleRead(d, el[d]->size() - 1);
        //actPart.setNumConnRead(d, cl[d]->size() - 1);
        actPart.setNumConnRead(d, cl[d]->size());
    }

    if (degCells > 0) {
        ens->sendInfo(" -> found %d fully degenerated cells in part %d", degCells, actPart.getPartNum());
    }

    return true;
}


vistle::Object::ptr GeoGold::read(int timestep, int block, EnPart *part, const EnPart *refPart)
{
    vistle::Object::ptr out;

    auto in = open();
    if (!in) {
        return out;
    }

    if (!readHeader(in)) {
        return out;
    }

    m_partFound = false;
    file::seek(in, part->startPos(), SEEK_SET);
    if (refPart && (part->geoMode() == EnPart::NO_CHANGE)) {
        part->copyCoord(*refPart);
    } else if (!readPartCoords(in, *part, refPart)) {
        ens->sendWarning("reading part#%d failed - skipping", part->getPartNum());
        return out;
    }
    if (refPart && (part->geoMode() == EnPart::NO_CHANGE || part->geoMode() == EnPart::COORD_CHANGE)) {
        part->copyConn(*refPart);
    } else if (!readPartConn(in, *part)) {
        ens->sendWarning("reading connectivity for part#%d failed - skipping", part->getPartNum());
        return out;
    }
    out = buildGrid(part);
    if (out) {
        CERR << "read: " << *out << " for " << part->comment() << std::endl;
    }
    return out;
}

vistle::Object::ptr GeoGold::buildGrid(EnPart *part)
{
    vistle::Object::ptr out;
    if (part->isEmpty()) {
        CERR << "empty part: " << *part << std::endl;
        out = std::make_shared<vistle::UnstructuredGrid>(0, 0, 0);
    } else if (part->x3d_ && part->y3d_ && part->z3d_) {
        if (part->numEleRead(3) > 0 && part->numConnRead(3) > 0) {
            auto unstr = std::make_shared<vistle::UnstructuredGrid>(0, 0, 0);
            unstr->d()->el = part->el_[3];
            unstr->d()->cl = part->cl_[3];
            unstr->d()->tl = part->tl_[3];
            unstr->d()->x[0] = part->x3d_;
            unstr->d()->x[1] = part->y3d_;
            unstr->d()->x[2] = part->z3d_;
            out = unstr;
        } else if (part->numEleRead(2) > 0 && part->numConnRead(2) > 0) {
            auto poly = std::make_shared<vistle::Polygons>(0, 0, 0);
            poly->d()->el = part->el_[2];
            poly->d()->cl = part->cl_[2];
            poly->d()->x[0] = part->x3d_;
            poly->d()->x[1] = part->y3d_;
            poly->d()->x[2] = part->z3d_;
            out = poly;
        } else if (part->numEleRead(1) > 0 && part->numConnRead(1) > 0) {
            auto lines = std::make_shared<vistle::Lines>(0, 0, 0);
            lines->d()->el = part->el_[1];
            lines->d()->cl = part->cl_[1];
            lines->d()->x[0] = part->x3d_;
            lines->d()->x[1] = part->y3d_;
            lines->d()->x[2] = part->z3d_;
            out = lines;
        } else if (part->numEleRead(0) > 0 && part->numConnRead(0) > 0) {
            auto pts = std::make_shared<vistle::Points>(0);
            pts->d()->x[0] = part->x3d_;
            pts->d()->x[1] = part->y3d_;
            pts->d()->x[2] = part->z3d_;
            out = pts;
        } else {
            auto unstr = std::make_shared<vistle::UnstructuredGrid>(0, 0, 0);
            unstr->d()->x[0] = part->x3d_;
            unstr->d()->x[1] = part->y3d_;
            unstr->d()->x[2] = part->z3d_;
            out = unstr;
        }
    } else {
        std::stringstream str;
        str << "cannot handle part: #" << part->getPartNum() << ": " << part->partInfoString(-1) << std::endl;
        ens->sendWarning(str.str());
    }

    return out;
}


// read header (multi-timestep check, node and element id)
bool GeoGold::readHeader(FILE *in)
{
    if (binType_ != CaseFile::NOBIN) {
        // bin type (binary)
        getStr(in);
    }
    // second description line - ignore it, but check for a
    // multiple timestep - single file situation (we only read the first)
    std::string checkTs(getStr(in));
    size_t tt(checkTs.find("BEGIN TIME STEP"));
    if (tt != std::string::npos) {
        ens->sendWarning(
            "found multiple timesteps in one file - ONLY THE FIRST TIMESTEP IS READ - ALL OTHERS ARE IGNORED");
        getStr(in);
    }
    //second description line (ascii) - ignore it
    getStr(in);

    std::string tmp(getStr(in));
    const std::string node_id{"node id"};
    size_t beg(tmp.find(node_id));

    if (beg == std::string::npos) {
        CERR << "ERROR node-id not found" << std::endl;
        return false;
    }
    bool ret = true;
    beg += node_id.length() + 1;
    std::string cmpStr(trim_copy(tmp.substr(beg)));
    nodeId_ = parseIdType(cmpStr);
    if (nodeId_ == EnFile::UNKNOWN) {
        CERR << "NODE-ID Error" << std::endl;
        ret = false;
    }
    // element id
    tmp = getStr(in);

    const std::string element_id{"element id"};
    beg = tmp.find(element_id);

    if (beg == std::string::npos) {
        CERR << "ERROR element-id not found" << std::endl;
        return false;
    }

    beg += element_id.length() + 1;
    cmpStr = trim_copy(tmp.substr(beg));
    elementId_ = parseIdType(cmpStr);
    if (elementId_ == EnFile::UNKNOWN) {
        CERR << "ELEMENT-ID Error" << std::endl;
        ret = false;
    }

    // read bounding box
    if (!readBoundingBox(in)) {
        ret = false;
    }

    return ret;
}

// get Bounding Box section in EnSight GOLD (only)
bool GeoGold::readBoundingBox(FILE *in)
{
    assert(in);
    // 2 lines description - ignore it
    std::string line(getStr(in));
    if (line.find("extents") != std::string::npos) {
        // Vistle is not using the bounding box so far
        skipFloat(in, 6);
    } else {
        // no extents - rewind
        file::seek(in, filePos(), SEEK_SET);
    }
    return true;
}


bool GeoGold::readPartCoords(FILE *in, EnPart &actPart, const EnPart *refPart)
{
    // 2 lines description - ignore it
    std::string line;
    if (!m_partFound) {
        line = getStr(in);
        m_partFound = parsePartLine(line);
    }
    if (!m_partFound) {
        CERR << "NO part header found" << std::endl;
        return false;
    }

    // part No
    auto actPartNum = getInt(in);
    assert(actPartNum == actPart.getPartNum());
    (void)actPartNum;

    // description line
    auto comment = getStr(in);

    // coordinates token
    line = getStr(in);
    if (line.find("block") != std::string::npos) {
        ens->sendWarning("%s", "found structured part - not implemented yet -");
        return false;
    }

    if (line.find("coordinates") == std::string::npos) {
        if (actPart.isEmpty()) {
            return true;
        }

        CERR << "coordinates key not found, got instead: " << line << std::endl;
        return false;
    }
    // number of coordinates
    size_t nc(getUInt(in));
    if (nodeId_ == GIVEN || nodeId_ == EN_IGNORE) {
        // node ids are not used, skip them
        skipInt(in, nc);
    }

    actPart.clearCoords();
    actPart.setNumCoords(nc);
    // id's or coordinates
    vistle::Scalar *x = actPart.x3d_->data();
    vistle::Scalar *y = actPart.y3d_->data();
    vistle::Scalar *z = actPart.z3d_->data();
    getFloatArr(in, nc, x);
    getFloatArr(in, nc, y);
    getFloatArr(in, nc, z);
    return true;
}

GeoGold::~GeoGold() = default;
