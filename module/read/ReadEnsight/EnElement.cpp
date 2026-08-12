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

#include <iostream>
#include <cstring>

#include <boost/algorithm/string.hpp>
using boost::algorithm::trim_copy;

#include <vistle/core/celltypes.h>
#include <vistle/core/unstr.h>

namespace cell = vistle::cell;

#define CERR std::cerr << "EnElement::" << __func__ << ": "


//
// Constructor
//
EnElement::EnElement(): valid_(false), empty_(true)
{
    check();
}

EnElement::EnElement(const std::string &name)
: valid_(false)
, empty_(false)
, enTypeStr_(trim_copy(name).substr(0, 12)) // the longest ensight cell type is 'g_pyramid13'
{
    if (enTypeStr_.starts_with("g_")) {
        ghost_ = true;
        enTypeStr_ = enTypeStr_.substr(2);
    }

    if (enTypeStr_ == "point") {
        valid_ = true;
        numCorn_ = 1;
        dim_ = D0;
        vistleType_ = cell::POINT;
        enType_ = point;
    } else if (enTypeStr_ == "bar2") {
        valid_ = true;
        numCorn_ = 2;
        dim_ = D1;
        vistleType_ = cell::BAR;
        enType_ = bar2;
    } else if (enTypeStr_ == "bar3") {
        valid_ = true;
        numCorn_ = 3;
        dim_ = D1;
        vistleType_ = cell::BAR;
        enType_ = bar3;
    } else if (enTypeStr_ == "tria3") {
        valid_ = true;
        numCorn_ = 3;
        dim_ = D2;
        vistleType_ = cell::TRIANGLE;
        enType_ = tria3;
    } else if (enTypeStr_ == "tria6") {
        valid_ = true;
        numCorn_ = 6;
        dim_ = D2;
        vistleType_ = cell::TRIANGLE;
        enType_ = tria6;
    } else if (enTypeStr_ == "quad4") {
        valid_ = true;
        numCorn_ = 4;
        dim_ = D2;
        vistleType_ = cell::QUAD;
        enType_ = quad4;
    } else if (enTypeStr_ == "quad8") {
        valid_ = true;
        numCorn_ = 8;
        dim_ = D2;
        vistleType_ = cell::QUAD;
        enType_ = quad8;
    } else if (enTypeStr_ == "tetra4") {
        valid_ = true;
        numCorn_ = 4;
        dim_ = D3;
        vistleType_ = cell::TETRAHEDRON;
        enType_ = tetra4;
    } else if (enTypeStr_ == "tetra10") {
        valid_ = true;
        numCorn_ = 10;
        dim_ = D3;
        vistleType_ = cell::TETRAHEDRON;
        enType_ = tetra10;
    } else if (enTypeStr_ == "pyramid5") {
        valid_ = true;
        numCorn_ = 5;
        dim_ = D3;
        vistleType_ = cell::PYRAMID;
        enType_ = pyramid5;
    } else if (enTypeStr_ == "pyramid13") {
        valid_ = true;
        numCorn_ = 13;
        dim_ = D3;
        vistleType_ = cell::PYRAMID;
        enType_ = pyramid13;
    } else if (enTypeStr_ == "hexa8") {
        valid_ = true;
        numCorn_ = 8;
        dim_ = D3;
        vistleType_ = cell::HEXAHEDRON;
        enType_ = hexa8;
    } else if (enTypeStr_ == "hexa20") {
        valid_ = true;
        numCorn_ = 20;
        dim_ = D3;
        vistleType_ = cell::HEXAHEDRON;
        enType_ = hexa20;
    } else if (enTypeStr_ == "penta6") {
        valid_ = true;
        numCorn_ = 6;
        dim_ = D3;
        vistleType_ = cell::PRISM;
        enType_ = penta6;
    } else if (enTypeStr_ == "penta15") {
        valid_ = true;
        numCorn_ = 15;
        dim_ = D3;
        vistleType_ = cell::PRISM;
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
    // we allow a dummy element that's the only one which is empty
    else if (name.find("dummy") != std::string::npos) {
        valid_ = false;
        empty_ = true;
    }

    check();
}

EnElement::EnElement(const EnElement &e)
: valid_(e.valid_)
, empty_(e.empty_)
, ghost_(e.ghost_)
, numCorn_(e.numCorn_)
, dim_(e.dim_)
, vistleType_(e.vistleType_)
, enType_(e.enType_)
, enTypeStr_(trim_copy(e.enTypeStr_))
, dataBlanklist_(e.dataBlanklist_)
{
    check();
}

EnElement::EnElement(EnElement &&e)
: valid_(e.valid_)
, empty_(e.empty_)
, ghost_(e.ghost_)
, numCorn_(e.numCorn_)
, dim_(e.dim_)
, vistleType_(e.vistleType_)
, enType_(e.enType_)
, enTypeStr_(trim_copy(e.enTypeStr_))
, dataBlanklist_(std::move(e.dataBlanklist_))
{
    check();
}

const EnElement &EnElement::operator=(const EnElement &e)
{
    if (this == &e)
        return *this;

    valid_ = e.valid_;
    empty_ = e.empty_;
    ghost_ = e.ghost_;
    numCorn_ = e.numCorn_;
    dim_ = e.dim_;
    vistleType_ = e.vistleType_;
    enType_ = e.enType_;
    enTypeStr_ = trim_copy(e.enTypeStr_);
    dataBlanklist_ = e.dataBlanklist_;

    check();

    return *this;
}

bool EnElement::check() const
{
    if (!valid_) {
        return true;
    }
    if (empty_) {
        return true;
    }
    if (vistleType_ == cell::NONE) {
        CERR << "invalid Vistle type" << std::endl;
        abort();
        return false;
    }
    return true;
}

// higher order elements like hexa20 are treated as linear elements (e.g. like hexa8)
size_t EnElement::remap(const unsigned *cornIn, unsigned *cornOut)
{
    // only 2D elements have to be remapped
    // at the first stage
    if (cornOut != nullptr) {
        switch (enType_) {
        case tria3:
        case tria6:
            cornOut[0] = cornIn[0];
            cornOut[1] = cornIn[2];
            cornOut[2] = cornIn[1];
            break;
        case quad4:
        case quad8:
            cornOut[0] = cornIn[0];
            cornOut[1] = cornIn[3];
            cornOut[2] = cornIn[2];
            cornOut[3] = cornIn[1];
            break;
        default:
            memcpy(cornOut, cornIn, getNumberOfVistleCorners() * sizeof(unsigned));
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

size_t EnElement::getNumberOfVistleCorners() const
{
    assert(valid_);
    assert(vistleType_ < vistle::cell::NUM_TYPES);
    return vistle::UnstructuredGrid::NumVertices[vistleType_];
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

void EnElement::setBlanklist(std::vector<vistle::Byte> &&bl)
{
    dataBlanklist_ = bl;
}

const std::vector<vistle::Byte> &EnElement::getBlanklist() const
{
    assert(valid_);
    return dataBlanklist_;
}

//
// Destructor
//
EnElement::~EnElement()
{}
