#ifndef VISTLE_INSITU_SENSEI_METADATA_H
#define VISTLE_INSITU_SENSEI_METADATA_H

#include "export.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace vistle {
namespace insitu {
namespace sensei {
class V_SENSEIEXPORT MetaData { //This contains the names of all meshes and their linked data fields and must be provided before the connection to Vistle
public:

	typedef std::unordered_map<std::string, std::vector<std::string>>::const_iterator ConstMeshIter;
	typedef std::unordered_map<std::string, std::vector<std::string>>::iterator MeshIter;
	typedef std::vector<std::string>::const_iterator VarIter;

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
	ConstMeshIter getMesh(const std::string& name) const;
	MeshIter addMesh(const std::string& name);
	MeshIter getMesh(const std::string& name);

	ConstMeshIter begin() const;
	ConstMeshIter end() const;
	MeshIter begin() ;
	MeshIter end() ;

	void addVariable(const std::string& varName, const MeshIter& mesh);
	void addVariable(const std::string& varName, const std::string& meshName);
	MetaVar getVariables(const ConstMeshIter& mesh) const;
	MetaVar getVariables(const std::string& mesh) const;

private:
	std::unordered_map<std::string, std::vector<std::string>> m_meshes;

};

} //sensei
} //insitu
} //vistle
#endif