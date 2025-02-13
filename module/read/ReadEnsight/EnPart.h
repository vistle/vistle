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
//
// Changes:
//

#include "EnElement.h"

#include <vistle/core/object.h>

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
    EnPart();
    EnPart(int pNum, const std::string &comment = "");
    EnPart(EnPart &p) = delete;
    EnPart(EnPart &&p);
    ~EnPart();

    const EnPart &operator=(const EnPart &p);

    // add element and number of elements to part data
    void addElement(const EnElement &ele, size_t anz, bool complete = true);

    // remove all elements from part
    void clearElements();

    // print out part object to stderr - debug
    void print(std::ostream &os) const;

    void setStartPos(long pos, const std::string &filename = std::string());
    long startPos(const std::string &filename = std::string()) const;

    static std::string partInfoHeader();
    static std::string partInfoFooter();
    // returns a string formatted according to partInfoHeader
    std::string partInfoString(int ref = 0) const;

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
    EnElement findElementType(const std::string &name) const;
    EnElement findElementType(EnElement::Type type) const;

    // returns number of cells per element
    size_t getElementNum(const std::string &name) const;
    size_t getElementNum(EnElement::Type type) const;

    // return the total number of corners in *this
    size_t getTotNumberOfCorners() const;
    size_t getTotNumCorners2d() const;
    size_t getTotNumCorners3d() const;

    // return the total number of elements contained in *this
    size_t getTotNumEle() const;
    size_t getTotNumEle2d() const;
    size_t getTotNumEle3d() const;

    // return the total number of elements contained in *this
    // havig a dimensionality dim
    size_t getTotNumEle(int dim);

    // return the number of different element-types contained in *this
    size_t getNumEle() const;

    void setComment(const std::string &comm);
    void setComment(const char *ch);
    std::string comment() const;

    // set number of Coords - needed for GOLD
    void setNumCoords(const size_t n);

    size_t numCoords() const;

    size_t numEleRead2d() const;
    size_t numEleRead3d() const;
    void setNumEleRead2d(size_t n);
    void setNumEleRead3d(size_t n);

    size_t numConnRead2d() const;
    size_t numConnRead3d() const;
    void setNumConnRead2d(size_t n);
    void setNumConnRead3d(size_t n);

    // delete all fields (see below) and reset pointers to nullptr
    void clearFields();

    // these pointers have to be used with care
    vistle::ShmVector<vistle::Scalar> x3d_, y3d_, z3d_;
    vistle::ShmVector<vistle::Index> el2d_, cl2d_;
    vistle::ShmVector<vistle::Index> el3d_, cl3d_;
    vistle::ShmVector<vistle::Byte> tl2d_, tl3d_;

private:
    std::map<std::string, long> startPos_;
    int partNum_ = -1;
    std::vector<EnElement> elementList_;
    std::vector<size_t> numList_;
    std::vector<size_t> numList2d_;
    std::vector<size_t> numList3d_;
    bool empty_ = true;
    std::string comment_;
    // if true, part will be used to build up grid and mapped data
    size_t numCoords_ = 0;

    size_t numEleRead2d_ = 0;
    size_t numEleRead3d_ = 0;
    size_t numConnRead2d_ = 0;
    size_t numConnRead3d_ = 0;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &startPos_;
        ar &partNum_;
        ar &elementList_;
        ar &numList_;
        ar &numList2d_;
        ar &numList3d_;
        ar &empty_;
        ar &comment_;
        ar &numCoords_;
        ar &numEleRead2d_;
        ar &numEleRead3d_;
        ar &numConnRead2d_;
        ar &numConnRead3d_;
    }
};

typedef std::vector<EnPart> PartList;

bool hasPartWithDim(const PartList &pl, int dim);
EnPart *findPart(const PartList &pl, int partNum);

std::ostream &operator<<(std::ostream &os, const EnPart &p);

#endif
