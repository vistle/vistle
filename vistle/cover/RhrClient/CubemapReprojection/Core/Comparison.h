// Core/Comparison.h

#ifndef _CORE_COMPARISON_H_
#define _CORE_COMPARISON_H_

#include <string>

// For operator ==.
#define BaseEqualCompareBlock(type, otherName) if(!type::operator==(otherName)) { return false; }
#define CStringEqualCompareBlock(name) { int compareResult = strcmp(this->name, other.name); if(compareResult != 0) { return false; } }
#define StringEqualCompareBlock(name) { int compareResult = this->name.compare(other.name); if(compareResult != 0) { return false; } }
#define BoolEqualCompareBlock(name) if(this->name != other.name) { return false; }
#define NumericalEqualCompareBlock(name) if(this->name != other.name) { return false; }
#define MemoryEqualCompareBlock(name) { int compareResult = memcmp(&this->name, &other.name, sizeof(name)); if(compareResult != 0) { return false; } }
#define ArrayEqualCompareBlock(name, size) { if(this->size != other.size) { return false; } int compareResult = memcmp(this->name, other.name, size); if(compareResult != 0) { return false; } }
#define StructureEqualCompareBlock(name) if(this->name != other.name) { return false; }
#define SmartPointerAddressEqualCompareBlock(name) { auto ptr1 = this->name.get(); auto ptr2 = other.name.get(); if(ptr1 != ptr2) { return false; } }
#define PointerEqualCompareBlock(name) { auto ptr1 = this->name; auto ptr2 = other.name; if(ptr1 == nullptr || ptr2 == nullptr) { if(ptr1 != ptr2) { return false; } } else if(*ptr1 != *ptr2) { return false; } }
#define SmartPointerEqualCompareBlock(name) { auto ptr1 = this->name.get(); auto ptr2 = other.name.get(); if(ptr1 == nullptr || ptr2 == nullptr) { if(ptr1 != ptr2) { return false; } } else if(*ptr1 != *ptr2) { return false; } }

// For operator <.
#define BaseLessCompareBlock(type, otherName) if(!type::operator==(otherName)) { return (type::operator<(otherName)); }
#define CStringLessCompareBlock(name) { int compareResult =strcmp(this->name, other.name); if(compareResult != 0) { return compareResult < 0; } }
#define StringLessCompareBlock(name) { int compareResult = this->name.compare(other.name); if(compareResult != 0) { return compareResult < 0; } }
#define BoolLessCompareBlock(name) if(this->name != other.name) { return name; }
#define NumericalLessCompareBlock(name) if(this->name != other.name) { return (this->name < other.name); }
#define MemoryLessCompareBlock(name) { int compareResult = memcmp(&this->name, &other.name, sizeof(name)); if(compareResult != 0) { return compareResult < 0; } }
#define ArrayLessCompareBlock(name, size) { if(this->size != other.size) { return (this->size < other.size); } int compareResult = memcmp(this->name, other.name, size); if(compareResult != 0) { return compareResult < 0; } }
#define StructureLessCompareBlock(name) if(this->name != other.name) { return (this->name < other.name); }
#define SmartPointerAddressLessCompareBlock(name) { auto ptr1 = this->name.get(); auto ptr2 = other.name.get(); if(ptr1 != ptr2) { return (ptr1 < ptr2); } }
#define PointerLessCompareBlock(name) { auto ptr1 = this->name; auto ptr2 = other.name; if(ptr1 == nullptr || ptr2 == nullptr) { if(ptr1 != nullptr && ptr2 != nullptr) { return (ptr1 == nullptr); } } else if(*ptr1 != *ptr2) { return (*ptr1 < *ptr2); } }
#define SmartPointerLessCompareBlock(name) { auto ptr1 = this->name.get(); auto ptr2 = other.name.get(); if(ptr1 == nullptr || ptr2 == nullptr) { if(ptr1 != nullptr && ptr2 != nullptr) { return (ptr1 == nullptr); } } else if(*ptr1 != *ptr2) { return (*ptr1 < *ptr2); } }

#endif