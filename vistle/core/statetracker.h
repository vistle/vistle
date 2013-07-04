#ifndef STATETRACKER_H
#define STATETRACKER_H

#include <vector>
#include <map>
#include <string>

#include <boost/scoped_ptr.hpp>

#include <core/message.h>

namespace vistle {

class Parameter;
class PortTracker;

class StateTracker {

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

 protected:
   typedef std::map<std::string, Parameter *> ParameterMap;
   struct Module {
      bool initialized = false;
      bool killed = false;
      bool busy = false;
      std::string name;
      ParameterMap parameters;

      ~Module() {
      }
   };
   typedef std::map<int, Module> RunningMap;
   RunningMap runningMap;
   typedef std::set<int> ModuleSet;
   ModuleSet busySet;

 private:
   PortTracker *m_portTracker;

};

} // namespace vistle

#endif
