//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#include "pointeroarchive.h"


// << OPERATOR: STD::STRINGS
//-------------------------------------------------------------------------
PointerOArchive & PointerOArchive::operator<<(const std::string & t) {

    m_currentEntry.size = t.size();

    return * this << t.c_str();
}


