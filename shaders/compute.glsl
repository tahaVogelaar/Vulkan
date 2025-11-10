#version 450
/*
struct Particle {
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(binding = 0) uniform GlobalUbo {
    float deltaTime;
} ubo;*/

layout(binding = 0, rgba8) uniform writeonly image2D colorImage;
//layout(binding = 1, r32f)  uniform writeonly image2D depthImage;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    imageStore(colorImage, ivec2(gl_GlobalInvocationID.xy), vec4(1));
}