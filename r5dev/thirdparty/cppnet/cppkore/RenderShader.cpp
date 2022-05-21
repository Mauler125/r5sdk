#include "stdafx.h"
#include "RenderShader.h"
#include "File.h"

// We need to include the OpenGL classes
#include "Mangler.h"

namespace Assets
{
	RenderShader::RenderShader()
		: _ProgramID(0)
	{
	}

	RenderShader::~RenderShader()
	{
		glDeleteProgram(this->_ProgramID);
	}

	void RenderShader::LoadShader(const char* VertSource, const char* FragSource)
	{
		auto VertShaderID = glCreateShader(GL_VERTEX_SHADER);
		auto FragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		if (VertSource != nullptr && FragSource != nullptr)
		{
			glShaderSource(VertShaderID, 1, &VertSource, nullptr);
			glCompileShader(VertShaderID);

			glShaderSource(FragShaderID, 1, &FragSource, nullptr);
			glCompileShader(FragShaderID);
		}

		this->_ProgramID = glCreateProgram();

		glAttachShader(this->_ProgramID, VertShaderID);
		glAttachShader(this->_ProgramID, FragShaderID);
		glLinkProgram(this->_ProgramID);

		glDetachShader(this->_ProgramID, VertShaderID);
		glDetachShader(this->_ProgramID, FragShaderID);

		glDeleteShader(VertShaderID);
		glDeleteShader(FragShaderID);
	}

	void RenderShader::LoadShader(const string& VertPath, const string& FragPath)
	{
		auto VertShaderID = glCreateShader(GL_VERTEX_SHADER);
		auto FragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

		auto VertSource = IO::File::ReadAllText(VertPath);
		auto FragSource = IO::File::ReadAllText(FragPath);

		char* VertPtr = (char*)VertSource.ToCString();
		char* FragPtr = (char*)FragSource.ToCString();

		glShaderSource(VertShaderID, 1, &VertPtr, nullptr);
		glCompileShader(VertShaderID);

		glShaderSource(FragShaderID, 1, &FragPtr, nullptr);
		glCompileShader(FragShaderID);

#if _DEBUG
		GLint success = 0;
		glGetShaderiv(VertShaderID, GL_COMPILE_STATUS, &success);
		
		if (!success)
		{
			GLint maxLength = 0;
			glGetShaderiv(VertShaderID, GL_INFO_LOG_LENGTH, &maxLength);
			auto log = std::make_unique<char[]>(maxLength);
			glGetShaderInfoLog(VertShaderID, maxLength, &maxLength, log.get());
			printf("Shader compile error: %s\n", log.get());
		}
#endif

		this->_ProgramID = glCreateProgram();

		glAttachShader(this->_ProgramID, VertShaderID);
		glAttachShader(this->_ProgramID, FragShaderID);
		glLinkProgram(this->_ProgramID);

		glDetachShader(this->_ProgramID, VertShaderID);
		glDetachShader(this->_ProgramID, FragShaderID);

		glDeleteShader(VertShaderID);
		glDeleteShader(FragShaderID);
	}

	void RenderShader::Use()
	{
		glUseProgram(this->_ProgramID);
	}

	void RenderShader::Detatch()
	{
		glUseProgram(0);
	}

	uint32_t RenderShader::GetUniformLocation(const char* Name)
	{
		return glGetUniformLocation(this->_ProgramID, Name);
	}
}
