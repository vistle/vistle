 #pragma once
#include "ClassMacros.h"
#include "ItemAddress.h"
#include "SzlFileLoader.h"
namespace tecplot { namespace ___3931 { class PartitionMetadata { UNCOPYABLE_CLASS(PartitionMetadata); public: FileLoc2DArray m_secCszConnectivityFileLocs; ___1390   m_nszConnectivityFileLocs; ItemAddress64::___2978 m_numRefPartitions; PartitionArray             m_refPartitions; ___2238<RefSubzoneOffsetArray> m_secCszNumRefNszs; RefSubzoneOffsetArray                    m_nszNumRefCszs; ___2238<UInt8Array> m_secCszIncludesPtn; UInt8Array                    m_nszIncludesPtn; ___1390 m_nszMinMaxFileLocs; ___1390 m_cszMinMaxFileLocs; ___1390 m_szDataStartFileLocs; public: PartitionMetadata() {} }; }}
