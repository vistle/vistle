#include "objectRetriever.h"

using namespace vistle::insitu;

ObjectRetriever::ObjectRetriever(std::function<PortAssignedObjectList(const MetaData &usedData)> getData)
: m_getData(getData)
{}

std::vector<ObjectRetriever::PortAssignedObject> ObjectRetriever::getData(const MetaData &usedData)
{
    return m_getData(usedData);
}

ObjectRetriever::PortAssignedObject::PortAssignedObject(const std::string &gridName, vistle::Object::const_ptr obj)
: m_portName(gridName), m_obj(obj)
{}

ObjectRetriever::PortAssignedObject::PortAssignedObject(const std::string &gridName, const std::string &varName,
                                                        vistle::Object::const_ptr obj)
: m_portName(gridName + "_" + varName), m_obj(obj)
{}

const std::string &ObjectRetriever::PortAssignedObject::portName() const
{
    return m_portName;
}

vistle::Object::const_ptr ObjectRetriever::PortAssignedObject::object() const
{
    return m_obj;
}

ObjectRetriever::PortAssignedObject::operator bool() const
{
    return m_obj != nullptr;
}
