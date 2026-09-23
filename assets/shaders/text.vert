#version 450

layout(location = 0) in ivec2 inPos;

struct GlyphInstance {
    vec2 position;
    float scale;
    uint glyphIndex; // TODO: do we need this in shader?
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(std430, set = 1, binding = 0) readonly buffer InstanceBuffer {
    GlyphInstance instances[];
};

layout(location = 0) out vec3 fragColor;

void main() {
    GlyphInstance instance = instances[gl_InstanceIndex];
    vec2 pos = inPos * instance.scale;
    pos.y = -pos.y;
    instance.position.y = -instance.position.y;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(pos + instance.position, 0.0, 1.0);
    fragColor = vec3(1.0, 1.0, 1.0);
}
