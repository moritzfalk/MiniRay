struct VS_Input {
    float2 pos : POS;
    float2 uv : TEX;
};

struct VS_Output {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

VS_Output vs_main(VS_Input input)
{
    VS_Output output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = input.uv;
    return output;
}

//NOTE(moritz): Hash source https://www.shadertoy.com/view/XlXcW4
//NOTE(moritz): Shoutout to The Sandvich Maker on the Handmade Network
static uint k = 1103515245U;

float3 hash( uint3 x )
{
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    x = ((x>>8U)^x.yzx)*k;
    
    return float3(x)*(1.0/float(0xffffffffU));
}

float3 
ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

float4 ps_main(VS_Output input) : SV_Target
{
	float4 OutColor = mytexture.Sample(mysampler, input.uv);

	//NOTE(moritz): Throwing in tone mapping just for fun
	OutColor.rgb = ACESFilm(OutColor.rgb);

	//NOTE(moritz): Dithering
	float3 OutColor255 = 255.0f*OutColor.rgb;
	//OutColor255 += hash(uint3(input.pos.xyz));
	OutColor255 += (2.0f*hash(uint3(input.pos.xyz)) - 1.0f);
    OutColor.rgb = OutColor255/255.0f;
	clamp(OutColor.rgb, float3(0, 0, 0), float3(1, 1, 1));
    return OutColor;   
}