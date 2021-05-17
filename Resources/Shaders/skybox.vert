#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 UV;

layout (binding = 0) uniform SkyboxUBO {
    mat4 PVMat;
    vec3 cameraPos;
} ubo;

void main() {
    UV = inPos;
    gl_Position = (PVmat * vec4(aPos + vec3(CamPos.x, CamPos.y, -CamPos.z), 1.0)).xyww;
}
