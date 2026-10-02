struct Particle {
  [[vk::location(0)]] float2 position : POSITION;
  float2 velocity;
  [[vk::location(1)]] float4 color : COLOR;
};

[[vk::binding(0, 1)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(0, 2)]]
RWStructuredBuffer<Particle> particlesOut;

struct VertexInput
{
    float2 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float pointSize : PSIZE;
    float2 pointCoord : TEXCOORD0;
    float3 color : COLOR;
};

struct UniformBufferObject {
    float deltaTime;
};

// TODO: could be a pushconstant?
[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;

    Particle particleIn = particlesIn[index];
    Particle particleOut;

    particleOut.position = particleIn.position + particleIn.velocity.xy * ubo.deltaTime;
    particleOut.velocity = particleIn.velocity;

    // Flip movement at window border
    if ((particleOut.position.x <= -1.0) || (particleOut.position.x >= 1.0)) {
        particleOut.velocity.x = -particleOut.velocity.x;
    }
    if ((particleOut.position.y <= -1.0) || (particleOut.position.y >= 1.0)) {
        particleOut.velocity.y = -particleOut.velocity.y;
    }

    particlesOut[index] = particleOut;
}

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 1.0, 1.0);
    output.pointSize = 14.0;
    output.color = input.color;
    return output;
}


[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target0
{
    float2 coord = input.pointCoord - float2(0.5);
    return float4(input.color, 0.5 - length(coord));
}
