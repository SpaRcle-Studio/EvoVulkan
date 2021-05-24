#version 450

#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout(location = 0) in vec2 UV;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 albedo = texture(samplerColor, 1 - UV);

    //if (albedo.b > albedo.g)
    //    discard;
    outColor = albedo;

    //outColor = albedo.b > albedo.g ? vec4(0, 0, 0, 0) : albedo;

    //outColor = vec4(fragColor.xyz, 1.0);
    //outColor = vec4(UV, 1.0, 1.0);
}