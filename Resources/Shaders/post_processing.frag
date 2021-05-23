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

    vec3 albedo  = texture(samplerColor, inUV).rgb;
    outFragcolor = vec4(pow(albedo, vec3(1.0 / gamma)), 1);
}