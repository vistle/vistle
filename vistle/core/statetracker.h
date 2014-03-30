#ifndef STATETRACKER_H
#define STATETRACKER_H

#include <vector>
#include <map>
#include <set>
#include <string>

#include <boost/scoped_ptr.hpp>

#include "export.h"
#include "message.h"

namespace vistle {

class Parameter;
class PortTracker;

class V_COREEXPORT StateObserver {

 public:

   StateObserver(): m_modificationCount(0) {}
   virtual ~StateObserver() {}

   virtual void moduleAvailable(const std::string &moduleName) = 0;

   virtual void newModule(int moduleId, const boost::uuids::uuid &spawnUuid, const std::string &moduleName) = 0;
   virtual void deleteModule(int moduleId) = 0;

   enum ModuleStateBits {
      Initialized = 1,
      Killed = 2,
      Busy = 4,
   };
   virtual void moduleStateChanged(int moduleId, int stateBits) = 0;

   virtual void newParameter(int moduleId, const std::string &parameterName) = 0;
   virtual void parameterValueChanged(int moduleId, const std::string &parameterName) = 0;
   virtual void parameterChoicesChanged(int moduleId, const std::string &parameterName) = 0;

   virtual void newPort(int moduleId, const std::string &portName) = 0;

   virtual void newConnection(int fromId, const std::string &fromName,
         int toId, const std::string &toName) = 0;

   virtual void deleteConnection(int fromId, const std::string &fromName,
         int toId, const std::string &toName) = 0;

   virtual void info(const std::string &text, message::SendText::TextType textType, int senderId, int senderRank, message::Message::Type refType, const message::Message::uuid_t &refUuid) = 0;

   virtual void quitRequested();

   virtual void resetModificationCount();
   virtual void incModificationCount();
   long modificationCount() const;

private:
   long m_modificationCount;
};

class V_COREEXPORT StateTracker {
   friend class PortTracker;

 public:
   StateTracker(PortTracker *portTracker);
   ~StateTracker();

   bool dispatch(bool &received);

   std::vector<int> getRunningList() const;
   std::vector<int> getBusyList() const;
   std::string getModuleName(int id) const;
   int getModuleState(int id) const;

   std::vector<std::string> getParameters(int id) const;
   Parameter *getParameter(int id, const std::string &name) const;

   bool handleMessage(const message::Message &msg);

   bool handle(const message::Ping &ping);
   bool handle(const message::Pong &pong);
   bool handle(const message::Trace &trace);
   bool handle(const message::Spawn &spawn);
   bool handle(const message::Started &started);
   bool handle(const message::Connect &connect);
   bool handle(const message::Disconnect &disc);
   bool handle(const message::ModuleExit &moduleExit);
   bool handle(const message::Compute &compute);
   bool handle(const message::Reduce &reduce);
   bool handle(const message::ExecutionProgress &prog);
   bool handle(const message::Busy &busy);
   bool handle(const message::Idle &idle);
   bool handle(const message::CreatePort &createPort);
   bool handle(const message::AddParameter &addParam);
   bool handle(const message::SetParameter &setParam);
   bool handle(const message::SetParameterChoices &choices);
   bool handle(const message::Kill &kill);
   bool handle(const message::AddObject &addObj);
   bool handle(const message::ObjectReceived &objRecv);
   bool handle(const message::Barrier &barrier);
   bool handle(const message::BarrierReached &barrierReached);
   bool handle(const message::ResetModuleIds &reset);
   bool handle(const message::ReplayFinished &reset);
   bool handle(const message::SendText &info);
   bool handle(const message::Quit &quit);
   bool handle(const message::ModuleAvailable &mod);

   PortTracker *portTracker() const;

   std::vector<char> getState() const;

   struct AvailableModule {
       std::string name;
   };
   const std::vector<AvailableModule> &availableModules() const;

   void registerObserver(StateObserver *observer);

 protected:
   typedef std::map<std::string, Parameter *> ParameterMap;
   typedef std::map<int, std::string> ParameterOrder;
   struct Module {
      bool initialized;
      bool killed;
      bool busy;
      std::string name;
      ParameterMap parameters;
      ParameterOrder paramOrder;

      int state() const;
      Module(): initialized(false), killed(false), busy(false) {}
   };
   typedef std::map<int, Module> RunningMap;
   RunningMap runningMap;
   typedef std::set<int> ModuleSet;
   ModuleSet busySet;

   std::vector<AvailableModule> m_availableModules;

   std::set<StateObserver *> m_observers;

 private:
   PortTracker *m_portTracker;
};

} // namespace vistle

#endif
