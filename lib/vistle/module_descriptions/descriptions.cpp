#include "descriptions.h"
#include <iostream>
#include <sstream>
#include <cmrc/cmrc.hpp>

#ifndef MODULE_STATIC
#include <fstream>
#include <vistle/util/filesystem.h>
#endif

CMRC_DECLARE(moduledescriptions);

namespace vistle {

namespace {
std::vector<std::string> readModuleCategories(std::istream &str)
{
    std::vector<std::string> categories;

    std::string line;
    while (std::getline(str, line)) {
        auto sep = line.find(' ');
        auto cat = line.substr(0, sep);
        categories.push_back(cat);
    }
    return categories;
}

std::map<std::string, std::string> readCategoryDescriptions(std::istream &str)
{
    std::map<std::string, std::string> categories;

    std::string line;
    while (std::getline(str, line)) {
        auto sep = line.find(' ');
        auto cat = line.substr(0, sep);
        line = line.substr(sep + 1);
        categories[cat] = line;
    }
    return categories;
}
} // namespace

std::vector<std::string> getModuleCategories(const std::string &share_prefix)
{
    static const std::string filename = CATFILE;
    std::vector<std::string> categories;
    try {
        auto fs = cmrc::moduledescriptions::get_filesystem();
        auto data = fs.open(filename);
        std::string desc(data.begin(), data.end());
        std::stringstream str(desc);
        categories = readModuleCategories(str);
    } catch (std::exception &ex) {
        std::cerr << "getModuleCategories: exception: " << ex.what() << std::endl;
    }
    return categories;
}

std::map<std::string, std::string> getCategoryDescriptions(const std::string &share_prefix)
{
    static const std::string filename = CATFILE;
    std::map<std::string, std::string> categories;
    try {
        auto fs = cmrc::moduledescriptions::get_filesystem();
        auto data = fs.open(filename);
        std::string desc(data.begin(), data.end());
        std::stringstream str(desc);
        categories = readCategoryDescriptions(str);
    } catch (std::exception &ex) {
        std::cerr << "getCategoryDescriptions: exception: " << ex.what() << std::endl;
    }
    return categories;
}

namespace {
std::map<std::string, ModuleDescription> readModuleDescriptions(std::istream &str)
{
    std::map<std::string, ModuleDescription> moduleDescriptions;

    std::string line;
    while (std::getline(str, line)) {
        auto sep = line.find(' ');
        auto mod = line.substr(0, sep);
        line = line.substr(sep + 1);
        sep = line.find(' ');
        auto cat = line.substr(0, sep);
        auto desc = line.substr(sep + 1);
        moduleDescriptions[mod].category = cat;
        moduleDescriptions[mod].description = desc;
        //std::cerr << "module: " << mod << " -> " << cat << ", description: " << cat << std::endl;
    }
    return moduleDescriptions;
}
} // namespace

std::map<std::string, ModuleDescription> getModuleDescriptions(const std::string &share_prefix)
{
    static const std::string filename = MODFILE;
    std::map<std::string, ModuleDescription> moduleDescriptions;
#ifdef MODULE_STATIC
    try {
        auto fs = cmrc::moduledescriptions::get_filesystem();
        auto data = fs.open(filename);
        std::string desc(data.begin(), data.end());
        std::stringstream str(desc);
        moduleDescriptions = readModuleDescriptions(str);
    } catch (std::exception &ex) {
        std::cerr << "getModuleDescriptions: exception: " << ex.what() << std::endl;
    }
#else
    filesystem::path pshare(share_prefix);
    try {
        if (!filesystem::is_directory(pshare)) {
            std::cerr << "getModuleDescriptions: " << share_prefix << " is not a directory" << std::endl;
        } else {
        }
    } catch (const filesystem::filesystem_error &e) {
        std::cerr << "getModuleDescriptions: error in" << share_prefix << ": " << e.what() << std::endl;
    }
    std::string file = share_prefix + filename;
    std::fstream str(file, std::ios_base::in);
    if (str.is_open()) {
        moduleDescriptions = readModuleDescriptions(str);
    } else {
        std::cerr << "getModuleDescriptions: failed to open module description file " << file << std::endl;
    }
#endif
    return moduleDescriptions;
}


} // namespace vistle
