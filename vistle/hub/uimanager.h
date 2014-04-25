#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <map>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

namespace vistle {

namespace message {
class Message;
};

class UiClient;
class StateTracker;
class Hub;

class UiManager {
   friend class UiThreadWrapper;

 public:
   ~UiManager();

   void requestQuit();
   boost::mutex &interpreterMutex();
   bool handleMessage(const message::Message &msg) const;
   void sendMessage(const message::Message &msg) const;
   UiManager(Hub &hub, StateTracker &stateTracker);
   void addClient(boost::shared_ptr<UiClient> c);
   void lockUi(bool lock);

 private:
   void removeThread(boost::thread *thread);
   void sendMessage(boost::shared_ptr<UiClient> c, const message::Message &msg) const;

   void join();
   void disconnect();

   Hub &m_hub;
   StateTracker &m_stateTracker;

   bool m_requestQuit;

   typedef std::map<boost::thread *, boost::shared_ptr<UiClient>> ThreadMap;
   ThreadMap m_threads;
   int m_uiCount;
   bool m_locked;
};

} // namespace vistle

#endif
