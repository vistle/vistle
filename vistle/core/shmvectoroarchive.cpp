//-------------------------------------------------------------------------
// VISTLE OBJECT OARCHIVE CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#include "shmvectoroarchive.h"

// GET VECTOR ENTRY BY NAME
// * often multiple consecutive searches for the same enrty will occur, therefore the hint aids performance
//-------------------------------------------------------------------------
ShmVectorOArchive::ArrayData * ShmVectorOArchive::getVectorEntryByNvpName(std::string name) {
    if (m_arrayDataVector[m_indexHint].nvpName == name) {
        return &m_arrayDataVector[m_indexHint];
    } else {
        // linear search
        for (m_indexHint = 0; m_indexHint < m_arrayDataVector.size(); m_indexHint++) {
            if (m_arrayDataVector[m_indexHint].nvpName == name) {
                return &m_arrayDataVector[m_indexHint];
            }
        }

        // not found
        m_indexHint = 0;
        return nullptr;
    }
}
