#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UniformBuffer {
    mat4 projection;
    mat4 view;
    mat4 model;
} ubo;

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

vec3 colors[6] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),

    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);


void main() {
    //gl_Position = vec4(inPos, 1.0);
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0);

    fragColor = colors[gl_VertexIndex];
}