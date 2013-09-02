#ifndef VISTLECONNECTION_H
#define VISTLECONNECTION_H

#include <string>
#include <boost/thread/recursive_mutex.hpp>
#include <userinterface/userinterface.h>

namespace vistle {

class VistleConnection
{
   friend class PythonModule;
public:
   VistleConnection(vistle::UserInterface &ui);
   static VistleConnection &the();

   void cancel();
   void operator()();

   void sendMessage(const vistle::message::Message &msg) const;
   vistle::Parameter *getParameter(int id, const std::string &name) const;
   template<class T>
   void setParameter(int id, const std::string &name, const T &value) const;
   bool requestReplyAsync(const vistle::message::Message &send);
   bool waitForReplyAsync(const vistle::message::Message::uuid_t &uuid, vistle::message::Message &reply);
   bool waitForReply(const vistle::message::Message &send, vistle::message::Message &reply);

   vistle::UserInterface &ui() const;

private:
   vistle::UserInterface &m_ui;
   bool m_done = false;

public:
   typedef boost::recursive_mutex mutex;
   typedef mutex::scoped_lock mutex_lock;
   mutable mutex m_mutex;

private:
   static VistleConnection *s_instance;
};

template<class T>
void VistleConnection::setParameter(int id, const std::string &name, const T &value) const
{
   mutex_lock lock(m_mutex);
   vistle::ParameterBase<T> *p = dynamic_cast<vistle::ParameterBase<T> *>(getParameter(id, name));
   if (!p) {
      std::cerr << "parameter not of appropriate type: " << id << ":" << name << std::endl;
      return;
   }

   p->setValue(value);
   vistle::message::SetParameter set(id, name, p);
   sendMessage(set);
}

} //namespace vistle
#endif
