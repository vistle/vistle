#ifndef VISTLE_READENSIGHT_ENPART_H
#define VISTLE_READENSIGHT_ENPART_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CLASS    EnPart
//
// Description: general data class for the handling of parts of EnSight geometry

//
// Initial version: 27.06.2005
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2005 by VISENSO GmbH
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "EnElement.h"

#include <vistle/core/object.h>

#include <array>
#include <vector>
#include <string>
#include <ostream>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>


class EnPart;

//
// object to allow bookkeeping of parts
//
class EnPart {
    friend std::ostream &operator<<(std::ostream &os, const EnPart &p);
    friend class boost::serialization::access;

public:
    enum GeoMode { INVALID, UNSPECIFIED, NO_CHANGE, COORD_CHANGE, CONN_CHANGE };

    EnPart();
    EnPart(int pNum, const std::string &comment = "");
    EnPart(const EnPart &p);
    EnPart(EnPart &&p);
    ~EnPart();

    const EnPart &operator=(const EnPart &p);
    const EnPart &operator=(EnPart &&p);

    bool check() const;
    void clear();

    // add element and number of elements to part data
    void addElement(EnElement &&ele, size_t anz, bool complete = true);

    // remove all elements from part
    void clearElements();

    // print out part object to stderr - debug
    void print(std::ostream &os) const;

    void setStartPos(ssize_t pos, const std::string &filename = std::string());
    ssize_t startPos(const std::string &filename = std::string()) const;

    static std::string partInfoHeader();
    static std::string partInfoFooter();
    // returns a string formatted according to partInfoHeader
    std::string partInfoString(int block = -1) const;

    void setPartNum(int partNum);
    // returns part number
    int getPartNum() const;

    // a part object is empty is it got not filled yet
    bool isEmpty() const;
    unsigned getDim() const;
    bool hasDim(int dim) const;

    // find Element by name
    // we assume that each element occurs only once
    // in a given part
    const EnElement *findElementType(const std::string &name) const;
    const EnElement *findElementType(EnElement::Type type) const;

    // returns number of cells per element
    size_t getElementNum(const std::string &name) const;
    size_t getElementNum(EnElement::Type type) const;

    // return the total number of elements contained in *this
    size_t getTotNumEle(int dim = -1) const;

    // return the number of different element-types contained in *this
    size_t getNumEle() const;

    void setComment(const std::string &comm);
    void setComment(const char *ch);
    std::string comment() const;

    // set number of Coords - needed for GOLD
    void setNumCoords(const ssize_t n);

    size_t numCoords() const;

    size_t numEleRead(int dim) const;
    void setNumEleRead(int dim, size_t n);
    size_t numConnRead(int dim) const;
    void setNumConnRead(int dim, size_t n);

    // delete all fields (see below) and reset pointers to nullptr
    void clearCoords();

    GeoMode geoMode() const;
    void setGeoMode(GeoMode mode);
    void copyConn(const EnPart &refPart);
    void copyCoord(const EnPart &refPart);

    // these pointers have to be used with care
    vistle::ShmVector<vistle::Scalar> x3d_, y3d_, z3d_;
    std::array<vistle::ShmVector<vistle::Index>, 4> el_;
    std::array<vistle::ShmVector<vistle::Index>, 4> cl_;
    std::array<vistle::ShmVector<vistle::Byte>, 4> tl_;

private:
    std::map<std::string, ssize_t> startPos_;
    GeoMode geoMode_ = INVALID;
    int partNum_ = -1;
    std::vector<EnElement> elementList_;
    std::vector<size_t> sumNumList_;
    std::array<std::vector<size_t>, 4> numList_;
    bool empty_ = true;
    std::string comment_;
    // if true, part will be used to build up grid and mapped data
    ssize_t numCoords_ = 0;

    std::array<size_t, 4> numEleRead_ = {};
    std::array<size_t, 4> numConnRead_ = {};

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &startPos_;
        ar &geoMode_;
        ar &partNum_;
        ar &elementList_;
        ar &numList_;
        ar &empty_;
        ar &comment_;
        ar &numCoords_;
        ar &numEleRead_;
        ar &numConnRead_;
    }
};

typedef std::vector<EnPart> PartList;

bool hasPartWithDim(const PartList &pl, int dim);
EnPart *findPart(const PartList &pl, int partNum);

std::ostream &operator<<(std::ostream &os, const EnPart &p);

#endif
