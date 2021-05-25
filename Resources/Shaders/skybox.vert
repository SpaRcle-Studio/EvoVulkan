#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 UV;

layout (binding = 0) uniform SkyboxUBO {
    mat4 PVMat;
    vec3 CamPos;
} ubo;

void main() {
    UV = inPos;
    //gl_Position = vec4(inPos, 0);
    gl_Position = (ubo.PVMat * vec4(inPos + vec3(-ubo.CamPos.x, -ubo.CamPos.y, -ubo.CamPos.z), 1.0)).xyww;
}
