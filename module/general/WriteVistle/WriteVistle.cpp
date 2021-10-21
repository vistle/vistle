#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <vistle/core/archives.h>
#include <vistle/core/object.h>

#include "WriteVistle.h"

namespace ba = boost::archive;
using namespace vistle;

MODULE_MAIN(WriteVistle)

WriteVistle::WriteVistle(const std::string &shmname, const std::string &name, int moduleID)
: Module(shmname, name, moduleID), m_ofs(NULL), m_binAr(NULL), m_textAr(NULL), m_xmlAr(NULL)
{
    createInputPort("grid_in");
    auto fp = addIntParameter("format", "file format", 0, Parameter::Choice);
    std::vector<std::string> choices;
    choices.push_back("binary");
    choices.push_back("ASCII");
    choices.push_back("XML");
    setParameterChoices(fp, choices);
    addStringParameter("filename", "filename of Vistle archive", "vistle.archive", Parameter::Filename);
}

WriteVistle::~WriteVistle()
{
    close();
}

void WriteVistle::close()
{
    delete m_binAr;
    m_binAr = NULL;
    delete m_textAr;
    m_textAr = NULL;
    delete m_xmlAr;
    m_xmlAr = NULL;

    if (m_ofs) {
        *m_ofs << std::endl;
        *m_ofs << "vistle separator" << std::endl;
    }
    delete m_ofs;
    m_ofs = NULL;
}

bool WriteVistle::compute()
{
    int count = 0;
    bool trunc = false;
    bool end = false;
    std::string format;
    Object::const_ptr obj = expect<Object>("grid_in");
    if (!obj)
        return false;

    std::ios_base::openmode flags = std::ios::out;
    if (obj->hasAttribute("_mark_begin")) {
        close();
        trunc = true;
        flags |= std::ios::trunc;
    } else {
        flags |= std::ios::app;
    }
    if (obj->hasAttribute("_mark_end")) {
        end = true;
    }
    switch (getIntParameter("format")) {
    default:
    case 0:
        flags |= std::ios::binary;
        format = "binary";
        break;
    case 1:
        format = "text";
        break;
    case 2:
        format = "xml";
        break;
    }
    if (!m_ofs) {
        m_ofs = new std::ofstream(getStringParameter("filename").c_str(), flags);
    }
    if (trunc)
        *m_ofs << "vistle " << format << " 1 start" << std::endl;

    switch (getIntParameter("format")) {
    default:
    case 0: {
        if (!m_binAr)
            m_binAr = new ba::binary_oarchive(*m_ofs);
        obj->save(*m_binAr);
        break;
    }
    case 1: {
        if (!m_textAr)
            m_textAr = new ba::text_oarchive(*m_ofs);
        obj->save(*m_textAr);
        break;
    }
    case 2: {
        if (!m_xmlAr)
            m_xmlAr = new ba::xml_oarchive(*m_ofs);
        obj->save(*m_xmlAr);
        break;
    }
    }
    ++count;

    close();

    //*m_ofs << std::endl << "vistle separator" << std::endl;

    if (trunc) {
        std::cerr << "saved";
    } else {
        std::cerr << "appended";
    }
    if (end) {
        std::cerr << " final";
        close();
    }
    std::cerr << " [" << count << "] to " << getStringParameter("filename") << " (" << format << ")" << std::endl;

    return true;
}
