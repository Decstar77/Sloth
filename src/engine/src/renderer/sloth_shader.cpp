#include "sloth_shader.h"

#include <glad/gl.h>

namespace sloth
{
    static u32 CompileStage(GLenum stage, const char* source)
    {
        u32 id = glCreateShader(stage);
        glShaderSource(id, 1, &source, nullptr);
        glCompileShader(id);

        i32 success = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetShaderInfoLog(id, sizeof(infoLog), nullptr, infoLog);
            SL_LOG_ERROR("Shader compilation failed: %s", infoLog);
            glDeleteShader(id);
            return 0;
        }

        return id;
    }

    Shader::Shader(const char* vertexSource, const char* fragmentSource)
    {
        u32 vertexShader = CompileStage(GL_VERTEX_SHADER, vertexSource);
        u32 fragmentShader = CompileStage(GL_FRAGMENT_SHADER, fragmentSource);

        rendererId = glCreateProgram();
        glAttachShader(rendererId, vertexShader);
        glAttachShader(rendererId, fragmentShader);
        glLinkProgram(rendererId);

        i32 success = 0;
        glGetProgramiv(rendererId, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[1024];
            glGetProgramInfoLog(rendererId, sizeof(infoLog), nullptr, infoLog);
            SL_LOG_ERROR("Shader linking failed: %s", infoLog);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    Shader::~Shader()
    {
        glDeleteProgram(rendererId);
    }

    void Shader::Bind() const
    {
        glUseProgram(rendererId);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }

    void Shader::SetMat4(const char* name, const glm::mat4& value)
    {
        Bind();
        i32 location = glGetUniformLocation(rendererId, name);
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }

    void Shader::SetInt(const char* name, i32 value)
    {
        Bind();
        i32 location = glGetUniformLocation(rendererId, name);
        glUniform1i(location, value);
    }

} // namespace sloth
