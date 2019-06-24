#ifndef VISTLE_READER_H
#define VISTLE_READER_H

#include "module.h"
#include <set>
#include <future>

namespace vistle {

class V_MODULEEXPORT Reader: public Module {
    friend class Token;

public:

   class V_MODULEEXPORT Token {
       friend class vistle::Reader;
       friend V_MODULEEXPORT std::ostream &operator<<(std::ostream &os, const Token &tok);

   public:
       Token(Reader *reader, std::shared_ptr<Token> previous);
       Reader *reader() const;
       const Meta &meta() const;
       bool wait(const std::string &port = std::string());
       bool addObject(const std::string &port, Object::ptr obj);
       bool addObject(Port *port, Object::ptr obj);
       void applyMeta(vistle::Object::ptr obj) const;
       unsigned long id() const;

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

       struct PortState {
           PortState()
           : future(promise.get_future().share())
           {}
           bool valid = false;
           std::promise<bool> promise;
           std::shared_future<bool> future;
           bool ready = false;
       };
       std::map<std::string, std::shared_ptr<PortState>> m_ports;
   };

   Reader(const std::string &description,
          const std::string &name, const int moduleID, mpi::communicator comm);
   ~Reader() override;
   bool prepare() override;
   bool compute() override;

   virtual bool examine(const Parameter *param);
   virtual bool read(Token &token, int timestep=-1, int block=-1) = 0;
   virtual bool prepareRead();
   virtual bool finishRead();

   int timeIncrement() const;

protected:
   protected:
   enum ParallelizationMode {
       Serial, //< only one operation at a time, all blocks of a timestep first, then other timesteps
       ParallelizeTimesteps, //< up to 'concurrency' operations at a time, but the same block from different timesteps may be scheduled on other ranks
       ParallelizeTimeAndBlocks, //< up to 'concurrency' operations at a time
       ParallelizeBlocks, //< up to 'concurrency' operations at a time, all operations for one timestep have finished before operations for another timestep are started
   };

   void setParallelizationMode(ParallelizationMode mode);
   void setHandlePartitions(bool enable);
   void setAllowTimestepDistribution(bool allow);

   //! whenever an observed parameter changes, data set should be rescanned
   void observeParameter(const Parameter *param);
   void setTimesteps(int number);
   void setPartitions(int number);

   bool changeParameter(const Parameter *param) override;
   void prepareQuit() override;

   bool checkConvexity() const;

   virtual int rankForTimestepAndPartition(int t, int p) const;

   IntParameter *m_first = nullptr;
   IntParameter *m_last = nullptr;
   IntParameter *m_increment = nullptr;
   IntParameter *m_distributeTime = nullptr;
   IntParameter *m_firstRank = nullptr;
   IntParameter *m_checkConvexity = nullptr;

private:
   ParallelizationMode m_parallel = Serial;
   std::mutex m_mutex; // protect ports and message queues
   std::deque<std::shared_ptr<Token>> m_tokens;
   size_t waitForReaders(size_t maxRunning, bool &result);

   std::set<const Parameter *> m_observedParameters;

   int m_numTimesteps = 0;
   int m_numPartitions = 0;
   bool m_readyForRead = true;

   bool m_handlePartitions = true;
   bool m_allowTimestepDistribution = false;

   unsigned long m_tokenCount = 0;

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
}
#endif
