#version 450

layout (location = 0) in vec3 UV;

//layout (binding = 1) uniform samplerCube cubeMapTexture;

layout (location = 1) out vec4 outColor;

void main() {
    //gl_FragColor = texture(cubeMapTexture, UV);
    //outColor = texture(cubeMapTexture, UV);
    outColor = vec4(1,0,0, 1);
}
