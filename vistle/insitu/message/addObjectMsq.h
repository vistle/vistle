#ifndef ADD_OBJECT_MSQ_H
#define ADD_OBJECT_MSQ_H
#include "export.h"
#include <vistle/insitu/core/moduleInfo.h>
#include <vistle/core/object.h>

namespace vistle {
namespace message {
class MessageQueue;
}
namespace insitu {
namespace message {
	class V_INSITUMESSAGEEXPORT AddObjectMsq {
	public:
		AddObjectMsq(const vistle::insitu::ModuleInfo& moduleInfo, size_t rank);
		~AddObjectMsq();
		AddObjectMsq(AddObjectMsq& other) = delete;
		AddObjectMsq(AddObjectMsq&& other) noexcept;
		AddObjectMsq operator=(AddObjectMsq& other) = delete;
		AddObjectMsq& operator=(AddObjectMsq&& other) noexcept;

		void addObject(const std::string& port, vistle::Object::const_ptr obj);

	private:
		vistle::message::MessageQueue* m_sendMessageQueue = nullptr; //Queue to send addObject messages to LibSImController module
		vistle::insitu::ModuleInfo m_moduleInfo;
		size_t m_rank = 0;

	};
}//message
} //insitu
} //vistle
#endif // !ADD_OBJECT_MSQ_H
