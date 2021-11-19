#ifndef INSITU_EXCEPTION
#define INSITU_EXCEPTION

#include "export.h"

#include <vistle/util/exception.h>

#include <string>

namespace vistle {
namespace insitu {
// base class that lets us catch all insitu exceptions
class V_INSITUCOREEXPORT InsituException: public vistle::exception {
public:
    InsituException();

    InsituException &operator<<(const std::string &msg);
    InsituException &operator<<(int msg);
    virtual ~InsituException() = default;
    virtual const char *what() const throw() override;

protected:
    mutable std::string m_msg;
    int rank = -1, size = 0;
};
} // namespace insitu
} // namespace vistle

#endif // !SIMV2_EXCEPTION
