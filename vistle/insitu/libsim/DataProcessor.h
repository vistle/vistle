#ifndef VISTLE_LIBSIM_DATA_PROCESSOR_H
#define VISTLE_LIBSIM_DATA_PROCESSOR_H
namespace vistle {
namespace insitu {
namespace message {
class SyncShmIDs;
}
namespace libsim {

class DataProcessor {

public:
	DataProcessor(message::SyncShmIDs* creator);

private:
	message::SyncShmIDs* m_creator;

};

}
}
}


#endif // !VISTLE_LIBSIM_DATA_PROCESSOR_H
