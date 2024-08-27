#ifndef VISTLE_SENSEI_OBJECT_RETREIVER_H
#define VISTLE_SENSEI_OBJECT_RETREIVER_H

#include "export.h"
#include "metaData.h"

#include <vistle/core/database.h>
#include <vistle/core/object.h>

namespace vistle {
namespace insitu {

class V_INSITUADAPTEREXPORT ObjectRetriever { // callbacks to retrieve Grids
    // and Arrays from sensei
public:
    struct V_INSITUADAPTEREXPORT PortAssignedObject {
        PortAssignedObject() = default;
        PortAssignedObject(const std::string &gridName, vistle::Object::const_ptr obj);
        PortAssignedObject(const std::string &gridName, const std::string &varName, vistle::Object::const_ptr obj);
        const std::string &portName() const;
        vistle::Object::const_ptr object() const;
        operator bool() const;

    private:
        std::string m_portName;
        vistle::Object::const_ptr m_obj;
    };
    typedef std::vector<PortAssignedObject> PortAssignedObjectList;
    ObjectRetriever(std::function<PortAssignedObjectList(const MetaData &usedData)> getData);
    PortAssignedObjectList getData(const MetaData &usedData);

private:
    std::function<PortAssignedObjectList(const MetaData &usedData)> m_getData;
};

} // namespace insitu
} // namespace vistle

#endif
