#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 FragPos;
layout(location = 3) in vec2 UV;
layout(location = 4) in flat uint materialID;

layout (location = 0) out vec4 outColor;

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

layout(set = 0, binding = 0, std140) uniform GlobalUbo {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec3 camPos;
    vec3 rotation;
    vec3 forward;
} ubo;

layout(std430, set = 0, binding = 2) readonly buffer PointLightBuffer {
    PointLight light[];
} pointLight;

layout(set = 0, binding = 3) uniform sampler2D textures[];
layout(set = 0, binding = 4) uniform sampler2D shadowMap;

//nonuniformEXT uint idx = materialID;

vec3 spotLight(int l)
{
    vec3 color = vec3(pointLight.light[l].innerCone);

    return color;
}

void main()
{

    outColor = vec4(1, 1, 1, 1);
}