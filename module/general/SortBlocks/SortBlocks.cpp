#include <sstream>
#include <iomanip>

#include <vistle/core/object.h>
#include <vistle/core/vec.h>

#include "SortBlocks.h"
#include <vistle/util/enum.h>

using namespace vistle;

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Choices, (Rank)(BlockNumber)(Timestep))

SortBlocks::SortBlocks(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createInputPort("data_in", "partitioned data");
    createOutputPort("data_out0", "matching partitions");
    createOutputPort("data_out1", "non-matching partitions");

    m_criterionParam = addIntParameter("criterion", "Selection criterion", BlockNumber, Parameter::Choice);
    V_ENUM_SET_CHOICES(m_criterionParam, Choices);
    m_minParam = addIntParameter(
        "first_min", "Minimum number of MPI rank, block, timestep to output to first output (data_out0)", 0);
    m_maxParam = addIntParameter(
        "first_max", "Maximum number of MPI rank, block, timestep to output to first output (data_out0)", 0);
    setParameterRange(m_minParam, (Integer)0, (Integer)InvalidIndex);
    setParameterRange(m_maxParam, (Integer)0, (Integer)InvalidIndex);
    m_modulusParam = addIntParameter("modulus", "Check min/max after computing modulus (-1: disable)", (Integer)-1);
    setParameterRange(m_modulusParam, (Integer)-1, (Integer)(InvalidIndex - 1));
    m_invertParam = addIntParameter("invert", "Invert roles of 1st and 2nd output", false, Parameter::Boolean);
}

SortBlocks::~SortBlocks()
{}

bool SortBlocks::changeParameter(const Parameter *p)
{
    if (p == m_criterionParam) {
        switch (m_criterionParam->getValue()) {
        case Rank:
            setParameterRange(m_minParam, (Integer)0, (Integer)size() - 1);
            setParameterRange(m_maxParam, (Integer)0, (Integer)size() - 1);
            break;
        case BlockNumber:
        case Timestep:
        default:
            setParameterRange(m_minParam, (Integer)0, (Integer)InvalidIndex);
            setParameterRange(m_maxParam, (Integer)0, (Integer)InvalidIndex);
            break;
        }
    }
    return Module::changeParameter(p);
}

bool SortBlocks::compute()
{
    Object::const_ptr data = expect<Object>("data_in");
    if (!data)
        return true;

    const bool invert = m_invertParam->getValue();
    const Integer min = m_minParam->getValue();
    const Integer max = m_maxParam->getValue();
    const Integer modulus = m_modulusParam->getValue();

    Integer select = InvalidIndex;
    switch (m_criterionParam->getValue()) {
    case Rank:
        select = rank();
        break;
    case BlockNumber:
        select = data->getBlock();
        break;
    case Timestep:
        select = data->getTimestep();
        break;
    }

    if (modulus > 0) {
        select %= modulus;
    }

    bool first = min <= select && max >= select;
    if (invert)
        first = !first;

    auto ndata = data->clone();
    updateMeta(ndata);
    if (first) {
        addObject("data_out0", ndata);
    } else {
        addObject("data_out1", ndata);
    }

    return true;
}

MODULE_MAIN(SortBlocks)
