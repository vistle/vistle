#ifndef READVISTLE_H
#define READVISTLE_H

#include <string>
#include <vistle/module/module.h>

class ReadVistle: public vistle::Module {
public:
    ReadVistle(const std::string &shmname, const std::string &name, int moduleID);
    ~ReadVistle();

private:
    bool load(const std::string &name);
    bool prepare() override;
};

#endif
