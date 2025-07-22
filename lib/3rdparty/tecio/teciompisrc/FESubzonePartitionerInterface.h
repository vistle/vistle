 #pragma once
#include "basicTypes.h"
#include "ItemAddress.h"
namespace tecplot { namespace ___3931 { class FESubzonePartitionerInterface { public: virtual ~FESubzonePartitionerInterface() {} virtual ___81                     numIndexes() const = 0; virtual ItemAddress64::SubzoneOffset_t numSubzones() const = 0; virtual ItemAddress64::ItemOffset_t    numItemsInSubzone(ItemAddress64::SubzoneOffset_t ___3878) const = 0; virtual ___81                     baseIndex() const = 0; virtual ___81                     indexAtItemAddress(ItemAddress64 const& ___2087) const = 0; virtual ItemAddress64 const&           itemAddressAtIndex(___81 index) const = 0; virtual void                           setItemAddressAtIndex(___81 index, ItemAddress64 const& ___2087) = 0; }; }}
