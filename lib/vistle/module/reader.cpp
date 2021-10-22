#include "reader.h"

namespace vistle {

Reader::Reader(const std::string &name, const int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    setCurrentParameterGroup("Reader");

    m_first = addIntParameter("first_step", "first timestep to read", 0);
    setParameterRange(m_first, Integer(0), std::numeric_limits<Integer>::max());

    m_last = addIntParameter("last_step", "last timestep to read (-1: last)", -1);
    setParameterRange(m_last, Integer(-1), std::numeric_limits<Integer>::max());

    m_increment = addIntParameter("step_increment", "number of steps to increment", 1);

    m_firstRank = addIntParameter("first_rank", "rank for first partition of first timestep", 0);
    setParameterRange(m_firstRank, Integer(0), Integer(comm.size() - 1));

    m_checkConvexity =
        addIntParameter("check_convexity", "whether to check convexity of grid cells", 0, Parameter::Boolean);

    setCurrentParameterGroup();

    assert(m_concurrency);
    setParameterRange(m_concurrency, Integer(-1), Integer(hardware_concurrency() * 5));
}

Reader::~Reader()
{}

void Reader::prepareQuit()
{
    m_observedParameters.clear();
    Module::prepareQuit();
}

bool Reader::checkConvexity() const
{
    return m_checkConvexity->getValue();
}

int Reader::rankForTimestepAndPartition(int t, int p) const
{
    bool distTime = m_distributeTime && m_distributeTime->getValue();
    int baseRank = (int)m_firstRank->getValue();

    if (p < 0) {
        if (m_numPartitions > 0)
            // don't read unpartitioned data if there are partitions
            return -1;

        p = 0;
    }

    if (t < 0)
        t = 0;
    if (!distTime)
        t = 0;

    int np = m_numPartitions;
    if (np < 1)
        np = 1;
    if (np == comm().size()) {
        // ensure that consecutive timesteps of one partition do not map onto same rank with m_distributeTime
        ++np;
    }

    return (t * np + p + baseRank) % comm().size();
}

int Reader::numPartitions() const
{
    return m_numPartitions;
}

size_t Reader::waitForReaders(size_t maxRunning, bool &result)
{
    while (m_tokens.size() > maxRunning) {
        auto token = m_tokens.front();
        if (!token->result()) {
            sendInfo("read task %lu failed", token->id());
            result = false;
        }
        m_tokens.pop_front();
    }

    return m_tokens.size();
}

/**
 * @brief Calls read function for corresponding parallelizationmode for given timestep blockparallel.
 *
 * @param prev Previous Token.
 * @param prop Reader properties.
 * @param timestep Current timestep.
 */
void Reader::readTimestep(std::shared_ptr<Token> prev, ReaderProperties &prop, int timestep, int step)
{
    for (int p = -1; p < prop.numpart; ++p) {
        if (!m_handlePartitions || comm().rank() == rankForTimestepAndPartition(step, p)) {
            prop.meta->setBlock(p);
            auto token = std::make_shared<Token>(this, prev);
            ++m_tokenCount;
            token->m_id = m_tokenCount;
            token->m_meta = *prop.meta;
            if (waitForReaders(prop.concurrency - 1, prop.result) == 0)
                prev.reset();
            m_tokens.emplace_back(token);
            prev = token;
            token->m_future = std::async(std::launch::async, [this, token, timestep, p]() {
                if (!read(*token, timestep, p)) {
                    sendInfo("error reading time data %d on partition %d", timestep, p);
                    return false;
                }
                return true;
            });
        }
        if (cancelRequested()) {
            waitForReaders(0, prop.result);
            prev.reset();
            prop.result = false;
        }
        if (!prop.result)
            break;
    }
    if (m_parallel == ParallelizeBlocks) {
        waitForReaders(0, prop.result);
        prev.reset();
    }
}

/**
 * @brief Read timesteps.
 *
 * @param prev Previous token.
 * @param prop Reader properties.
 */
void Reader::readTimesteps(std::shared_ptr<Token> prev, ReaderProperties &prop)
{
    if (prop.result && prop.time.inc() != 0) {
        int step = 0;
        for (int t = prop.time.first(); prop.time.inc() < 0 ? t >= prop.time.last() : t <= prop.time.last();
             t += prop.time.inc()) {
            prop.meta->setTimeStep(step);
            readTimestep(prev, prop, t, step);
            ++step;
            if (!prop.result)
                break;
        }

        waitForReaders(0, prop.result);
        prev.reset();
    }
}

/**
 * @brief Prepares reader for reading file. Calls the read function for choosen ParallelizationMode.
 *
 * @return True if everything went well.
 */
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
        last = m_numTimesteps - 1;
    auto inc = m_increment->getValue();
    auto rTime = ReaderTime(first, last, inc);

    int numpart = m_numPartitions;
    if (!m_handlePartitions)
        numpart = 0;

    Meta meta;
    meta.setNumBlocks(m_numPartitions);
    auto numtime = rTime.calc_numtime();
    meta.setNumTimesteps(numtime);

    int concurrency = m_concurrency->getValue();
    if (concurrency <= 0)
        concurrency = std::thread::hardware_concurrency() / 2;
    if (concurrency <= 0)
        concurrency = 1;
    if (m_parallel == Serial)
        concurrency = 1;
    assert(concurrency >= 1);

    if (rank() == 0)
        sendInfo("reading %d timesteps with up to %d partitions", numtime, numpart);

    //read const parts
    std::shared_ptr<Token> prev;
    meta.setTimeStep(-1);
    ReaderProperties prop(&meta, rTime, numpart, concurrency, true);
    readTimestep(prev, prop, -1, -1);

    // read timesteps
    readTimesteps(prev, prop);

    //finish read
    if (!finishRead()) {
        sendInfo("error finishing read");
        prop.result = false;
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

/**
 * @brief Set parallelize mode per mpi process.
 *
 * @param mode enum.
 */
void Reader::setParallelizationMode(Reader::ParallelizationMode mode)
{
    m_parallel = mode;

    if (m_parallel == ParallelizeTimesteps) {
        setAllowTimestepDistribution(true);
    }
}

void Reader::setHandlePartitions(bool enable)
{
    m_handlePartitions = enable;
}

/**
 * @brief Allow timestep distribution across MPI processes.
 *
 * @param allow Adds a bool parameter to the reader if true and enables it when parallem mode is set to ParallelizeTimesteps.
 */
void Reader::setAllowTimestepDistribution(bool allow)
{
    m_allowTimestepDistribution = allow;
    if (m_allowTimestepDistribution) {
        if (!m_distributeTime) {
            setCurrentParameterGroup("Reader");
            m_distributeTime = addIntParameter("distribute_time", "distribute timesteps across MPI ranks",
                                               m_parallel == ParallelizeTimesteps, Parameter::Boolean);
            setCurrentParameterGroup();
        }
    } else {
        if (m_distributeTime)
            removeParameter(m_distributeTime);
        m_distributeTime = nullptr;
    }
}

void Reader::observeParameter(const Parameter *param)
{
    m_observedParameters.insert(param);
    m_readyForRead = false;
}

void Reader::setTimesteps(int number)
{
    Integer max(std::numeric_limits<Integer>::max());
    if (number < 0) {
        number = 0;
    }

    m_numTimesteps = number;

    if (number == 0) {
        number = 1;
    } else {
        max = number - 1;
    }

    if (m_first->getValue() >= max) {
        setParameter(m_first, max);
    }
    setParameterRange(m_first, Integer(0), max);

    if (m_last->getValue() >= max) {
        setParameter(m_last, max);
    }
    setParameterRange(m_last, Integer(-1), max);
}

void Reader::setPartitions(int number)
{
    if (number < 0)
        number = 0;
    m_numPartitions = number;
}

bool Reader::changeParameters(std::set<const Parameter *> params)
{
    bool ret = true;
    for (auto &p: params)
        ret &= Module::changeParameter(p);

    for (auto &p: params) {
        auto it = m_observedParameters.find(p);
        if (it != m_observedParameters.end()) {
            m_readyForRead = examine();
            ret &= m_readyForRead;
            break;
        }
    }

    return ret;
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

const Meta &Reader::Token::meta() const
{
    return m_meta;
}

bool Reader::Token::wait(const std::string &port)
{
    if (m_previous) {
        if (port.empty())
            return m_previous->waitDone();
        else
            return m_previous->waitPortReady(port);
    }

    return true;
}

bool Reader::Token::addObject(Port *port, Object::ptr obj)
{
    if (!port)
        return false;
    std::string name = port->getName();
    return addObject(name, obj);
}

bool Reader::Token::addObject(const std::string &port, Object::ptr obj)
{
    if (!obj)
        return false;

    applyMeta(obj);
#ifdef DEBUG
    if (obj)
        std::cerr << "waiting to add object to port " << port << ", t=" << obj->getTimestep() << std::endl;
    else
        std::cerr << "waiting to add object to port " << port << ": (null)" << std::endl;
#endif
    wait(port);
#ifdef DEBUG
    if (obj)
        std::cerr << "adding object to port " << port << ", t=" << obj->getTimestep() << std::endl;
    else
        std::cerr << "adding object to port " << port << ": (null)" << std::endl;
#endif
    bool ret = false;
    {
        std::lock_guard<std::mutex> locker(m_reader->m_mutex);
        ret = m_reader->addObject(port, obj);
    }
    setPortReady(port, ret);
    return ret;
}

bool Reader::Token::result()
{
    return waitDone();
}

bool Reader::Token::waitDone()
{
    //#ifdef DEBUG
    std::cerr << "Reader::Token: finishing " << id() << ", meta: " << m_meta << "..." << std::endl;
    //#endif
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_finished)
            return m_result;

        if (!m_future.valid()) {
            std::cerr << "Reader::Token: finishing " << id() << ", but future not valid 1" << std::endl;
            m_finished = true;
            m_result = false;
        }
    }

    if (!m_finished) {
        if (m_future.valid()) {
            try {
                m_future.wait();
            } catch (std::exception &ex) {
                std::cerr << "Reader::Token: finishing " << id() << ", but future throws: " << ex.what() << std::endl;
                std::lock_guard<std::mutex> locker(m_mutex);
                m_finished = true;
                m_result = false;
            }
        } else {
            std::cerr << "Reader::Token: finishing " << id() << ", but future not valid 2" << std::endl;
            std::lock_guard<std::mutex> locker(m_mutex);
            m_finished = true;
            m_result = false;
        }
    }

    std::lock_guard<std::mutex> locker(m_mutex);
    if (!m_finished && !m_future.valid()) {
        std::cerr << "Reader::Token: finishing " << id() << ", but future not valid 3" << std::endl;
        m_finished = true;
        m_result = false;
    }

    if (!m_finished) {
        try {
            m_result = m_future.get();
        } catch (std::exception &ex) {
            std::cerr << "Reader::Token: finishing " << id() << ", but future throws: " << ex.what() << std::endl;
            m_finished = true;
            m_result = false;
        } catch (...) {
            std::cerr << "Reader::Token: finishing " << id() << ", but future throws unknown exception" << std::endl;
            m_finished = true;
            m_result = false;
        }
    }

    for (auto p: m_ports) {
        auto ps = p.second;
        if (!ps->valid) {
            ps->valid = true;
            ps->promise.set_value(m_result);
        }
    }
    m_ports.clear();

    m_finished = true;
    return m_result;
}

bool Reader::Token::waitPortReady(const std::string &port)
{
    std::shared_ptr<PortState> p;
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_finished)
            return m_result;

        if (!m_future.valid())
            return false;

        auto it = m_ports.find(port);
        if (it == m_ports.end())
            it = m_ports.emplace(port, std::make_shared<PortState>()).first;
        p = it->second;
        if (p->valid) {
            if (!p->future.valid())
                return p->ready;
        }
    }

    assert(p->future.valid());
    p->future.wait();
    std::lock_guard<std::mutex> locker(m_mutex);
    p->valid = true;
    p->ready = p->future.get();
    return p->ready;
}

void Reader::Token::setPortReady(const std::string &port, bool ready)
{
#ifdef DEBUG
    std::cerr << "setting port ready: " << port << "=" << ready << std::endl;
#endif
    std::lock_guard<std::mutex> locker(m_mutex);
    assert(!m_finished);

    auto &p = m_ports[port];
    if (!p) {
        p.reset(new PortState);
    }
    if (p->valid) {
        std::cerr << "setting port ready but already valid: " << port << "=" << ready << std::endl;
    }
    assert(!p->valid);
    p->valid = true;
    p->promise.set_value(ready);
}

void Reader::Token::applyMeta(Object::ptr obj) const
{
    if (!obj)
        return;

    obj->setTimestep(m_meta.timeStep());
    obj->setNumTimesteps(m_meta.numTimesteps());
    obj->setBlock(m_meta.block());
    obj->setNumBlocks(m_meta.numBlocks());
}

unsigned long Reader::Token::id() const
{
    return m_id;
}

std::ostream &operator<<(std::ostream &os, const Reader::Token &tok)
{
    os << "id: " << tok.id() << ", ports:";
    for (const auto &p: tok.m_ports) {
        os << " " << p.first;
    }
    return os;
}

Reader::Token::Token(Reader *reader, std::shared_ptr<Token> previous): m_reader(reader), m_previous(previous)
{}

Reader *Reader::Token::reader() const
{
    return m_reader;
}

int Reader::ReaderTime::calc_numtime()
{
    int numtime{-1};
    if (m_inc > 0) {
        if (m_last >= m_first)
            numtime = (m_last - m_first) / m_inc + 1;
    } else if (m_inc < 0) {
        if (m_last <= m_first)
            numtime = (m_last - m_first) / m_inc + 1;
    }
    return numtime;
}

} // namespace vistle
