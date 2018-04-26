#include "reader.h"

namespace vistle {

Reader::Reader(const std::string &description, const std::string &name, const int moduleID, mpi::communicator comm)
: Module(description, name, moduleID, comm)
{
    setCurrentParameterGroup("Reader");

    m_first = addIntParameter("first_step", "first timestep to read", 0);
    setParameterRange(m_first, Integer(0), std::numeric_limits<Integer>::max());

    m_last = addIntParameter("last_step", "last timestep to read", -1);
    setParameterRange(m_last, Integer(-1), std::numeric_limits<Integer>::max());

    m_increment = addIntParameter("step_increment", "number of steps to increment", 1);

    m_firstRank = addIntParameter("first_rank", "rank for first partition of first timestep", 0);
    setParameterRange(m_firstRank, Integer(0), Integer(comm.size()-1));

    m_checkConvexity = addIntParameter("check_convexity", "whether to check convexity of grid cells", 0, Parameter::Boolean);

    setCurrentParameterGroup();
}

Reader::~Reader() {
}

void Reader::prepareQuit() {
    m_observedParameters.clear();
    Module::prepareQuit();
}

bool Reader::checkConvexity() const {

    return m_checkConvexity->getValue();
}

int Reader::rankForTimestepAndPartition(int t, int p) const {

    bool distTime = m_distributeTime && m_distributeTime->getValue();
    int baseRank = m_firstRank->getValue();

    if (p < 0 ) {
        if (m_numPartitions > 0)
            // don't read unpartitioned data if there are partitions
            return -1;

        p = 0;
    }

    if (t < 0)
        t = 0;
    if (!distTime)
        t = 0;

    return (t+p+baseRank) % comm().size();
}

bool Reader::prepare()
{
    if (!m_readyForRead) {
        sendInfo("not ready for reading");
        return true;
    }

    if (!prepareRead()) {
        sendInfo("initiating read failed");
        return true;
    }

    auto first = m_first->getValue();
    auto last = m_last->getValue();
    if (last < 0)
        last = m_numTimesteps-1;
    auto inc = m_increment->getValue();

    int numpart = m_numPartitions;
    if (!m_handlePartitions)
        numpart = 0;

    bool result = true;

    Meta meta;
    meta.setNumBlocks(m_numPartitions);
    int numtime = -1;
    if (inc > 0) {
        if (last >= first)
            numtime = (last+inc-1 - first)/inc+1;
    } else if (inc < 0) {
        if (last <= first)
            numtime = (last+inc+1 - first)/inc+1;
    }
    meta.setNumTimesteps(numtime);

    // read constant parts
    meta.setTimeStep(-1);
    for (int p=-1; p<numpart; ++p) {
        if (!m_handlePartitions || comm().rank() == rankForTimestepAndPartition(-1, p)) {
            meta.setBlock(p);
            if (!read(meta, -1, p)) {
                sendInfo("error reading constant data on partition %d", p);
                result = false;
                break;
            }
        }
        if (cancelRequested()) {
            result = false;
            break;
        }
    }

    // read timesteps
    if (result && inc != 0) {
        int step = 0;
        for (int t=first; inc<0 ? t>=last : t<=last; t+=inc) {
            meta.setTimeStep(step);
            for (int p=-1; p<numpart; ++p) {
                if (!m_handlePartitions || comm().rank() == rankForTimestepAndPartition(step, p)) {
                    meta.setBlock(p);
                    if (!read(meta, t, p)) {
                        sendInfo("error reading time data %d on partition %d", t, p);
                        result = false;
                        break;
                    }
                }
                if (cancelRequested()) {
                    result = false;
                    break;
                }
            }
            ++step;
            if (!result)
                break;
        }
    }

    if (!finishRead()) {
        sendInfo("error inishing read");
        result = false;
    }

    return true;
}

bool Reader::compute()
{
    // work is done in prepare()
    return true;
}

bool Reader::examine(const Parameter *param)
{
    return false;
}

bool Reader::prepareRead()
{
    return true;
}

bool Reader::finishRead()
{
    return true;
}

int Reader::timeIncrement() const
{
    return m_increment->getValue();
}

void Reader::setHandlePartitions(bool enable)
{
    m_handlePartitions = enable;
}

void Reader::setAllowTimestepDistribution(bool allow)
{
    m_allowTimestepDistribution = allow;
    if (m_allowTimestepDistribution) {
        m_distributeTime = addIntParameter("distribute_time", "distribute timesteps across MPI ranks", 0, Parameter::Boolean);
    } else {
        removeParameter(m_distributeTime);
        m_distributeTime = nullptr;
    }
}

void Reader::observeParameter(const Parameter *param) {

    m_observedParameters.insert(param);
    m_readyForRead = false;
}

void Reader::setTimesteps(int number) {

    if (number < 0)
        number = 0;

    m_numTimesteps = number;

    if (number == 0)
        number = 1;

    if (m_first->getValue() >= number) {
        m_first->setValue(number-1);
    }
    setParameterRange(m_first, Integer(0), Integer(number-1));

    if (m_last->getValue() >= number) {
        m_last->setValue(number-1);
    }
    setParameterRange(m_last, Integer(-1), Integer(number-1));
}

void Reader::setPartitions(int number)
{
    if (number < 0)
        number = 0;
    m_numPartitions = number;
}

bool Reader::changeParameter(const Parameter *param)
{
    bool ret = Module::changeParameter(param);

    auto it = m_observedParameters.find(param);
    if (it != m_observedParameters.end()) {
        m_readyForRead = examine(param);
        ret &= m_readyForRead;
    }

    return ret;
}
}
