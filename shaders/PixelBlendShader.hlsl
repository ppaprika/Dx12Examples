struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
Texture2D another_texture : register(t1);
SamplerState g_sampler : register(s0);

cbuffer TextureTypeBuffer : register(b1)
{
	int g_texture_type;
}

float4 main( PixelShaderInput IN ) : SV_Target
{
     return another_texture.Sample(g_sampler, IN.TexCoord);
}