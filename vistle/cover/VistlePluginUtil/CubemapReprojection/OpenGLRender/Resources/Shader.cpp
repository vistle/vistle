// OpenGLRender/Resources/Shader.cpp

#include <OpenGLRender/Resources/Shader.h>

#include <Core/Comparison.h>
#include <Core/System/SimpleIO.h>
#include <EngineBuildingBlocks/ErrorHandling.h>

#include <sstream>
#include <cassert>

using namespace OpenGLRender;

const unsigned c_VertexElementTypeMap[] =
{
	0,					// Unknown
	GL_FLOAT,			// Float
	GL_UNSIGNED_INT		// Uint32
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShaderDefine::operator==(const ShaderDefine& other) const
{
	StringEqualCompareBlock(Name);
	StringEqualCompareBlock(Definition);
	return true;
}

bool ShaderDefine::operator!=(const ShaderDefine& other) const
{
	return !(*this == other);
}

bool ShaderDefine::operator<(const ShaderDefine& other) const
{
	StringLessCompareBlock(Name);
	StringLessCompareBlock(Definition);
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

inline std::string GetShaderVersion(const std::string& version)
{
	if(version.empty()) return  "450 core";
	return version;
}

ShaderDescription ShaderDescription::FromText(const std::string& shaderText,
	ShaderType type, const std::string& version, const std::vector<ShaderDefine>& defines)
{
	return{ shaderText, ShaderSourceType::Text, type, GetShaderVersion(version), defines };
}

ShaderDescription ShaderDescription::FromFile(const std::string& filePath,
	ShaderType type, const std::string& version, const std::vector<ShaderDefine>& defines)
{
	return { filePath, ShaderSourceType::File, type, GetShaderVersion(version), defines };
}

ShaderDescription ShaderDescription::FromFile(const EngineBuildingBlocks::PathHandler& pathHandler,
	const std::string& fileName, ShaderType type, const std::string& version,
	const std::vector<ShaderDefine>& defines)
{
	return{ pathHandler.GetPathFromRootDirectory("Shaders/" + fileName), ShaderSourceType::File,
		type, GetShaderVersion(version), defines };
}

const std::string& ShaderDescription::GetFilePath() const
{
	assert(SourceType == ShaderSourceType::File);
	return SourceString;
}

std::string ShaderDescription::GetText() const
{
	if (SourceType == ShaderSourceType::Text) return SourceString;
	std::string text;
	Core::ReadAllText(SourceString, text);
	return text;
}

bool ShaderDescription::operator==(const ShaderDescription& other) const
{
	NumericalEqualCompareBlock(Type);
	NumericalEqualCompareBlock(SourceType);
	StringEqualCompareBlock(SourceString);
	StructureEqualCompareBlock(Defines);
	StringEqualCompareBlock(Version);
	return true;
}

bool ShaderDescription::operator!=(const ShaderDescription& other) const
{
	return !(*this == other);
}

bool ShaderDescription::operator<(const ShaderDescription& other) const
{
	NumericalLessCompareBlock(Type);
	NumericalLessCompareBlock(SourceType);
	StringLessCompareBlock(SourceString);
	StructureLessCompareBlock(Defines);
	StringLessCompareBlock(Version);
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

Shader::Shader(const ShaderDescription& description) noexcept
{
	m_Description = description;

	std::stringstream ss;

	// Adding the shader version.
	if (!description.Version.empty())
	{
		ss << "#version " << description.Version << "\n";
	}

	// Adding shader defines.
	for (size_t i = 0; i < description.Defines.size(); i++)
	{
		auto& define = description.Defines[i];
		ss << "#define " << define.Name << " " << define.Definition << "\n";
	}

	// Adding starting line.
	ss << "#line 1" << "\n";

	// Adding the shader the text.
	ss << description.GetText();

	// Compiling the shader.
	auto text = ss.str();
	auto cText = text.c_str();
	m_Handle = glCreateShader((GLenum)description.Type);
	glShaderSource(m_Handle, 1, &cText, nullptr);
	glCompileShader(m_Handle);

	// Checking compiled shader.
	{
		int result;
		glGetShaderiv(m_Handle, GL_COMPILE_STATUS, &result);
		if (result != GL_TRUE)
		{
			std::stringstream ss;
			ss << "Shader error in '";
			if (description.SourceType == ShaderSourceType::Text) ss << "shader from text";
			else ss << description.SourceString;
			ss << "' :\n";
			int infoLogLength = 0;
			glGetShaderiv(m_Handle, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0)
			{
				char* buffer = new char[infoLogLength];
				glGetShaderInfoLog(m_Handle, infoLogLength, nullptr, buffer);
				ss << buffer << "\n";
				delete[] buffer;
			}
			EngineBuildingBlocks::RaiseException(ss);
		}
	}
}

void Shader::DeleteResource(GLuint handle)
{
	glDeleteShader(handle);
}

GLuint Shader::GetHandle() const
{
	return m_Handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderProgram::DeleteResource(GLuint handle)
{
	glDeleteProgram(handle);
}

GLuint ShaderProgram::GetHandle() const
{
	return m_Handle;
}

GLint ShaderProgram::GetUniformLocation(const char* name) const
{
	return glGetUniformLocation(m_Handle, name);
}

GLint ShaderProgram::GetAttributeLocation(const char* name) const
{
	return glGetAttribLocation(m_Handle, name);
}

void ShaderProgram::Initialize(const ShaderProgramDescription& description)
{
	// Creating shaders.
	for (size_t i = 0; i < description.Shaders.size(); i++)
	{
		m_Shaders.emplace_back(description.Shaders[i]);
	}

	// Creating the shader program.
	m_Handle = glCreateProgram();
	if (m_Handle == 0) EngineBuildingBlocks::RaiseException("Invalid program handle.");

	// Attaching shaders.
	for (size_t i = 0; i < m_Shaders.size(); ++i)
	{
		glAttachShader(m_Handle, m_Shaders[i].GetHandle());
	}

	// Linking the program.
	glLinkProgram(m_Handle);

	// Checking shader program.
	{
		int result = GL_FALSE;
		glGetProgramiv(m_Handle, GL_LINK_STATUS, &result);
		if (result != GL_TRUE)
		{
			std::stringstream ss;
			ss << "Shader program error:\n";
			int infoLogLength = 0;
			glGetProgramiv(m_Handle, GL_INFO_LOG_LENGTH, &infoLogLength);
			if (infoLogLength > 0)
			{
				char* buffer = new char[infoLogLength];
				glGetProgramInfoLog(m_Handle, infoLogLength, nullptr, buffer);
				ss << buffer << "\n";
				delete[] buffer;
			}
			EngineBuildingBlocks::RaiseException(ss);
		}
	}
}

void ShaderProgram::Bind()
{
	glUseProgram(m_Handle);
}

void ShaderProgram::Unbind()
{
	glUseProgram(0U);
}

void ShaderProgram::SetAttributeBuffer(GLint location, int tupleSize, GLenum type, int stride, int offset,
	GLboolean normalized)
{
	auto offsetPtr = reinterpret_cast<const void*>(static_cast<long long>(offset));
	switch (type)
	{
	case GL_BYTE:
	case GL_UNSIGNED_BYTE:
	case GL_SHORT:
	case GL_UNSIGNED_SHORT:
	case GL_INT:
	case GL_UNSIGNED_INT:
		glVertexAttribIPointer(location, tupleSize, type, stride, offsetPtr); break;
	case GL_DOUBLE:
		glVertexAttribLPointer(location, tupleSize, type, stride, offsetPtr); break;
	default:
		assert(type == GL_HALF_FLOAT || type == GL_FLOAT || type == GL_FIXED || type == GL_INT_2_10_10_10_REV
			|| type == GL_UNSIGNED_INT_2_10_10_10_REV || type == GL_UNSIGNED_INT_10F_11F_11F_REV);
		glVertexAttribPointer(location, tupleSize, type, normalized, stride, offsetPtr); break;
	}
}

void ShaderProgram::SetAttributeBuffer(const char* name, int tupleSize, GLenum type, int stride, int offset,
	GLboolean normalized)
{
	SetAttributeBuffer(GetAttributeLocation(name), tupleSize, type, stride, offset, normalized);
}

void ShaderProgram::EnableAttributeArray(GLint location)
{
	assert(location != -1);
	glEnableVertexAttribArray(location);
}

void ShaderProgram::EnableAttributeArray(const char* name)
{
	EnableAttributeArray(GetAttributeLocation(name));
}

void ShaderProgram::SetAttributeDivisor(GLint location, GLuint divisor)
{
	glVertexAttribDivisor(location, divisor);
}

void ShaderProgram::SetAttributeDivisor(const char* name, GLuint divisor)
{
	SetAttributeDivisor(GetAttributeLocation(name), divisor);
}

void ShaderProgram::SetInputLayout(const EngineBuildingBlocks::Graphics::VertexInputLayout& inputLayout)
{
	SetInputLayout(inputLayout, Core::c_InvalidSizeU);
}

void ShaderProgram::SetInputLayout(const EngineBuildingBlocks::Graphics::VertexInputLayout& inputLayout,
	unsigned vertexAttributeDivisor)
{
	int offset = 0;
	int stride = (int)inputLayout.GetVertexStride();
	auto& ilElements = inputLayout.Elements;
	for (size_t i = 0; i < ilElements.size(); i++)
	{
		auto& element = ilElements[i];
		auto location = GetAttributeLocation(element.Name.c_str());
		assert(location != -1);
		SetAttributeBuffer(location, element.Count, c_VertexElementTypeMap[(int)element.Type], stride, offset, GL_FALSE);
		EnableAttributeArray(location);
		if (vertexAttributeDivisor != Core::c_InvalidSizeU) SetAttributeDivisor(location, vertexAttributeDivisor);
		offset += element.GetTotalSize();
	}
}

void ShaderProgram::SetUniformValue(GLint location, int value)
{
	assert(location != -1);
	glUniform1i(location, value);
}

void ShaderProgram::SetUniformValue(GLint location, unsigned value)
{
	assert(location != -1);
	glUniform1ui(location, value);
}

void ShaderProgram::SetUniformValue(GLint location, float value)
{
	assert(location != -1);
	glUniform1f(location, value);
}

void ShaderProgram::SetUniformValue(GLint location, double value)
{
	assert(location != -1);
	glUniform1d(location, value);
}

void ShaderProgram::SetUniformValue(GLint location, const glm::ivec2& value)
{
	assert(location != -1);
	glUniform2iv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::uvec2& value)
{
	assert(location != -1);
	glUniform2uiv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::vec2& value)
{
	assert(location != -1);
	glUniform2fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::dvec2& value)
{
	assert(location != -1);
	glUniform2dv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::ivec3& value)
{
	assert(location != -1);
	glUniform3iv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::uvec3& value)
{
	assert(location != -1);
	glUniform3uiv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::vec3& value)
{
	assert(location != -1);
	glUniform3fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::dvec3& value)
{
	assert(location != -1);
	glUniform3dv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::ivec4& value)
{
	assert(location != -1);
	glUniform4iv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::uvec4& value)
{
	assert(location != -1);
	glUniform4uiv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::vec4& value)
{
	assert(location != -1);
	glUniform4fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::dvec4& value)
{
	assert(location != -1);
	glUniform4dv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(GLint location, const glm::mat4& value)
{
	assert(location != -1);
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::SetUniformValue(const char* name, int value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, unsigned value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, float value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, double value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::ivec2& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::uvec2& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::vec2& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::dvec2& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::ivec3& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::uvec3& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::vec3& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::dvec3& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::ivec4& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::uvec4& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::vec4& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::dvec4& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValue(const char* name, const glm::mat4& value)
{
	SetUniformValue(GetUniformLocation(name), value);
}

void ShaderProgram::SetUniformValueArray(GLint location, const int* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform1iv(location, count, pValue);
}

void ShaderProgram::SetUniformValueArray(GLint location, const unsigned* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform1uiv(location, count, pValue);
}

void ShaderProgram::SetUniformValueArray(GLint location, const float* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform1fv(location, count, pValue);
}

void ShaderProgram::SetUniformValueArray(GLint location, const double* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform1dv(location, count, pValue);
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::ivec2* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform2iv(location, count, reinterpret_cast<const int*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::uvec2* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform2uiv(location, count, reinterpret_cast<const unsigned*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::vec2* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform2fv(location, count, reinterpret_cast<const float*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::dvec2* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform2dv(location, count, reinterpret_cast<const double*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::ivec3* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform3iv(location, count, reinterpret_cast<const int*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::uvec3* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform3uiv(location, count, reinterpret_cast<const unsigned*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::vec3* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform3fv(location, count, reinterpret_cast<const float*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::dvec3* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform3dv(location, count, reinterpret_cast<const double*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::ivec4* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform4iv(location, count, reinterpret_cast<const int*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::uvec4* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform4uiv(location, count, reinterpret_cast<const unsigned*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::vec4* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform4fv(location, count, reinterpret_cast<const float*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::dvec4* pValue, GLsizei count)
{
	assert(location != -1);
	glUniform4dv(location, count, reinterpret_cast<const double*>(pValue));
}

void ShaderProgram::SetUniformValueArray(GLint location, const glm::mat4* pValue, GLsizei count)
{
	assert(location != -1);
	glUniformMatrix4fv(location, count, GL_FALSE, reinterpret_cast<const float*>(pValue));
}

void ShaderProgram::SetUniformValueArray(const char* name, const int* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const unsigned* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const float* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const double* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::ivec2* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::uvec2* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::vec2* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::dvec2* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::ivec3* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::uvec3* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::vec3* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::dvec3* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::ivec4* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::uvec4* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::vec4* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::dvec4* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}

void ShaderProgram::SetUniformValueArray(const char* name, const glm::mat4* pValue, GLsizei count)
{
	SetUniformValueArray(GetUniformLocation(name), pValue, count);
}