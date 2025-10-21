#version 460
#extension GL_ARB_shader_draw_parameters : enable
#extension VK_EXT_descriptor_indexing : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 Anormal;
layout(location = 2) out vec3 FragPos;
layout(location = 3) out vec2 UV;
layout(location = 4) out flat uint materialID;

struct Object {
    mat4 model;
    uint materialId;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(set = 0, binding = 0, std140) uniform GlobalUbo {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec3 camPos;
    vec3 rotation;
    vec3 forward;
} ubo;

layout(std430, set = 0, binding = 1) readonly buffer ObjectBuffer {
    Object objects[];
} aa;



void main() {
    uint objectID = gl_BaseInstance + gl_InstanceIndex;

    gl_Position = ubo.projView * aa.objects[gl_InstanceIndex].model * vec4(position, 1.0);

    FragPos = vec3(aa.objects[gl_InstanceIndex].model * vec4(position, 1.0));

    Anormal = normal;
    fragColor = color;
    UV = uv;
    materialID = aa.objects[gl_InstanceIndex].materialId;
}
