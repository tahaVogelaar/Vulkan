#version 460
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

void main() {
    vec3 col = vec3(uv.x, uv.y, 0);

    // Output to screen
    fragColor = vec4(col,1.0);
}