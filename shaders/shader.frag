#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 Anormal;
layout(location = 2) in vec3 FragPos;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform GlobalUbo {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec3 camPos;
    vec3 rotation;
} ubo;

vec3 lightPos = vec3(2.0, 2.0, 2.0);
vec3 lightColor = vec3(1.0);
float shininess = 126.0;        // typical for Blinn–Phong
float specularStrength = 1.1;  // tweak this
float ambientStrength = 0.1;   // tweak this

void main() {
    vec3 norm = normalize(Anormal);
    vec3 lightDir   = normalize(lightPos - FragPos);
    vec3 viewDir    = normalize(ubo.camPos - FragPos);

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * fragColor * lightColor;

    // Specular (Blinn–Phong: halfway vector)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Ambient
    vec3 ambient = ambientStrength * fragColor;

    // Final result
    vec3 result = ambient + diffuse + specular;

    outColor = vec4(result, 1.0);
}
