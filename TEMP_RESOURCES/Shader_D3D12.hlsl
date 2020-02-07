cbuffer ConstantBufferObject : register(b0)
{
	float constrast;
    float4x4 camera;
};

struct VSInput
{
    float4 position : POSITION0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    float4 pos = input.position;
    result.position = mul(pos, camera);
    result.color = input.color * constrast;
    result.uv = input.uv;

    return result;
}

Texture2D mainTexture : register(t0);
Texture2D mainTexture2 : register(t1);
SamplerState mainSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color * (mainTexture.Sample(mainSampler, input.uv) + mainTexture2.Sample(mainSampler, input.uv));
}
