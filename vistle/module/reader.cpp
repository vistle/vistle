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

    int concurrency = 1;
    if (m_concurrency) {
        concurrency = m_concurrency->getValue();
        if (concurrency <= 0)
            concurrency = std::thread::hardware_concurrency();
    }

    if (m_parallel == Serial) {
        assert(concurrency == 1);
        concurrency = 1;
    }
    assert(concurrency >= 1);

    sendInfo("reading %d timesteps with up to %d partitions", numtime, numpart);

    std::shared_ptr<Token> prev;
    // read constant parts
    meta.setTimeStep(-1);
    for (int p=-1; p<numpart; ++p) {
        if (!m_handlePartitions || comm().rank() == rankForTimestepAndPartition(-1, p)) {
            meta.setBlock(p);
            m_tokens.emplace_back(std::make_shared<Token>(this, prev));
            prev = m_tokens.back();
            auto &token = *prev;
            token.m_meta = meta;
            token.m_future = std::async(std::launch::async, [this, &token, p](){
                if (!read(token, -1, p)) {
                    sendInfo("error reading constant data on partition %d", p);
                    return false;
                }
                return true;
            });
            while (m_tokens.size() >= concurrency) {
                auto &token = *m_tokens.front();
                result = token.result();
                m_tokens.pop_front();
                if (m_tokens.empty())
                    prev.reset();
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
                    m_tokens.emplace_back(std::make_shared<Token>(this, prev));
                    prev = m_tokens.back();
                    auto &token = *prev;
                    token.m_meta = meta;
                    token.m_future = std::async(std::launch::async, [this, &token, t, p](){
                        if (!read(token, t, p)) {
                            sendInfo("error reading time data %d on partition %d", t, p);
                            return false;
                        }
                        return true;
                    });
                    while (m_tokens.size() >= concurrency) {
                        auto &token = *m_tokens.front();
                        result = token.result();
                        m_tokens.pop_front();
                        if (m_tokens.empty())
                            prev.reset();
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

    while (!m_tokens.empty()) {
        auto &token = m_tokens.front();
        result &= token->result();
        m_tokens.pop_front();
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

void Reader::setParallelizationMode(Reader::ParallelizationMode mode) {

    m_parallel = mode;

    if (mode == Serial) {
        if (m_concurrency)
            removeParameter(m_concurrency);
    } else {
        if (!m_concurrency) {
            setCurrentParameterGroup("Reader");
            m_concurrency = addIntParameter("concurrency", "number of read operations to keep in flight per MPI rank", 1);
            setParameterRange(m_concurrency, Integer(-1), Integer(std::thread::hardware_concurrency()*5));
            setCurrentParameterGroup();
        }
    }
}

void Reader::setHandlePartitions(bool enable)
{
    m_handlePartitions = enable;
}

void Reader::setAllowTimestepDistribution(bool allow)
{
    m_allowTimestepDistribution = allow;
    if (m_allowTimestepDistribution) {
        if (!m_distributeTime) {
            setCurrentParameterGroup("Reader");
            m_distributeTime = addIntParameter("distribute_time", "distribute timesteps across MPI ranks", 0, Parameter::Boolean);
            setCurrentParameterGroup();
        }
    } else {
        if (m_distributeTime)
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

const Meta &Reader::Token::meta() {

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

bool Reader::Token::addObject(Port *port, Object::ptr obj) {
    std::string name = port->getName();
    return addObject(name, obj);
}

bool Reader::Token::addObject(const std::string &port, Object::ptr obj) {
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
#ifdef DEBUG
    std::cerr << "finishing..." << std::endl;
#endif
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_finished)
            return m_result;

        if (!m_future.valid())
            return false;
    }

    m_future.wait();

    std::lock_guard<std::mutex> locker(m_mutex);
    if (m_finished)
        return m_result;
    if (!m_future.valid())
        return false;

    m_result = m_future.get();
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

void Reader::Token::setPortReady(const std::string &port, bool ready) {

#ifdef DEBUG
    std::cerr << "setting port ready: " << port << "=" << ready << std::endl;
#endif
    std::lock_guard<std::mutex> locker(m_mutex);
    assert(!m_finished);
    assert(m_future.valid());

    auto &p = m_ports[port];
    if (!p) {
        p.reset(new PortState);
    }
    assert(!p->valid);
    p->valid = true;
    p->promise.set_value(ready);
}

void Reader::Token::applyMeta(Object::ptr obj) const {

    if (!obj)
        return;

    obj->setTimestep(m_meta.timeStep());
    obj->setNumTimesteps(m_meta.numTimesteps());
    obj->setBlock(m_meta.block());
    obj->setNumBlocks(m_meta.numBlocks());
}

Reader::Token::Token(Reader *reader, std::shared_ptr<Token> previous)
: m_reader(reader)
, m_previous(previous)
{
}

}
