#version 460
#extension GL_ARB_shader_draw_parameters : enable
#extension VK_EXT_descriptor_indexing : enable

layout(location = 0) in vec3 position;

struct Object {
    mat4 model;
    uint materialId;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

struct PointLight {
    mat4 proj;
    vec3 position;
    vec3 rotation;
    float innerCone;
    float outerCone;
    float range;
    float specular;
    float intensity;
    float filler;
};

layout(std430, set = 0, binding = 2) readonly buffer PointLightBuffer {
    PointLight light[];
} pointLight;

layout(std430, set = 0, binding = 1) readonly buffer ObjectBuffer {
    Object objects[];
} aa;

void main() {
    gl_Position = pointLight.light[0].proj * aa.objects[gl_InstanceIndex].model * vec4(position, 1.0);
}
