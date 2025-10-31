#include "TestDynamicPorts.h"

MODULE_MAIN(TestDynamicPorts)

using namespace vistle;

TestDynamicPorts::TestDynamicPorts(const std::string &name, int moduleID, mpi::communicator comm)
: Module(name, moduleID, comm), m_numPorts(0), m_numPortsParam(nullptr)
{
    m_numPortsParam = addIntParameter("num_ports", "number of output ports", m_numPorts);
    setParameterRange(m_numPortsParam, (Integer)0, (Integer)10);
}

std::string portName(int num)
{
    std::stringstream str;
    str << "data_out" << num;
    return str.str();
}

bool TestDynamicPorts::changeParameter(const Parameter *param)
{
    if (param == m_numPortsParam) {
        int numPorts = m_numPortsParam->getValue();
        for (int i = m_numPorts; i < numPorts; ++i) {
            createOutputPort(portName(i), "test port");
        }
        for (int i = numPorts; i < m_numPorts; ++i) {
            destroyPort(portName(i));
        }
        m_numPorts = numPorts;
    }
    return true;
}

bool TestDynamicPorts::compute()
{
    return true;
}
