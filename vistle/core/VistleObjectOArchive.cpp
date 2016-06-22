//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#include "VistleObjectOArchive.h"


// << OPERATOR: STD::STRINGS
//-------------------------------------------------------------------------
VistleObjectOArchive & VistleObjectOArchive::operator<<(const std::string & t) {

    m_currentSize = t.size();

    return * this << t.c_str();
}


