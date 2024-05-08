//-------------------------------------------------------------------------
// PRINT METADATA H
// * Prints Meta-Data  about the input object to the Vistle console
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#ifndef PRINTMETADATA_H
#define PRINTMETADATA_H

#include <vector>

#include <vistle/module/module.h>
#include <vistle/core/object.h>
#include <vistle/core/index.h>
#include <vistle/core/unstr.h>
#include <vistle/core/vec.h>

//-------------------------------------------------------------------------
// PRINT METADATA CLASS DECLARATION
//-------------------------------------------------------------------------
class PrintMetaData: public vistle::Module {
public:
    struct ObjectProfile {
        static const unsigned NUM_VECS = 4;
        static const unsigned NUM_STRUCTURED = 3;

        vistle::Index blocks;
        vistle::Index grids;
        vistle::Index ghostCells;
        vistle::Index normals;
        vistle::Index elements;
        vistle::Index vertices;
        std::vector<vistle::Scalar> unifMin;
        std::vector<vistle::Scalar> unifMax;
        std::vector<vistle::Index> structuredGridSize;
        std::vector<vistle::Index> vecs;
        std::vector<unsigned> types;

        ObjectProfile()
        : blocks(0)
        , grids(0)
        , ghostCells(0)
        , normals(0)
        , elements(0)
        , vertices(0)
        , unifMin(NUM_STRUCTURED, std::numeric_limits<double>::max())
        , unifMax(NUM_STRUCTURED, -std::numeric_limits<double>::max())
        , structuredGridSize(NUM_STRUCTURED, 0)
        , vecs(NUM_VECS, 0)
        {}

        // applies an operation over all members
        template<class operation>
        static ObjectProfile apply(const operation &op, const ObjectProfile &lhs, const ObjectProfile &rhs);

        // operations
        template<typename T>
        struct ProfileMaximum;
        template<typename T>
        struct ProfileMinimum;
        template<typename T>
        struct ProfilePlus;

        // serialization function for passing object over mpi
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version);
    };

    PrintMetaData(const std::string &name, int moduleID, mpi::communicator comm);
    ~PrintMetaData();

private:
    // overridden functions
    bool prepare() override;
    bool compute() override;
    bool reduce(int timestep) override;
    bool changeParameter(const vistle::Parameter *p) override;

    // helper functions
    void compute_acquireGenericData(vistle::Object::const_ptr data);
    void compute_acquireGridData(vistle::Object::const_ptr data);
    void compute_printVerbose(vistle::Object::const_ptr data);
    void reduce_printData();
    std::string reduce_conditionalProfileEntryPrint(std::string message, vistle::Index total, vistle::Index min,
                                                    vistle::Index max);
    void util_printMPIInfo(std::string printTag = "");

    // parameters
    vistle::IntParameter *m_param_doPrintTotals;
    vistle::IntParameter *m_param_doPrintMinMax;
    vistle::IntParameter *m_param_doPrintMPIInfo;
    vistle::IntParameter *m_param_doPrintVerbose;

    // private member variables
    bool m_isRootNode;

    ObjectProfile m_currentProfile;
    ObjectProfile m_minProfile;
    ObjectProfile m_maxProfile;
    ObjectProfile m_TotalsProfile;

    std::vector<std::string> m_attributesVector;
    std::string m_dataType;
    int m_executionCounter;
    int m_iterationCounter;
    int m_creator;
    int m_numBlocks;
    int m_numTotalTimesteps;
    int m_numParsedTimesteps;
    int m_numAnimationSteps;
    double m_realTime;
    vistle::Matrix4 m_transform;
    bool m_isFirstComputeCall;


    // private constants
    const int M_ROOT_NODE = 0;
    const std::string M_HORIZONTAL_RULER = "\n-----------------------------------------------------";
};

//-------------------------------------------------------------------------
// TEMPLATE METHOD DEFINITIONS
//-------------------------------------------------------------------------

// OBJECT PROFILE FUNCTION - SERIALIZE
//-------------------------------------------------------------------------
template<class Archive>
void PrintMetaData::ObjectProfile::serialize(Archive &ar, const unsigned int version)
{
    ar &blocks;
    ar &grids;
    ar &normals;
    ar &ghostCells;
    ar &elements;
    ar &vertices;
    ar &structuredGridSize;
    ar &unifMin;
    ar &unifMax;
    ar &vecs;
    ar &types;
}

// OBJECT PROFILE FUNCTION - APPLIES AN OPERATION TO ALL MEMBERS OF OBJECT PROFILE
//-------------------------------------------------------------------------
template<class operation>
PrintMetaData::ObjectProfile PrintMetaData::ObjectProfile::apply(const operation &op,
                                                                 const PrintMetaData::ObjectProfile &lhs,
                                                                 const PrintMetaData::ObjectProfile &rhs)
{
    PrintMetaData::ObjectProfile result;

    result.blocks = op(lhs.blocks, rhs.blocks);
    result.grids = op(lhs.grids, rhs.grids);
    result.normals = op(lhs.normals, rhs.normals);
    result.ghostCells = op(lhs.ghostCells, rhs.ghostCells);
    result.elements = op(lhs.elements, rhs.elements);
    result.vertices = op(lhs.vertices, rhs.vertices);

    // currently takes max/min of each individual coordinate
    for (unsigned i = 0; i < PrintMetaData::ObjectProfile::NUM_STRUCTURED; i++) {
        result.structuredGridSize[i] = op(lhs.structuredGridSize[i], rhs.structuredGridSize[i]);
        result.unifMax[i] = op(lhs.unifMax[i], rhs.unifMax[i]);
        result.unifMin[i] = op(lhs.unifMin[i], rhs.unifMin[i]);
    }

    for (unsigned i = 0; i < PrintMetaData::ObjectProfile::NUM_VECS; i++) {
        result.vecs[i] = op(lhs.vecs[i], rhs.vecs[i]);
    }

    return result;
}

// PROFILE MINIMUM OPERATION - UNSPECIALIZED MIN FUNCTION FOR USE WITH PROFILE
//-------------------------------------------------------------------------
template<typename T>
struct ProfileMinimum {
    T operator()(const T &lhs, const T &rhs) const { return std::min<T>(lhs, rhs); }
};

// PROFILE MINIMUM OPERATION - UNSPECIALIZED MIN FUNCTION FOR USE WITH PROFILE
//-------------------------------------------------------------------------
template<>
struct ProfileMinimum<PrintMetaData::ObjectProfile> {
    PrintMetaData::ObjectProfile operator()(const PrintMetaData::ObjectProfile &lhs,
                                            const PrintMetaData::ObjectProfile &rhs)
    {
        return PrintMetaData::ObjectProfile::apply(ProfileMinimum<vistle::Index>(), lhs, rhs);
    }
};

// PROFILE MAXIMUM OPERATION - SPECIALIZED MAX FUNCTION FOR USE WITH PROFILE
//-------------------------------------------------------------------------
template<typename T>
struct ProfileMaximum {
    T operator()(const T &lhs, const T &rhs) const { return std::max<T>(lhs, rhs); }
};

// PROFILE MAXIMUM OPERATION - SPECIALIZED MAX FUNCTION FOR USE WITH PROFILE
//-------------------------------------------------------------------------
template<>
struct ProfileMaximum<PrintMetaData::ObjectProfile> {
    PrintMetaData::ObjectProfile operator()(const PrintMetaData::ObjectProfile &lhs,
                                            const PrintMetaData::ObjectProfile &rhs)
    {
        return PrintMetaData::ObjectProfile::apply(ProfileMaximum<vistle::Index>(), lhs, rhs);
    }
};

// OBJECT PROFILE FUNCTION - ADDITION OPERATOR OVERLOAD
//-------------------------------------------------------------------------
PrintMetaData::ObjectProfile operator+(const PrintMetaData::ObjectProfile &lhs, const PrintMetaData::ObjectProfile &rhs)
{
    return PrintMetaData::ObjectProfile::apply(std::plus<vistle::Index>(), lhs, rhs);
}


#endif /* PRINTMETADATA_H */
