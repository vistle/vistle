#include <OpenConfig/access.h>
#include "COVER.h"
#include <vistle/core/parameter.h>
#include <string>
#include <map>

class CoverConfigBridge: public opencover::config::Bridge {
public:
    CoverConfigBridge(COVER *cover);

    bool wasChanged(const opencover::config::ConfigBase *entry) override;
    bool changeParameter(const vistle::Parameter *param);

    struct Key {
        Key();
        Key(const opencover::config::ConfigBase *entry);

        std::string path;
        std::string section;
        std::string name;

        bool operator<(const Key &o) const;
        std::string paramName() const;
    };

private:
    template<class V>
    bool trySetParam(const opencover::config::ConfigBase *entry, const Key &key, vistle::Parameter *param);
    template<class V>
    bool trySetValueOrArray(const Key &key, const vistle::Parameter *param);

    COVER *m_cover = nullptr;
    std::map<Key, vistle::Parameter *> m_params;
    std::map<std::string, Key> m_values;
};
