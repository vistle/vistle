// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++                                               (C)2005 VISENSO GmbH  ++
// ++ Description:                                                        ++
// ++             Implementation of class EnPart                          ++
// ++                                                                     ++
// ++ Author:  Ralf Mikulla (rm@visenso.de)                            ++
// ++                                                                     ++
// ++               VISENSO GmbH                                        ++
// ++               Nobelstrasse 15                                       ++
// ++               70569 Stuttgart                                       ++
// ++                                                                     ++
// ++ Date: 05.06.2002                                                    ++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "EnPart.h"
#include <numeric>
#include <iostream>

#define HTML

#define CERR std::cerr << "EnPart::" << __func__ << ": "

EnPart::EnPart()
{}

EnPart::EnPart(int pNum, const std::string &comment): partNum_(pNum), comment_(comment)
{}

EnPart::EnPart(EnPart &&p)
: x3d_(p.x3d_)
, y3d_(p.y3d_)
, z3d_(p.z3d_)
, el2d_(p.el2d_)
, cl2d_(p.cl2d_)
, el3d_(p.el3d_)
, cl3d_(p.cl3d_)
, tl2d_(p.tl2d_)
, tl3d_(p.tl3d_)
, startPos_(p.startPos_)
, partNum_(p.partNum_)
, empty_(p.empty_)
, comment_(p.comment_)
, numCoords_(p.numCoords_)
, numEleRead2d_(p.numEleRead2d_)
, numEleRead3d_(p.numEleRead3d_)
, numConnRead2d_(p.numConnRead2d_)
, numConnRead3d_(p.numConnRead3d_)
{
    elementList_ = std::move(p.elementList_);
    numList_ = std::move(p.numList_);
    numList2d_ = std::move(p.numList2d_);
    numList3d_ = std::move(p.numList3d_);
}

EnPart::~EnPart()
{
    clearElements();
    clearFields();
}

const EnPart &EnPart::operator=(const EnPart &p)
{
    if (this == &p)
        return *this;

    startPos_ = p.startPos_;
    partNum_ = p.partNum_;
    empty_ = p.empty_;
    comment_ = p.comment_;
    numCoords_ = p.numCoords_;
    numEleRead2d_ = p.numEleRead2d_;
    numConnRead2d_ = p.numConnRead2d_;
    numEleRead3d_ = p.numEleRead3d_;
    numConnRead3d_ = p.numConnRead3d_;

    el2d_ = p.el2d_;
    cl2d_ = p.cl2d_;
    tl2d_ = p.tl2d_;

    el3d_ = p.el3d_;
    cl3d_ = p.cl3d_;
    tl3d_ = p.tl3d_;

    x3d_ = p.x3d_;
    y3d_ = p.y3d_;
    z3d_ = p.z3d_;

    elementList_ = p.elementList_;
    numList_ = p.numList_;
    numList2d_ = p.numList2d_;
    numList3d_ = p.numList3d_;

    return *this;
}

void EnPart::setStartPos(long pos, const std::string &name)
{
    //CERR << name << " " << pos << std::endl;
    startPos_[name] = pos;
}

long EnPart::startPos(const std::string &name) const
{
    auto it = startPos_.find(name);
    if (it == startPos_.end()) {
        //CERR << "startPos not found for file " << name << std::endl;
        return 0;
    }
    return it->second;
}

bool EnPart::isEmpty() const
{
    return empty_;
}

unsigned EnPart::getDim() const
{
    if (!numList3d_.empty()) {
        return 3;
    }

    if (!numList2d_.empty()) {
        return 2;
    }

    if (isEmpty()) {
        return 0;
    }

    return 0;
}

bool EnPart::hasDim(int dim) const
{
    if (dim == 3 && !numList3d_.empty()) {
        return true;
    }

    if (dim == 2 && !numList2d_.empty()) {
        return true;
    }

    return false;
}

void EnPart::setComment(const std::string &c)
{
    comment_ = c;
}

void EnPart::setComment(const char *ch)
{
    comment_ = ch;
}

std::string EnPart::comment() const
{
    return comment_;
}

void EnPart::addElement(const EnElement &ele, const size_t anz, bool complete)
{
    // we add only valid elements
    if (ele.valid()) {
        empty_ = false;
        elementList_.push_back(ele);
        numList_.push_back(anz);

        if (ele.getDim() == EnElement::D2) {
            numList2d_.push_back(anz);
        }
        if (ele.getDim() == EnElement::D3) {
            numList3d_.push_back(anz);
        }

        if (complete && ele.getBlanklist().size() != anz) {
            CERR << "adding elements without blanklist" << std::endl;
            abort();
        }
    }
}

void EnPart::clearElements()
{
    elementList_.clear();
    numList_.clear();
    numList2d_.clear();
    numList3d_.clear();
}

void EnPart::print(std::ostream &os) const
{
    os << "PART " << partNum_ << std::endl;
    os << "   COMMENT: " << comment_ << std::endl;

    for (size_t i = 0; i < elementList_.size(); ++i) {
        os << "   " << numList_[i] << " elements of type " << elementList_[i].getCovType() << "/"
           << elementList_[i].getEnTypeStr() << "/" << elementList_[i].getEnType() << std::endl;
    }

    os << "   numCoords " << numCoords_ << std::endl;

#define p(a) "  " << #a << " " << (a##_ ? "yes" : "no  ")
    os << p(el2d) << p(cl2d) << std::endl;
    os << p(el3d) << p(cl3d) << std::endl;

    os << "   numEleRead3d  " << numEleRead3d_ << "   numEleRead2d  " << numEleRead2d_ << std::endl;
    os << "   numConnRead3d " << numConnRead3d_ << "   numConnRead2d " << numConnRead2d_ << std::endl;

    os << p(x3d) << p(y3d) << p(z3d) << std::endl;

    os << "   files:" << std::endl;
    for (auto &f: startPos_) {
        os << "      " << f.first << " -> " << f.second << std::endl;
    }
}

void EnPart::setPartNum(const int partNum)
{
    partNum_ = partNum;
}

int EnPart::getPartNum() const
{
    return partNum_;
}

EnElement EnPart::findElementType(EnElement::Type type) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnType() == type) {
            return elementList_[i];
        }
    }
    EnElement e;
    return e;
}

EnElement EnPart::findElementType(const std::string &name) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnTypeStr() == name) {
            return elementList_[i];
        }
    }
    EnElement e;
    return e;
}

size_t EnPart::getElementNum(EnElement::Type type) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnType() == type) {
            return numList_[i];
        }
    }
    return 0;
}

size_t EnPart::getElementNum(const std::string &name) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnTypeStr() == name) {
            return numList_[i];
        }
    }
    return 0;
}

size_t EnPart::getTotNumEle() const
{
    return std::accumulate(numList_.begin(), numList_.end(), 0);
}

size_t EnPart::getTotNumEle2d() const
{
    return std::accumulate(numList2d_.begin(), numList2d_.end(), 0);
}

size_t EnPart::getTotNumEle3d() const
{
    return std::accumulate(numList3d_.begin(), numList3d_.end(), 0);
}

size_t EnPart::getTotNumCorners2d()
    const // doesnt work with nsided since that element type doesnt have a specific number of corners
{
    size_t ret = 0, anz = 0, nc = 0;
    auto it(numList2d_.begin());
    auto itEle(elementList_.begin());
    while (it != numList2d_.end()) {
        anz = *it;
        nc = itEle->getNumberOfCorners();
        ret += nc * anz;
        ++it;
        ++itEle;
    }
    return ret;
}

size_t EnPart::getTotNumCorners3d()
    const // doesnt work with nfaced since that element type doesnt have a specific number of corners
{
    size_t ret = 0, anz = 0, nc = 0;
    auto it(numList3d_.begin());
    auto itEle(elementList_.begin());
    while (it != numList3d_.end()) {
        anz = *it;
        nc = itEle->getNumberOfCorners();
        ret += nc * anz;
        ++it;
        ++itEle;
    }
    return ret;
}

size_t EnPart::getNumEle() const
{
    return elementList_.size();
}

namespace {
const std::string cellStyle = "style=\"padding-right:5px; padding-left:0px;\"";
}

std::string EnPart::partInfoHeader()
{
    std::string header = "EnSight Parts:\n";
#ifdef HTML
    std::stringstream str;
    str << "<th " << cellStyle << " align=\"left\">";
    auto sep = str.str();
    str.clear();
    str << "<th " << cellStyle << " align=\"right\">";
    auto sepr = str.str();
    header += "<table><tr>" + sepr + "Part#" + sepr + "#Elements" + sep + "Dim" + sep + "Part Description";
#else
    header += "Part#  | Dim   | #Elements | Part Description\n";
    header += "---------------------------------------------------------------------";
#endif
    return header;
}

std::string EnPart::partInfoFooter()
{
    std::string footer;
#ifdef HTML
    footer = "</table>";
#else
    footer = "---------------------------------------------------------------------";
#endif
    return footer;
}

std::string EnPart::partInfoString(int ref) const
{
    std::string infoStr;
#ifdef HTML
    std::stringstream str;
    str << "<td " << cellStyle << " align=\"left\">";
    auto sep = str.str();
    str.clear();
    str << "<td " << cellStyle << " align=\"right\">";
    auto sepr = str.str();
    infoStr = "<tr>";
    infoStr += sepr;
#else
    std::string sep = " | ";
#endif

    // ref No.
    char nStr[32];
    sprintf(nStr, "%6d", ref);
    infoStr += nStr;

    infoStr += sepr;
    // total number of elements
    size_t nTot(0);
    for (auto j = 0; j < elementList_.size(); ++j)
        nTot += numList_[j];
    sprintf(nStr, "%8zu", nTot);
    infoStr += nStr;

    infoStr += sep;
    // dimensionality
    if ((numEleRead2d_ > 0) && (numEleRead3d_ > 0))
        infoStr += "2D/3D";
    else if (numEleRead2d_ > 0)
        infoStr += "2D   ";
    else if (numEleRead3d_ > 0)
        infoStr += "3D   ";

    infoStr += sep;
    // comment
    std::string outComment(comment_);
    infoStr += outComment;

    return infoStr;
}

size_t EnPart::getTotNumberOfCorners() const
{
    size_t numCorn(0);
    for (size_t i = 0; i < elementList_.size(); ++i) {
        auto nc = elementList_[i].getNumberOfCorners();
        nc *= numList_[i];
        numCorn += nc;
    }
    return numCorn;
}

void EnPart::setNumCoords(const size_t n)
{
    numCoords_ = n;
}

size_t EnPart::numCoords() const
{
    return numCoords_;
}

size_t EnPart::numEleRead2d() const
{
    return numEleRead2d_;
}

size_t EnPart::numEleRead3d() const
{
    return numEleRead3d_;
}

void EnPart::setNumEleRead2d(size_t n)
{
    numEleRead2d_ = n;
}

void EnPart::setNumEleRead3d(size_t n)
{
    numEleRead3d_ = n;
}

size_t EnPart::numConnRead2d() const
{
    return numConnRead2d_;
}

size_t EnPart::numConnRead3d() const
{
    return numConnRead3d_;
}

void EnPart::setNumConnRead2d(size_t n)
{
    numConnRead2d_ = n;
}

void EnPart::setNumConnRead3d(size_t n)
{
    numConnRead3d_ = n;
}

void EnPart::clearFields()
{
    x3d_.reset();
    y3d_.reset();
    z3d_.reset();
    el2d_.reset();
    cl2d_.reset();
    tl2d_.reset();
    el3d_.reset();
    cl3d_.reset();
    tl3d_.reset();
}

EnPart *findPart(const PartList &pl, const int partNum)
{
    for (size_t i = 0; i < pl.size(); ++i) {
        if (pl[i].getPartNum() == partNum) {
            return const_cast<EnPart *>(&pl[i]);
        }
    }
    return nullptr;
}

std::ostream &operator<<(std::ostream &os, const EnPart &p)
{
    p.print(os);
    return os;
}

bool hasPartWithDim(const PartList &pl, int dim)
{
    for (const auto &p: pl) {
        if (p.hasDim(dim)) {
            return true;
        }
    }

    return false;
}
