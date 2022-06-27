#ifndef VISTLE_DATABASE_H
#define VISTLE_DATABASE_H

#include "index.h"
#include "dimensions.h"
#include "object.h"
#include "archives_config.h"
#include "shm_obj_ref.h"


namespace vistle {

class V_COREEXPORT DataBase: public Object {
    V_OBJECT(DataBase);

public:
    DEFINE_ENUM_WITH_STRING_CONVERSIONS(Mapping, (Unspecified)(Vertex)(Element));

    typedef Object Base;
    virtual void resetArrays(); //< remove reference to data arrays and create empty ones
    virtual Index getSize();
    virtual Index getSize() const;
    virtual void setSize(const size_t size);
    virtual void applyDimensionHint(Object::const_ptr grid);
    Object::const_ptr grid() const;
    void setGrid(Object::const_ptr grid);
    Mapping guessMapping(Object::const_ptr grid = Object::const_ptr())
        const; //< if Unspecified, try to derive a mapping based on array and grid size
    Mapping mapping() const;
    void setMapping(Mapping m);
    virtual void setExact(bool enable); //< whether data arrays may not be compressed lossy

    void copyAttributes(Object::const_ptr src, bool replace = true) override;

    virtual unsigned dimension() const;
    virtual bool copyEntry(Index to, DataBase::const_ptr src, Index from);
    virtual void setValue(Index idx, unsigned component, const double &value);
    virtual double value(Index idx, unsigned component = 0) const;

private:
    V_DATA_BEGIN(DataBase);
    shm_obj_ref<Object> grid;
    Mapping mapping;

    Data(const Data &o, const std::string &name, Type id);
    ~Data();
    static Data *create(Type id = UNKNOWN, const Meta &meta = Meta());
    V_DATA_END(DataBase);
};

V_OBJECT_DECL(DataBase)

//ARCHIVE_ASSUME_ABSTRACT(DataBase)

} // namespace vistle
#endif
