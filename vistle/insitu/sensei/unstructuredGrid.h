#ifndef SENSEI_UNSTRUCTURED_GRID_H
#define SENSEI_UNSTRUCTURED_GRID_H

#include "export.h"
#include "gridInterface.h"
#include "structuredGridBase.h"

#include <insitu/core/dataType.h>
#include <insitu/core/array.h>

namespace vistle {
namespace insitu {
namespace sensei {

class V_SENSEIEXPORT UnstructuredMesh : public GridInterface, private vistle::insitu::sensei::StructuredGridBase
{
public:
	// Inherited via GridInterface
	virtual vistle::Object_const_ptr toVistle(size_t timestep, vistle::insitu::message::SyncShmIDs& syncIDs) const override;
		
	//types are void and will be converted with ...ToVistle function;
	Array cl;
	Array tl;
	Array el;
	//functions to convert cl, tl and el to vistle
	bool (*clToVistle)(vistle::Index* vistleTl);
	bool (*tlToVistle)(vistle::Byte* vistleTl);
	bool (*elToVistle)(vistle::Index* vistleTl);

private:
	size_t numVerts() const;

};
}//sensei
}//insitu
}//vistle

#endif			
			
