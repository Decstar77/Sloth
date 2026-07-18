#include "sloth_gui_renderer.h"

#include "renderer/sloth_shader.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sloth
{
    static const char* RectVertexShaderSource = R"(
        #version 450 core
        layout(location = 0) in vec2 aUnit;
        layout(location = 1) in vec2 aQuadMin;
        layout(location = 2) in vec2 aQuadMax;
        layout(location = 3) in vec4 aColor;

        uniform mat4 uViewProjection;

        out vec4 vColor;

        void main()
        {
            vec2 pixelPos = mix(aQuadMin, aQuadMax, aUnit);
            vColor = aColor;
            gl_Position = uViewProjection * vec4(pixelPos, 0.0, 1.0);
        }
    )";

    static const char* RectFragmentShaderSource = R"(
        #version 450 core

        in vec4 vColor;

        out vec4 FragColor;

        void main()
        {
            FragColor = vColor;
        }
    )";

    glm::mat4 MakeScreenProjection(f32 width, f32 height)
    {
        return glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    }

    GuiRenderer::GuiRenderer()
    {
        shader = std::make_unique<Shader>(RectVertexShaderSource, RectFragmentShaderSource);

        const f32 unitQuad[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
        };

        glGenVertexArrays(1, &quadVertexArray);
        glBindVertexArray(quadVertexArray);

        glGenBuffers(1, &quadVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuad), unitQuad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(f32) * 2, reinterpret_cast<void*>(0));

        glGenBuffers(1, &instanceBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);

        auto instanceAttrib = [](u32 location, i32 componentCount, usize offset)
        {
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(location, componentCount, GL_FLOAT, GL_FALSE, sizeof(RectInstance), reinterpret_cast<void*>(offset));
            glVertexAttribDivisor(location, 1);
        };

        instanceAttrib(1, 2, offsetof(RectInstance, QuadMin));
        instanceAttrib(2, 2, offsetof(RectInstance, QuadMax));
        instanceAttrib(3, 4, offsetof(RectInstance, Color));

        glBindVertexArray(0);
    }

    GuiRenderer::~GuiRenderer()
    {
        glDeleteBuffers(1, &instanceBuffer);
        glDeleteBuffers(1, &quadVertexBuffer);
        glDeleteVertexArrays(1, &quadVertexArray);
    }

    void GuiRenderer::DrawRect(glm::vec2 min, glm::vec2 max, const glm::vec4& color)
    {
        instances.push_back(RectInstance{ min, max, color });
    }

    void GuiRenderer::Flush(const glm::mat4& viewProjection)
    {
        if (instances.empty())
        {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
        usize requiredBytes = instances.size() * sizeof(RectInstance);
        if (requiredBytes > instanceBufferCapacity)
        {
            instanceBufferCapacity = requiredBytes;
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instanceBufferCapacity), instances.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(requiredBytes), instances.data());
        }

        bool blendWasEnabled = glIsEnabled(GL_BLEND);
        bool depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        shader->Bind();
        shader->SetMat4("uViewProjection", viewProjection);

        glBindVertexArray(quadVertexArray);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
        glBindVertexArray(0);

        if (depthTestWasEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }

        if (!blendWasEnabled)
        {
            glDisable(GL_BLEND);
        }

        instances.clear();
    }

} // namespace sloth
