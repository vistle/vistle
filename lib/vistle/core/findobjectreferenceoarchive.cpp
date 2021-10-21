//-------------------------------------------------------------------------
// FIND OBJECT REFERENCE OARCHIVE CPP
// *
// *
// * Sever Topan, 2016
//-------------------------------------------------------------------------

#include "findobjectreferenceoarchive.h"

#ifdef USE_INTROSPECTION_ARCHIVE

namespace vistle {

//-------------------------------------------------------------------------
// STATIC CONST MEMBER OUT OF CLASS DEFINITIONS
//-------------------------------------------------------------------------
const std::string FindObjectReferenceOArchive::nullObjectReferenceName = "--NULL--";

//-------------------------------------------------------------------------
// METHOD DEFINITIONS
//-------------------------------------------------------------------------

// GET VECTOR ENTRY BY NAME
// * often multiple consecutive searches for the same entry will occur, therefore the hint aids performance
//-------------------------------------------------------------------------
FindObjectReferenceOArchive::ReferenceData *FindObjectReferenceOArchive::getVectorEntryByNvpName(std::string name)
{
    if (m_referenceDataVector[m_indexHint].nvpName == name) {
        return &m_referenceDataVector[m_indexHint];
    } else {
        // linear search
        for (m_indexHint = 0; m_indexHint < m_referenceDataVector.size(); m_indexHint++) {
            if (m_referenceDataVector[m_indexHint].nvpName == name) {
                return &m_referenceDataVector[m_indexHint];
            }
        }

        // not found
        m_indexHint = 0;
        return nullptr;
    }
}

} // namespace vistle

#endif
