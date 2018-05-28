// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                                  (C)2002 VirCinity  ++
// ++ Description:                                                        ++
// ++             Implementation of class EnElement                       ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@vircinity.com)                            ++
// ++                                                                     ++
// ++               VirCinity GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 05.06.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "EnElement.h"
//#include "GeoFileAsc.h"

#include <iostream>
#include <cstring>

#include <boost/algorithm/string.hpp>
using boost::algorithm::trim_copy;

#include <vistle/core/celltypes.h>

namespace cell = vistle::cell;

#define CERR std::cerr << "EnElement::" << __func__ << ": "


//
// Constructor
//
EnElement::EnElement(): valid_(false), empty_(true)
{}

EnElement::EnElement(const std::string &name)
: valid_(false)
, empty_(false)
, enTypeStr_(trim_copy(name).substr(0, 10)) // the max. length of an ensight cell type is 10 (pyramid13)
{
    if (enTypeStr_ == "point") {
        valid_ = true;
        numCorn_ = 1;
        dim_ = D0;
        vistleType_ = cell::POINT;
        enType_ = point;
        //CERR << "point found" << std::endl;
    } else if (enTypeStr_ == "bar2") {
        valid_ = true;
        //CERR << "bar2 found" << std::endl;
        numCorn_ = 2;
        dim_ = D1;
        vistleType_ = cell::BAR;
        enType_ = bar2;
    } else if (enTypeStr_ == "bar3") {
        valid_ = false;
        CERR << "bar3 found" << std::endl;
        numCorn_ = 3;
        dim_ = D1;
        vistleType_ = cell::POINT;
        enType_ = bar3;
    } else if (enTypeStr_ == "tria3") {
        valid_ = true;
        //CERR << "tria3 found" << std::endl;
        numCorn_ = 3;
        dim_ = D2;
        vistleType_ = cell::TRIANGLE;
        enType_ = tria3;
    } else if (enTypeStr_ == "tria6") {
        valid_ = false;
        CERR << "tria6 found" << std::endl;
        numCorn_ = 6;
        dim_ = D2;
        enType_ = tria6;
    } else if (enTypeStr_ == "quad4") {
        valid_ = true;
        //CERR << "quad4 found" << std::endl;
        numCorn_ = 4;
        dim_ = D2;
        vistleType_ = cell::QUAD;
        enType_ = quad4;
    } else if (enTypeStr_ == "quad8") {
        valid_ = false;
        CERR << "quad8 found" << std::endl;
        numCorn_ = 8;
        dim_ = D2;
        vistleType_ = cell::POINT;
        enType_ = quad8;
    } else if (enTypeStr_ == "tetra4") {
        valid_ = true;
        //CERR << "tetra4 found" << std::endl;
        numCorn_ = 4;
        dim_ = D3;
        vistleType_ = cell::TETRAHEDRON;
        enType_ = tetra4;
    } else if (enTypeStr_ == "tetra10") {
        valid_ = false;
        CERR << "tetra10 found" << std::endl;
        numCorn_ = 10;
        dim_ = D3;
        enType_ = tetra10;
    } else if (enTypeStr_ == "pyramid5") {
        valid_ = true;
        //CERR << "pyramid5 found" << std::endl;
        numCorn_ = 5;
        dim_ = D3;
        enType_ = pyramid5;
        vistleType_ = cell::PYRAMID;
    } else if (enTypeStr_ == "pyramid13") {
        valid_ = false;
        CERR << "pyramid13 found" << std::endl;
        numCorn_ = 13;
        dim_ = D3;
        vistleType_ = cell::POINT;
        enType_ = pyramid13;
    } else if (enTypeStr_ == "hexa8") {
        valid_ = true;
        //CERR << "hexa8  found" << std::endl;
        numCorn_ = 8;
        dim_ = D3;
        vistleType_ = cell::HEXAHEDRON;
        enType_ = hexa8;
    } else if (enTypeStr_ == "hexa20") {
        valid_ = false;
        CERR << "hexa20 found" << std::endl;
        numCorn_ = 20;
        dim_ = D3;
        vistleType_ = cell::POINT;
        enType_ = hexa20;
    } else if (enTypeStr_ == "penta6") {
        valid_ = true;
        //CERR << "penta6  found" << std::endl;
        numCorn_ = 6;
        dim_ = D3;
        vistleType_ = cell::PRISM;
        enType_ = penta6;
    } else if (enTypeStr_ == "penta15") {
        valid_ = false;
        CERR << "pyramid15 found" << std::endl;
        numCorn_ = 15;
        dim_ = D3;
        vistleType_ = cell::POINT;
        enType_ = penta15;
    } else if (enTypeStr_ == "nsided") {
        valid_ = true;
        numCorn_ = 0; // not constant among the elements
        dim_ = D2;
        vistleType_ = cell::POINT; // not necessary, as all 2D elements go into DO_Polygons
        enType_ = nsided;
    } else if (enTypeStr_ == "nfaced") {
        valid_ = true;
        numCorn_ = 0; // not constant among the elements
        dim_ = D3;
        vistleType_ = cell::POLYHEDRON;
        enType_ = nfaced;
    }
    // we allow a dummy element thats the only one which is empty
    else if (name.find("dummy") != std::string::npos) {
        valid_ = false;
        empty_ = true;
    }
}

EnElement::EnElement(const EnElement &e)
: valid_(e.valid_)
, empty_(e.empty_)
, numCorn_(e.numCorn_)
, dim_(e.dim_)
, vistleType_(e.vistleType_)
, enType_(e.enType_)
, startIdx_(e.startIdx_)
, endIdx_(e.endIdx_)
, enTypeStr_(trim_copy(e.enTypeStr_))
, dataBlanklist_(e.dataBlanklist_)
{}

const EnElement &EnElement::operator=(const EnElement &e)
{
    if (this == &e)
        return *this;

    valid_ = e.valid_;
    empty_ = e.empty_;
    numCorn_ = e.numCorn_;
    dim_ = e.dim_;
    vistleType_ = e.vistleType_;
    enType_ = e.enType_;
    startIdx_ = e.startIdx_;
    endIdx_ = e.endIdx_;
    enTypeStr_ = trim_copy(e.enTypeStr_);
    dataBlanklist_ = e.dataBlanklist_;

    return *this;
}

// extend it later to EnSight elements which are not present in Vistle, like hexa20, ...
// EnElement::remap(int *eleIn, int *eleOut, int *newTypes, int *newElem...)
size_t EnElement::remap(unsigned *cornIn, unsigned *cornOut)
{
    // only 2D elements have to be remapped
    // at the first stage
    if (cornOut != nullptr) {
        switch (enType_) {
        case tria3:
            cornOut[0] = cornIn[0];
            cornOut[1] = cornIn[2];
            cornOut[2] = cornIn[1];
            break;
        case quad4:
            cornOut[0] = cornIn[0];
            cornOut[1] = cornIn[3];
            cornOut[2] = cornIn[2];
            cornOut[3] = cornIn[1];
            break;
        default:
            memcpy(cornOut, cornIn, numCorn_ * sizeof(unsigned));
            break;
        }
    }
    return numCorn_;
}

bool EnElement::valid() const
{
    return valid_;
}

bool EnElement::empty() const
{
    return empty_;
}

size_t EnElement::getNumberOfCorners() const
{
    assert(valid_);
    return numCorn_;
}

EnElement::Dimensionality EnElement::getDim() const
{
    assert(valid_);
    return dim_;
}

vistle::cell::CellType EnElement::getCovType() const
{
    assert(valid_);
    return vistleType_;
}

EnElement::Type EnElement::getEnType() const
{
    assert(valid_);
    return enType_;
}

std::string EnElement::getEnTypeStr() const
{
    return enTypeStr_;
}

bool EnElement::hasDistinctCorners(const unsigned *ci) const
{
    assert(valid_);
    if (ci == nullptr) {
        return false;
    }

    for (size_t i = 0; i < numCorn_; ++i) {
        for (size_t j = i + 1; j < numCorn_; ++j) {
            if (ci[i] != ci[j]) {
                return true;
            }
        }
    }

    return false;
}

// return number and array of distinct corners
unsigned EnElement::distinctCorners(const unsigned *ci, unsigned *co) const
{
    assert(valid_);
    if (ci == nullptr) {
        return 0;
    }

    unsigned distinct(0);
    for (size_t i = 0; i < numCorn_; ++i) {
        bool found = false;
        for (size_t j = i + 1; j < numCorn_; ++j) {
            if (ci[i] == ci[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            co[distinct] = ci[i];
            ++distinct;
        }
    }
#ifdef DEBUG
    if (distinct != numCorn_) {
        CERR << "expected " << numCorn_ << ", but got " << distinct << " corners" << std::endl;
    }
#endif

    return distinct;
}

void EnElement::setBlanklist(std::vector<int> &&bl)
{
    dataBlanklist_ = bl;
}

const std::vector<int> &EnElement::getBlanklist() const
{
    assert(valid_);
    return dataBlanklist_;
}

//
// Destructor
//
EnElement::~EnElement()
{}
