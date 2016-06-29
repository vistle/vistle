//-------------------------------------------------------------------------
// STRUCTURED GRID OBJECT BASE CLASS CPP
// *
// * Base class for Structured Grid Objects
//-------------------------------------------------------------------------

#include "structuredgridbase.h"

namespace vistle {

// CONSTRUCTOR
//-------------------------------------------------------------------------
StructuredGridBase::StructuredGridBase(const Meta &meta) : StructuredGridBase::Base(static_cast<Data *>(NULL)) {

}

// REFRESH IMPL
//-------------------------------------------------------------------------
void StructuredGridBase::refreshImpl() const {

}

// CHECK IMPL
//-------------------------------------------------------------------------
bool StructuredGridBase::checkImpl() const {

   return true;
}

// IS EMPTY
//-------------------------------------------------------------------------
bool StructuredGridBase::isEmpty() const {

   return Base::isEmpty();
}

// DATA OBJECT - CONSTRUCTOR FROM DATA OBJECT AND NAME
//-------------------------------------------------------------------------
StructuredGridBase::Data::Data(const StructuredGridBase::Data &o, const std::string &n)
: StructuredGridBase::Base::Data(o, n) {
}


// DATA OBJECT- CONSTRUCTOR FROM DATA OBJECT, NAME AND TYPE
//-------------------------------------------------------------------------
StructuredGridBase::Data::Data(const StructuredGridBase::Data &o, const std::string &n, Type id)
    : StructuredGridBase::Base::Data(o, n, id) {

}

// DATA OBJECT - DESTRUCTOR
//-------------------------------------------------------------------------
StructuredGridBase::Data::~Data() {

}

// DATA OBJECT - CREATE FUNCTION
//-------------------------------------------------------------------------
StructuredGridBase::Data * StructuredGridBase::Data::create(Type id, const Meta &meta) {

    // required for boost::serialization
    assert("should never be called" == NULL);

    return NULL;
}

// MACROS
//-------------------------------------------------------------------------
//@question do i need this?
//V_SERIALIZERS(StructuredGridBase)
V_OBJECT_CTOR(StructuredGridBase)

} // namespace vistle
