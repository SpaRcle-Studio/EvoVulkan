#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec2 UV;

layout (binding = 0) uniform UniformBuffer {
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

#extension GL_ARB_separate_shader_objects : enable

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);


void main() {
    UV = inUV;

    //gl_Position = vec4(inPos, 1.0);
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0);

    //fragColor = colors[gl_VertexIndex % 3];
}