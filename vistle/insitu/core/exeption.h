#ifndef INSITU_EXEPTION
#define INSITU_EXEPTION

#include <string>
#include <exception>

#include "export.h"
namespace vistle {
namespace insitu {
    //base class that lets us catch all insitu exeptions
    class V_INSITUCOREEXPORT InsituExeption :public std::exception {
    public:
        InsituExeption();

        InsituExeption& operator<< (const std::string& msg);
        InsituExeption& operator<< (int msg);
        virtual ~InsituExeption() = default;

    protected:
        mutable std::string m_msg;
        int rank = -1, size = 0;
    };
}//insitu
}//vistle


#endif // !SIMV2_EXEPTION
