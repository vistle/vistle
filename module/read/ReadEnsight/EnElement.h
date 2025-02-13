#ifndef VISTLE_READENSIGHT_ENELEMENT_H
#define VISTLE_READENSIGHT_ENELEMENT_H

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS    EnElement
//
// Description: general data class for the handling of EnSight geometry elements
//
// CLASS    EnPart
//
// Description: general data class for the handling of parts of EnSight geometry

//
// Initial version: 05.06.2002
//
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2002 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Changes:
//

#include <vector>
#include <string>

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <vistle/core/celltypes.h>

class EnPart;

typedef std::vector<EnPart> PartList;

//
// describes a EnSight element as class of elements
// it contains NO specific information about real elements (like corner indices)
// in out model but serves as a skelton for the elemts in a given EnSight part
//
class EnElement {
    friend class boost::serialization::access;

public:
    enum Dimensionality { D0, D1, D2, D3 };
    enum Type {
        point,
        bar2,
        bar3,
        tria3,
        tria6,
        quad4,
        quad8,
        tetra4,
        tetra10,
        pyramid5,
        pyramid13,
        hexa8,
        hexa20,
        penta6,
        penta15,
        nsided,
        nfaced
    };

    /// default CONSTRUCTOR
    EnElement();

    EnElement(const std::string &name);

    /// copy constructor
    EnElement(const EnElement &e);

    const EnElement &operator=(const EnElement &e);

    /// DESTRUCTOR
    ~EnElement();

    // return the dimEnsionality of element
    // i.e. 2D 3D
    Dimensionality getDim() const;

    // is it a valid EnSight element
    bool valid() const;

    // return the number of corners
    size_t getNumberOfCorners() const;
    size_t getNumberOfVistleCorners() const;

    // return Vistle type
    vistle::cell::CellType getCovType() const;

    // return EnSight type
    Type getEnType() const;

    bool empty() const;

    // remap: either resort element corners or make new connectivity
    size_t remap(unsigned *cornIn, unsigned *cornOut);

    // return EnSight type as a std::string
    std::string getEnTypeStr() const;

    // returns true if cell is not degenerated, i.e. a point
    bool hasDistinctCorners(const unsigned *ci) const;

    // returns true if cell is fully degenerated i.e. a point
    unsigned distinctCorners(const unsigned *ci, unsigned *co) const;

    void setBlanklist(std::vector<int> &&bl);

    const std::vector<int> &getBlanklist() const;

private:
    bool valid_ = false;
    bool empty_ = true;
    unsigned numCorn_ = 0;
    Dimensionality dim_;
    vistle::cell::CellType vistleType_ = vistle::cell::NONE;
    Type enType_;

    size_t startIdx_ = 0;
    size_t endIdx_ = 0;

    std::string enTypeStr_;

    std::vector<int> dataBlanklist_;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar &valid_;
        ar &empty_;
        ar &numCorn_;
        ar &dim_;
        ar &vistleType_;
        ar &enType_;
        ar &startIdx_;
        ar &endIdx_;
        ar &enTypeStr_;
        ar &dataBlanklist_;
    }
};

#endif
