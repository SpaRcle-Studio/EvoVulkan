#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec2 UV;

layout (binding = 0) uniform ModelUniformBuffer {
    mat4 model;
} ubo;

layout (binding = 1) uniform ViewUniformBuffer {
    mat4 projection;
    mat4 view;
} viewUbo;

#extension GL_ARB_separate_shader_objects : enable

void main() {
    UV = inUV;

    //gl_Position = vec4(inPos, 1.0);
    gl_Position = viewUbo.projection * viewUbo.view * ubo.model * vec4(inPos, 1.0);

    //fragColor = colors[gl_VertexIndex % 3];
}