#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 Anormal;
layout(location = 2) out vec3 FragPos;

layout(set = 0, binding = 0, std140) uniform GlobalUbo {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec3 camPos;
    vec3 rotation;
} ubo;

void main() {

    gl_Position = ubo.projView * vec4(position, 1.0);

    FragPos = vec3(mat4(1) * vec4(position, 1.0));
    Anormal = normal;
    fragColor = color;
}
