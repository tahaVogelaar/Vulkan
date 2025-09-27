#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

struct Ubo{
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec3 rotation;
    vec3 camPos;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    Ubo ubo;
} ubo;

layout(set = 0, binding = 1) buffer DrawData {
    mat4 model[1];
    uint materialId[1];
} ssbo;

void main() {
    mat4 modelMatrix = ssbo.model[gl_InstanceIndex];

    gl_Position = ubo.ubo.projView * modelMatrix * vec4(position, 1.0);

    fragColor = vec3(1.0);
}
