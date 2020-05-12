#ifndef VISTLE_SENSEI_EXEPTION_H
#define VISTLE_SENSEI_EXEPTION_H
#include <insitu/core/exeption.h>

namespace insitu {
	namespace sensei {
		struct Exeption : public insitu::InsituExeption {
			Exeption() {
				*this << "VistleSenseiAdapter: ";
			};

		};
	}
}
#endif