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
{
    clear();
}

EnPart::EnPart(int pNum, const std::string &comment): partNum_(pNum), comment_(comment)
{
    clear();
}

EnPart::EnPart(EnPart &&p)
: x3d_(p.x3d_)
, y3d_(p.y3d_)
, z3d_(p.z3d_)
, startPos_(p.startPos_)
, geoMode_(p.geoMode_)
, partNum_(p.partNum_)
, empty_(p.empty_)
, comment_(p.comment_)
, numCoords_(p.numCoords_)
{
    elementList_ = std::move(p.elementList_);
    sumNumList_ = std::move(p.sumNumList_);

    for (int d = 0; d < 4; ++d) {
        numList_[d] = std::move(p.numList_[d]);
        numEleRead_[d] = std::move(p.numEleRead_[d]);
        numConnRead_[d] = std::move(p.numConnRead_[d]);

        el_[d] = std::move(p.el_[d]);
        cl_[d] = std::move(p.cl_[d]);
        tl_[d] = std::move(p.tl_[d]);
    }
}

EnPart::EnPart(const EnPart &p)
: x3d_(p.x3d_)
, y3d_(p.y3d_)
, z3d_(p.z3d_)
, startPos_(p.startPos_)
, geoMode_(p.geoMode_)
, partNum_(p.partNum_)
, empty_(p.empty_)
, comment_(p.comment_)
, numCoords_(p.numCoords_)
{
    elementList_ = p.elementList_;
    sumNumList_ = p.sumNumList_;

    for (int d = 0; d < 4; ++d) {
        numList_[d] = p.numList_[d];
        numEleRead_[d] = p.numEleRead_[d];
        numConnRead_[d] = p.numConnRead_[d];

        el_[d] = p.el_[d];
        cl_[d] = p.cl_[d];
        tl_[d] = p.tl_[d];
    }
}

EnPart::~EnPart()
{
    clear();
}

const EnPart &EnPart::operator=(const EnPart &p)
{
    if (this == &p)
        return *this;

    startPos_ = p.startPos_;
    geoMode_ = p.geoMode_;
    partNum_ = p.partNum_;
    empty_ = p.empty_;
    comment_ = p.comment_;
    numCoords_ = p.numCoords_;

    x3d_ = p.x3d_;
    y3d_ = p.y3d_;
    z3d_ = p.z3d_;

    elementList_ = p.elementList_;
    sumNumList_ = p.sumNumList_;

    for (int d = 0; d < 4; ++d) {
        numList_[d] = p.numList_[d];
        numEleRead_[d] = p.numEleRead_[d];
        numConnRead_[d] = p.numConnRead_[d];

        el_[d] = p.el_[d];
        cl_[d] = p.cl_[d];
        tl_[d] = p.tl_[d];
    }

    return *this;
}

bool EnPart::check() const
{
    auto numEle = getTotNumEle();
    size_t num = 0;
    for (auto t: {EnElement::bar2, EnElement::bar3, EnElement::tria3, EnElement::tria6, EnElement::quad4,
                  EnElement::quad8, EnElement::hexa8, EnElement::hexa20, EnElement::pyramid5, EnElement::pyramid13,
                  EnElement::tetra4, EnElement::tetra10, EnElement::penta6, EnElement::penta15}) {
        num += getElementNum(t);
    }
    return numEle == num;
}

void EnPart::setStartPos(ssize_t pos, const std::string &name)
{
    //CERR << name << " " << pos << std::endl;
    startPos_[name] = pos;
}

ssize_t EnPart::startPos(const std::string &name) const
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
    for (int d = 3; d >= 0; --d) {
        if (!numList_[d].empty())
            return d;
    }

    if (isEmpty()) {
        return 0;
    }

    return 0;
}

bool EnPart::hasDim(int dim) const
{
    if (!numList_[dim].empty())
        return true;

    // if completely empty, then it might change for later timesteps
    for (int d = 0; d < 4; ++d) {
        if (!numList_[d].empty()) {
            return false;
        }
    }

    return true;
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

void EnPart::addElement(EnElement &&ele, const size_t anz, bool complete)
{
    // we add only valid elements
    if (ele.valid()) {
        if (complete && ele.getBlanklist().size() != anz) {
            CERR << "adding elements without blanklist" << std::endl;
            abort();
        }

        empty_ = false;
        elementList_.emplace_back(std::move(ele));
        sumNumList_.push_back(anz);

        numList_[ele.getDim()].push_back(anz);
    }
}

void EnPart::clearElements()
{
    empty_ = true;

    elementList_.clear();
    sumNumList_.clear();

    for (int d = 0; d < 4; ++d) {
        numList_[d].clear();
        el_[d].reset();
        el_[d].construct();
        el_[d]->push_back(0);
        cl_[d].reset();
        cl_[d].construct();
        tl_[d].reset();
        tl_[d].construct();
    }
}

void EnPart::print(std::ostream &os) const
{
    os << "PART " << partNum_ << std::endl;
    switch (geoMode_) {
    case UNSPECIFIED:
        os << "   UNSPECIFIED" << std::endl;
        break;
    case NO_CHANGE:
        os << "   NO CHANGE" << std::endl;
        break;
    case COORD_CHANGE:
        os << "   COORD_CHANGE" << std::endl;
        break;
    case CONN_CHANGE:
        os << "   CONN_CHANGE" << std::endl;
        break;
    default:
        break;
    }
    os << "   COMMENT: " << comment_ << std::endl;

    for (size_t i = 0; i < elementList_.size(); ++i) {
        os << "   " << sumNumList_[i] << " elements of type " << elementList_[i].getCovType() << "/"
           << elementList_[i].getEnTypeStr() << "/" << elementList_[i].getEnType() << std::endl;
    }

    os << "   numCoords " << numCoords_ << std::endl;

    for (int d = 0; d < 4; ++d) {
        os << "el" << d << " " << (el_[d] ? "yes" : "NO ");
        os << "    ";
        os << "cl" << d << " " << (cl_[d] ? "yes" : "NO ");
        os << std::endl;
    }

    os << "   #el: 3d " << numEleRead_[3] << "  2d " << numEleRead_[2] << "  1d " << numEleRead_[1] << "  0d"
       << numEleRead_[0] << std::endl;
    os << "   #cl: 3d " << numConnRead_[3] << "  2d " << numConnRead_[2] << "  1d " << numConnRead_[1] << "  0d"
       << numConnRead_[0] << std::endl;

#define p(a) "  " << #a << ":" << (a##_ ? "yes" : "NO ")
    os << p(x3d) << p(y3d) << p(z3d) << std::endl;
#undef p

    os << "   files:" << std::endl;
    for (auto &f: startPos_) {
        os << "      " << (f.first.empty() ? "(geo)" : f.first) << " -> " << f.second << std::endl;
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

const EnElement *EnPart::findElementType(EnElement::Type type) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnType() == type) {
            return &elementList_[i];
        }
    }
    return nullptr;
}

const EnElement *EnPart::findElementType(const std::string &name) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnTypeStr() == name) {
            return &elementList_[i];
        }
    }
    return nullptr;
}

size_t EnPart::getElementNum(EnElement::Type type) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnType() == type) {
            return sumNumList_[i];
        }
    }
    return 0;
}

size_t EnPart::getElementNum(const std::string &name) const
{
    for (size_t i = 0; i < elementList_.size(); ++i) {
        if (elementList_[i].getEnTypeStr() == name) {
            return sumNumList_[i];
        }
    }
    return 0;
}

size_t EnPart::getTotNumEle(int dim) const
{
    if (dim < 0) {
        return std::accumulate(sumNumList_.begin(), sumNumList_.end(), 0);
    } else {
        assert(dim < 4);
        return std::accumulate(numList_[dim].begin(), numList_[dim].end(), 0);
    }
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
    header += "<table><tr>" + sepr + "Block#" + sepr + "Part#" + sepr + "#Elements" + sepr + "Dim" + sep + "Flags" +
              sep + "Description</tr>";
#else
    header += "Block#  | Part#  | #Elements | Dim | Flags | Description\n";
    header += "---------------------------------------------------------------------\n";
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

std::string EnPart::partInfoString(int block) const
{
    std::string infoStr;
    std::string sep = " | ";
    std::string sepr = sep;
    bool html = true;
    if (block < 0)
        html = false;
#ifdef HTML
    if (html) {
        std::stringstream str;
        str << "<td " << cellStyle << " align=\"left\">";
        sep = str.str();
        str.clear();
        str << "<td " << cellStyle << " align=\"right\">";
        sepr = str.str();
        infoStr = "<tr>";
        infoStr += sepr;
    }
#endif

    char nStr[32];
    if (block >= 0) {
        snprintf(nStr, sizeof(nStr), "%6d", block);
        infoStr += nStr;
        infoStr += sepr;
    }

    // part No.
    snprintf(nStr, sizeof(nStr), "%6d", partNum_);
    infoStr += nStr;

    infoStr += sepr;
    // total number of elements
    size_t nTot(0);
    for (auto j = 0; j < elementList_.size(); ++j)
        nTot += sumNumList_[j];
    snprintf(nStr, sizeof(nStr), "%8zu", nTot);
    infoStr += nStr;

    infoStr += sepr;
    // dimensionality
    std::string dims;
    for (int d = 0; d < 4; ++d) {
        if (numEleRead_[d] > 0) {
            if (!dims.empty())
                dims += "/";
            dims += std::to_string(d);
        }
    }
    if (!dims.empty())
        dims += "D";
    infoStr += dims;

    infoStr += sep;
    // flags
    switch (geoMode_) {
    case INVALID:
        //infoStr += "invalid";
        break;
    case UNSPECIFIED:
        //infoStr += "unspecified";
        break;
    case NO_CHANGE:
        infoStr += "no_change";
        break;
    case COORD_CHANGE:
        infoStr += "coord_change";
        break;
    case CONN_CHANGE:
        infoStr += "conn_change";
        break;
    default:
        infoStr += "unknown";
        break;
    }

    infoStr += sep;
    // comment
    std::string outComment(comment_);
    infoStr += outComment;

    return infoStr;
}

void EnPart::setNumCoords(const ssize_t n)
{
    numCoords_ = n;
    if (n < 0) {
        x3d_.reset();
        y3d_.reset();
        z3d_.reset();
        return;
    }

    if (x3d_.valid())
        x3d_->resize(n);
    else
        x3d_.construct(n);

    if (y3d_.valid())
        y3d_->resize(n);
    else
        y3d_.construct(n);

    if (z3d_.valid())
        z3d_->resize(n);
    else
        z3d_.construct(n);
}

size_t EnPart::numCoords() const
{
    return numCoords_;
}

size_t EnPart::numEleRead(int dim) const
{
    return numEleRead_[dim];
}

void EnPart::setNumEleRead(int dim, size_t n)
{
    numEleRead_[dim] = n;
}

size_t EnPart::numConnRead(int dim) const
{
    return numConnRead_[dim];
}

void EnPart::setNumConnRead(int dim, size_t n)
{
    numConnRead_[dim] = n;
}

void EnPart::clear()
{
    clearElements();
    clearCoords();
}

void EnPart::clearCoords()
{
    x3d_.reset();
    y3d_.reset();
    z3d_.reset();
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

EnPart::GeoMode EnPart::geoMode() const
{
    return geoMode_;
}

void EnPart::setGeoMode(GeoMode mode)
{
    geoMode_ = mode;
}

void EnPart::copyConn(const EnPart &refPart)
{
    empty_ = refPart.empty_;

    elementList_ = refPart.elementList_;
    sumNumList_ = refPart.sumNumList_;

    for (int d = 0; d < 4; ++d) {
        numList_[d] = refPart.numList_[d];
        numEleRead_[d] = refPart.numEleRead_[d];
        numConnRead_[d] = refPart.numConnRead_[d];

        el_[d] = refPart.el_[d];
        cl_[d] = refPart.cl_[d];
        tl_[d] = refPart.tl_[d];
    }
}

void EnPart::copyCoord(const EnPart &refPart)
{
    numCoords_ = refPart.numCoords_;
    x3d_ = refPart.x3d_;
    y3d_ = refPart.y3d_;
    z3d_ = refPart.z3d_;
}
