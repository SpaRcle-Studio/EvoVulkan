#version 450

layout (binding = 0) uniform UniformBuffer {
    float gamma;
} ubo;

layout (binding = 1) uniform sampler2D samplerColor;

//!===================================================

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

void main() {
    float gamma = 0.9;

    vec4 albedo  = texture(samplerColor, inUV);

    outFragcolor = vec4(pow(albedo.rgb, vec3(1.0 / gamma)), albedo.a);
}