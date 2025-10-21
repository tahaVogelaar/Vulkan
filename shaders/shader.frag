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

void main() {
    vec3 normal = normalize(Normal);
    vec3 lightDirection = normalize(ubo.camPos - FragPos);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // specualr
    vec3 viewDirection = normalize(ubo.camPos - FragPos);
    vec3 reflectionDirection = reflect(-lightDirection, normal);
    float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.f), 16);
    float specualr = specAmount;

    float angle = dot(ubo.forward, -lightDirection);
    float inten = clamp((angle - pointLight.light[0].outerCone) / (pointLight.light[0].innerCone - pointLight.light[0].outerCone), 0.f, 1.f);

    vec3 diffuseColor = diffuse * pointLight.light[0].intensity * vec3(1.0); // can add material color
    vec3 specularColor = specAmount * pointLight.light[0].specular * pointLight.light[0].intensity * vec3(1.0);


    // shadow
    float shadow = .0;
    vec4 lightSpacePos = pointLight.light[0].proj * vec4(FragPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    shadow = 0.0;

    vec3 color = (diffuseColor + specularColor) * inten;

    if(shadow > 0.0)
        outColor = vec4(color, 1.0);
    else
        outColor = vec4(0);
    outColor = vec4(1, 1, 1, 1);
}
