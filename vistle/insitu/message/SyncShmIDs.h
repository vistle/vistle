#ifndef INSITU_SYNC_SHM_IDS_H
#define INSITU_SYNC_SHM_IDS_H
#include "InSituMessage.h"
#include "export.h"

namespace vistle {
namespace insitu {
namespace message {

    class V_INSITUMESSAGEEXPORT SyncShmIDs {
    public:
        enum class Mode {
            Create
            , Attach
        };
#ifdef MODULE_THREAD
        typedef std::lock_guard<std::mutex> Guard;
#else
        typedef boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> Guard;
#endif

        void initialize(int moduleID, int rank, int instance, Mode mode);
        void close();
        bool isInitialized();

        //we might need to use a mutex for this?
        void set(int objID, int arrayID);
        int objectID();
        int arrayID();
        template<typename T, typename...Args>
        typename T::ptr createVistleObject(Args&&... args) {
            auto obj = typename T::ptr(new T(args...));
            set(vistle::Shm::the().objectID(), vistle::Shm::the().arrayID());
            return obj;
        }
        struct ShmData {
            int objID = -1;
            int arrayID = -1;
#ifdef MODULE_THREAD
            std::mutex mutex;
#else
            boost::interprocess::interprocess_mutex mutex;
#endif

        };
    private:
#ifndef MODULE_THREAD
        class ShmSegment {
        public:
            ShmSegment() {}
            ShmSegment(ShmSegment&& other);
            ShmSegment(const std::string& name, Mode mode);
            ~ShmSegment();
            const ShmData* data() const;
            ShmData* data();
            ShmSegment& operator=(ShmSegment&& other) noexcept;
        private:
            std::string m_name;

            boost::interprocess::mapped_region m_region;
        };
        ShmSegment m_segment;
#else
        class Data { //wrapper if we dont need shm
        public:
            ShmData* data() {
                return &m_data;
            }
        private:
            ShmData m_data;
        };
        Data m_segment;
#endif
        int m_rank = -1;
        int m_moduleID = -1;

        bool m_initialized = false;
    };

} //message
} //insitu
}//vistle



#endif // !INSITU_SYNC_SHM_IDS_H
