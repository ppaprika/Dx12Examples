struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);


float4 main( PixelShaderInput IN ) : SV_Target
{
     return g_texture.Sample(g_sampler, IN.TexCoord);
}