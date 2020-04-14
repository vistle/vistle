#ifndef VISTLE_SENSEI_H
#define VISTLE_SENSEI_H

#include "export.h"
namespace insitu {
namespace sensei{
	class DataAdapter;
	
	class V_SENSEIEXPORT SenseiAdapter
	{
	public:
		void Initialize();
		bool Execute(DataAdapter* dataAdapter);
		void Finalize();
	};
}
}




#endif // !VISTLE_SENSEI_H
