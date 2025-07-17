#ifndef VISTLE_MODULE_READER_H
#define VISTLE_MODULE_READER_H

#include "module.h"
#include <set>
#include <future>

namespace vistle {

/**
 \class Reader

 \brief base class for Vistle read modules

 Derive from Reader, if you want to implement a module @ref Module for reading data from files.
 You should reimplement the methods
 - examine @ref Reader::examine
 - prepareRead @ref Reader::prepareRead
 - read @ref Reader::read
 - finishRead @ref Reader::finishRead
 */
class V_MODULEEXPORT Reader: public Module {
    friend class Token;

public:
    static inline const std::string InvalidChoice{"(NONE)"};

    class V_MODULEEXPORT Token {
        friend class vistle::Reader;
        friend V_MODULEEXPORT std::ostream &operator<<(std::ostream &os, const Token &tok);

    public:
        /// an instance of this class is handed out with every read @ref Reader::read call
        Token(Reader *reader, std::shared_ptr<Token> previous);
        Reader *reader() const;
        const Meta &meta() const;
        bool wait(const std::string &port = std::string());
        bool addObject(const std::string &port, Object::ptr obj);
        bool addObject(Port *port, Object::ptr obj);
        void applyMeta(vistle::Object::ptr obj) const;
        unsigned long id() const;
        mpi::communicator *comm() const;

    private:
        bool result();
        bool waitDone();
        bool waitPortReady(const std::string &port);
        void setPortReady(const std::string &port, bool ready);

        Reader *m_reader = nullptr;
        Meta m_meta;
        std::mutex m_mutex;
        bool m_finished = false;
        bool m_result = false;
        std::shared_ptr<Token> m_previous;
        std::shared_future<bool> m_future;
        unsigned long m_id = 0;
        std::shared_ptr<mpi::communicator> m_comm;

        struct PortState {
            PortState(): future(promise.get_future().share()) {}
            bool valid = false;
            std::promise<bool> promise;
            std::shared_future<bool> future;
            bool ready = false;
        };
        std::map<std::string, std::shared_ptr<PortState>> m_ports;
    };

    /// construct a read module, parameters correspond to @ref Module constructor
    /** construct a read module, parameters correspond to @ref Module constructor
    *  @param name name of the module in the workflow editor
    *  @param moduleID unique identifier of the module instance
    *  @param Boost.MPI communicator
    */
    Reader(const std::string &name, const int moduleID, mpi::communicator comm);
    ~Reader() override;
    /// called whenever an observed parameter (cf. @ref Reader::observeParameter) has been changed
    /** called whenever an observed parameter (cf. @ref Reader::observeParameter) has been changed.
    *  After constructing the class, the method is called after all parameters have been set to their initial value with param==nullptr.
    *  Otherwise, the changed parameter is passed.
    *  Call @ref setTimesteps and @ref setPartitions according to the data to be read.
    */
    virtual bool examine(const Parameter *param = nullptr);

    /// called for every unit of work to be read
    /** Called for every unit of work to be read.
    *  The size of a work unit depends on the partitioning that has been requested by @ref setPartitions
    */
    virtual bool read(Token &token, int timestep = -1, int block = -1) = 0;

    /// called once on every rank after execution of the module has been initiated before read is called
    virtual bool prepareRead();
    /// called once on every rank before any read call for this timestep is made
    virtual bool prepareTimestep(int timestep);
    /// called once on every rank after all read calls have been made and before execution finishes
    virtual bool finishRead();

    /// return number of timesteps to advance
    int timeIncrement() const;
    /// compute rank on which partition p of timestep t should be read
    virtual int rankForTimestepAndPartition(int t, int p) const;

    //! query into how many parts the data should be split
    int numPartitions() const;

protected:
    void initDone() override;
    Parameter *addParameterGeneric(const std::string &name, std::shared_ptr<Parameter> parameter) override;
    bool removeParameter(const std::string &name) override;

    enum ParallelizationMode {
        Serial, ///< only one operation at a time, all blocks of a timestep first, then other timesteps
        ParallelizeBlocks, ///< up to 'concurrency' operations at a time, all operations for one timestep have finished before operations for another timestep are started
        ParallelizeTimeAndBlocksAfterStatic, ///< up to 'concurrency' operations at a time, but finish reading of static data first
        ParallelizeTimeAndBlocks, ///< up to 'concurrency' operations at a time
    };

    enum PartitionHandling {
        Monolithic, ///< call into read on all ranks and let module handle partitioning
        PartitionTimesteps, ///< call into read on all ranks for constant data, and onto dedicated ranks for partitions of timesteps
        Partition, ///< call into read once per partition and timestep (including "constant timestep")
    };

    enum CollectiveIo {
        Individual, ///< never provide a common communicator for read calls
        CollectiveConstant, ///< provide a common communicator in Token passed to read for constant timestep
        Collective, ///< always provide a communicator spanning all participating ranks (timesteps and static data)
    };

    struct ReaderTime {
        ReaderTime(int first, int last, int inc): m_first(first), m_last(last), m_inc(inc) {}

        int first() const { return m_first; }
        int last() const { return m_last; }
        int inc() const { return m_inc; }
        int calc_numtime() const;

    private:
        int m_first;
        int m_last;
        int m_inc;
    };

    /// control whether and how @ref read invocations are called in parallel
    void setParallelizationMode(ParallelizationMode mode);
    /// control if read operations have to be called collectively
    /*! if read operations are to be called collectively, @ref Token
        will provide an MPI communicator */
    void setCollectiveIo(CollectiveIo collective);

    /// whether partitions should be handled by the @ref Reader class
    void setHandlePartitions(PartitionHandling part);
    /// whether timesteps may be distributed to different ranks
    void setAllowTimestepDistribution(bool allow);
    //! whenever an observed parameter changes, data set should be rescanned
    void observeParameter(const Parameter *param);
    //! call during @ref examine to inform module how many timesteps are present whithin dataset
    void setTimesteps(int number);
    //! call during @ref examine to inform module nto how many the dataset will be split
    void setPartitions(int number);

    bool changeParameters(std::map<std::string, const Parameter *> params) override;
    bool changeParameter(const Parameter *param) override;
    void prepareQuit() override;

    void linkPortAndParameter(Port *port, Parameter *param, std::function<bool(const Parameter *)> check = nullptr);

    struct PortUpdater {
        explicit PortUpdater(const Port *port, std::function<bool(const Parameter *)> check = nullptr)
        : port(port), check(check){};
        const Port *port = nullptr;
        std::function<bool(const Parameter *)> check = nullptr;
    };
    std::map<const Parameter *, std::vector<PortUpdater>> m_portUpdaters;

    IntParameter *m_first = nullptr;
    IntParameter *m_last = nullptr;
    IntParameter *m_increment = nullptr;
    IntParameter *m_distributeTime = nullptr;
    IntParameter *m_firstRank = nullptr;

private:
    struct ReaderProperties {
        ReaderProperties(const Meta *m, ReaderTime rtime, int nPart, int conc)
        : meta(m), time(rtime), numpart(nPart), concurrency(conc)
        {}

    public:
        const Meta *meta;
        ReaderTime time;
        int numpart;
        int concurrency;
    };

    bool readTimestep(std::shared_ptr<Token> &prev, const ReaderProperties &prop, int timestep, int step);
    bool readTimesteps(std::shared_ptr<Token> &prev, const ReaderProperties &prop);
    bool prepare() override;
    bool compute() override;

    ParallelizationMode m_parallel = Serial;
    std::mutex m_mutex; // protect ports and message queues
    std::deque<std::shared_ptr<Token>> m_tokens;
    size_t waitForReaders(size_t maxRunning, bool &result);

    std::set<const Parameter *> m_observedParameters;

    std::vector<int> m_minDomain;
    std::vector<int> m_maxDomain;
    int m_numTimesteps = 0;
    int m_dimDomain = 3;
    int m_numPartitions = 0;
    bool m_readyForRead = true;
    bool m_inhibitExamine = false; // in order to avoid multiple calls to examine() during changeParameters

    CollectiveIo m_collectiveIo = Individual;
    PartitionHandling m_handlePartitions = Partition;
    bool m_handleOwnDIYBlocks = false;
    bool m_allowTimestepDistribution = false;

    unsigned long m_tokenCount = 0;
    std::shared_ptr<StringParameter> m_firstFileBrowser;

    /*
    * # files (api)
    * file selection (ui)
    * # data ports/fields (api)
    * field -> port mapping (ui)
    * bool: directory/file (api)
    * timestep control (ui)
    * distribution over ranks (ui)
    * ? bool: can reorder (api)
    * ? extensions? (api)
    * ? bool: field name from file (api)
    * ? bool: create ghostcells (ui)
    */
};

V_MODULEEXPORT std::ostream &operator<<(std::ostream &os, const Reader::Token &tok);
} // namespace vistle
#endif
