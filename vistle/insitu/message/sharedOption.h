#pragma once
#include <string>
#include <vistle/core/object.h>

namespace vistle {
namespace insitu {
namespace message {

// Creates a serializable type which can be used as InsituMesaage
template<typename Identifier, typename ValueType>
struct SharedOption {
    SharedOption() = default;
    SharedOption(const char *name): m_name(name) {}

    SharedOption(const Identifier &name, ValueType initialValue = ValueType{}, std::function<void()> callback = nullptr)
    : m_name(name), m_val(initialValue), m_callback(callback)
    {}
    void setVal(ValueType val) const
    {
        m_val = val;
        if (m_callback) {
            m_callback();
        }
    }
    const Identifier name() const { return m_name; }

    int value() const { return m_val; }
    ARCHIVE_ACCESS
    template<class Archive>
    void serialize(Archive &ar)
    {
        ar &m_name;
        ar &m_val;
    }
    bool operator==(const SharedOption &other) const { return other.m_name == m_name; }
    bool operator<(const SharedOption &other) const { return other.m_name < m_name; }
    bool operator>(const SharedOption &other) const { return other.m_name > m_name; }

private:
    Identifier m_name;
    mutable ValueType m_val = ValueType{};
    std::function<void()> m_callback = nullptr;
};

template<class Identifier, class ValueType, class Container>
bool update(Container &container, const SharedOption<Identifier, ValueType> &newValue)
{
    auto option = std::find(container.begin(), container.end(), newValue);
    if (option != container.end()) {
        option->setVal(newValue.value());
        return true;
    }
    return false;
}

} // namespace message
} // namespace insitu
} // namespace vistle
