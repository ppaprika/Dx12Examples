struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position : MYPOSITION;
    float3 Color    : MYCOLOR;
    float2 TexCoord : MYTEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOO;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.TexCoord = IN.TexCoord;

    return OUT;
}