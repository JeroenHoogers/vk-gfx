struct VertexInput
{
    int2 position : POSITION;
    uint startInstance : SV_StartInstanceLocation;
    uint instanceIndex : SV_InstanceID;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

struct GlyphInstance {
    float2 position;
    float scale;
    uint glyphIndex; // TODO: do we need this in shader?
};

struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::binding(0, 1)]]
StructuredBuffer<GlyphInstance> instances;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    uint instanceIndex = input.startInstance + input.instanceIndex;
    GlyphInstance instance = instances[instanceIndex];

    float2 pos = input.position * instance.scale;
    pos.y = -pos.y;
    instance.position.y = -instance.position.y;

    float4x4 mvp = mul(ubo.proj, mul(ubo.view, ubo.model));
    output.position = mul(mvp, float4(pos + instance.position, 0.0, 1.0));
    output.color = float3(1.0, 1.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    return float4(input.color, 1.0);
}
