#version 450

layout (location = 0) in vec3 UV;

layout (binding = 1) uniform sampleCuber cubeMapTexture;

void main() {
    gl_FragColor = texture(cubeMapTexture, UV);
}
