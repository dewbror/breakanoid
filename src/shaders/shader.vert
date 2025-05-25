// GLSL version
#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 rotX90;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * ubo.rotX90 * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexColor = inTexColor;
}
