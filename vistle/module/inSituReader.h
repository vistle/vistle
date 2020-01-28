#ifndef INSITU_READER_H
#define INSITU_READER_H

#include "module.h"
#include "export.h"
namespace vistle {

class V_MODULEEXPORT InSituReader : public Module {
public:
    InSituReader(const std::string& description, const std::string& name, const int moduleID, mpi::communicator comm);
    bool isExecuting();

    void observeParameter(const Parameter* param);

    virtual bool examine(const Parameter* param = nullptr);
private:

    virtual bool changeParameters(std::set<const Parameter*> params) override;
    virtual bool changeParameter(const Parameter* param) override;
    virtual bool handleExecute(const message::Execute* exec) override;
    virtual void cancelExecuteMessageReceived(const message::Message* msg) override;



    bool m_isExecuting = false;
    const vistle::message::Execute* m_exec;
    std::set<const Parameter*> m_observedParameters;


};


}



#endif