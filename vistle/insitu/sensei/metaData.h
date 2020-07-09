#ifndef VISTLE_INSITU_SENSEI_METADATA_H
#define VISTLE_INSITU_SENSEI_METADATA_H

#include "export.h"

#include <string>
#include <vector>

namespace vistle {
namespace insitu {

class V_SENSEIEXPORT MetaData { //This contains the names of all meshes and their linked data fields and must be provided before the connection to Vistle
public:

	typedef std::vector<std::string>::iterator MeshIter;
	typedef std::vector<std::string>::iterator VarIter;

	struct V_SENSEIEXPORT MetaVar {//this only contains the begin and end iterators for the variables of a mesh
		VarIter begin();
		VarIter end();
		friend class MetaData;
	private:
		MetaVar(const VarIter& begin, const VarIter& end)
			:m_begin(begin)
			, m_end(end)
		{};
		const VarIter m_begin, m_end;
	};
	MeshIter addMesh(const std::string& name);
	MeshIter getMesh(const std::string& name);
	MeshIter begin();
	MeshIter end();
	void addVariable(const std::string& varName, const MeshIter& mesh);
	void addVariable(const std::string& varName, const std::string& meshName);
	MetaVar getVariables(const MeshIter& mesh);
	MetaVar getVariables(const std::string& mesh);

private:

	std::vector<std::string> m_meshes;
	std::vector<std::vector<std::string>> m_variables;
};

} //insitu
} //vistle
#endif