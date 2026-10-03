struct Particle {
  float2 position;
  float2 velocity;
  float4 color;
};

[[vk::binding(1, 0)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(2, 0)]]
RWStructuredBuffer<Particle> particlesOut;

struct VertexInput
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float pointSize : SV_PointSize;
    float3 color : COLOR;
};

struct FragmentInput
{
    float2 pointCoord : SV_PointCoord;
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
    output.color = input.color.rgb;
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentInput input) : SV_Target0
{
    float2 coord = input.pointCoord - float2(0.5);
    return float4(input.color, 0.5 - length(coord));
}
