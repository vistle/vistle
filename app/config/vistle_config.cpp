#include <vistle/util/hostname.h>
#include <vistle/util/directory.h>
#include <vistle/config/access.h>
#include <vistle/config/file.h>
#include <vistle/config/value.h>
#include <vistle/config/array.h>

#include <iostream>

int main(int argc, char *argv[])
{
    std::string hostname = vistle::hostname();
    auto dir = std::make_unique<vistle::Directory>(argc, argv);

    auto config = vistle::config::Access(hostname, hostname);
    config.setPrefix(dir->prefix());

    if (argc < 5 || argc > 6) {
        std::cerr << "usage: vistle_config type file section entry [default]" << std::endl;
        return 1;
    }

    std::string type = argv[1];
    std::string file = argv[2];
    std::string section = argv[3];
    std::string entry = argv[4];
    std::string def, ndef = "0";
    bool haveDefault = false;
    if (argc > 5) {
        haveDefault = true;
        def = argv[5];
        ndef = def;
    }

    if (type == "b" || type == "bool" || type == "boolean") {
        auto val = config.value<bool>(file, section, entry, std::stoi(ndef) ? true : false);
        if (val->exists() || haveDefault)
            std::cout << val->value() << std::endl;
    } else if (type == "i" || type == "int" || type == "integer") {
        auto val = config.value<int64_t>(file, section, entry, std::stoi(ndef));
        if (val->exists() || haveDefault)
            std::cout << val->value() << std::endl;
    } else if (type == "f" || type == "float" || type == "d" || type == "double") {
        auto val = config.value<double>(file, section, entry, std::stod(ndef));
        if (val->exists() || haveDefault)
            std::cout << val->value() << std::endl;
    } else if (type == "s" || type == "string") {
        auto val = config.value<std::string>(file, section, entry, def);
        if (val->exists() || haveDefault)
            std::cout << val->value() << std::endl;
    } else if (type == "ba" || type == "bool_array") {
        auto arr = config.array<bool>(file, section, entry);
        if (arr->exists()) {
            auto vec = arr->value();
            for (auto v: vec) {
                std::cout << v << std::endl;
            }
        }
    } else if (type == "ia" || type == "int_array") {
        auto arr = config.array<int64_t>(file, section, entry);
        if (arr->exists()) {
            auto vec = arr->value();
            for (auto v: vec) {
                std::cout << v << std::endl;
            }
        }
    } else if (type == "fa" || type == "float_array" || type == "da" || type == "double_array") {
        auto arr = config.array<int64_t>(file, section, entry);
        if (arr->exists()) {
            auto vec = arr->value();
            for (auto v: vec) {
                std::cout << v << std::endl;
            }
        }
    } else if (type == "sa" || type == "string_array") {
        auto arr = config.array<std::string>(file, section, entry);
        if (arr->exists()) {
            auto vec = arr->value();
            for (auto v: vec) {
                std::cout << v << std::endl;
            }
        }
    } else {
        std::cerr << "unknown type \"" << type
                  << "\", supported are b[ool], i[nt], d[ouble], and s[tring] and arrays thereof (e.g. ia or int_array)"
                  << std::endl;
        return 1;
    }

    return 0;
}
