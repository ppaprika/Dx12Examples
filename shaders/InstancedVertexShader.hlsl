struct ModelViewProjection
{
    matrix MVP;
};

struct InstancePos
{
    matrix TranslationMatrix;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

ConstantBuffer<InstancePos> TranslationMatrixCB : register(b1);

struct VertexPosColor
{
    float3 Position : MYPOSITION;
    float3 Color : MYCOLOR;
    float2 TexCoord : MYTEXCOORD;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.Position = mul(TranslationMatrixCB.TranslationMatrix, OUT.Position);

    OUT.TexCoord = IN.TexCoord;

    return OUT;
}