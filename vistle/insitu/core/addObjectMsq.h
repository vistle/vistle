#ifndef ADD_OBJECT_MSQ_H
#define ADD_OBJECT_MSQ_H
#include "export.h"
#include "moduleInfo.h"
#include <core/object.h>
namespace vistle {
namespace message {

class MessageQueue;
}
}
namespace insitu {

class V_INSITUCOREEXPORT AddObjectMsq {
public:
	AddObjectMsq(const ModuleInfo& moduleInfo, size_t rank);
	~AddObjectMsq();
	AddObjectMsq(AddObjectMsq& other) = delete;
	AddObjectMsq(AddObjectMsq&& other) noexcept;
	AddObjectMsq operator=(AddObjectMsq& other) = delete;
	AddObjectMsq &operator=(AddObjectMsq&& other) noexcept;

	void addObject(const std::string& port, vistle::Object::const_ptr obj);

private: 
	vistle::message::MessageQueue* m_sendMessageQueue = nullptr; //Queue to send addObject messages to LibSImController module


};

}

#endif // !ADD_OBJECT_MSQ_H
