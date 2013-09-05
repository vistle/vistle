#ifndef STATETRACKER_H
#define STATETRACKER_H

#include <vector>
#include <map>
#include <string>

#include <boost/scoped_ptr.hpp>

#include "export.h"
#include "message.h"

namespace vistle {

class Parameter;
class PortTracker;

class V_COREEXPORT StateObserver {

 public:

   virtual ~StateObserver() {}

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

   std::vector<std::string> getParameters(int id) const;
   Parameter *getParameter(int id, const std::string &name) const;

   bool handleMessage(const message::Message &msg);

   bool handle(const message::Ping &ping);
   bool handle(const message::Pong &pong);
   bool handle(const message::Spawn &spawn);
   bool handle(const message::Started &started);
   bool handle(const message::Connect &connect);
   bool handle(const message::Disconnect &disc);
   bool handle(const message::ModuleExit &moduleExit);
   bool handle(const message::Compute &compute);
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

   PortTracker *portTracker() const;

   std::vector<char> getState() const;

   void registerObserver(StateObserver *observer);

 protected:
   typedef std::map<std::string, Parameter *> ParameterMap;
   struct Module {
      bool initialized = false;
      bool killed = false;
      bool busy = false;
      std::string name;
      ParameterMap parameters;

      int state() const;
      ~Module() {
      }
   };
   typedef std::map<int, Module> RunningMap;
   RunningMap runningMap;
   typedef std::set<int> ModuleSet;
   ModuleSet busySet;

   std::set<StateObserver *> m_observers;

 private:
   PortTracker *m_portTracker;

};

} // namespace vistle

#endif
