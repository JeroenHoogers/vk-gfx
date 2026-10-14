struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::binding(1, 0)]]
Texture2D<float4> texture;

[[vk::binding(2, 0)]]
SamplerState sampler;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    float4x4 mvp = mul(ubo.proj, mul(ubo.view, ubo.model));
    output.position = mul(mvp, float4(input.position, 1.0));
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    return texture.Sample(sampler, input.uv);
}
