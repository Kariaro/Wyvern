cbuffer UbInstanceData : register(b1)
{
    row_major float4x4 _17_u_Projection : packoffset(c0);
    row_major float4x4 _17_u_View : packoffset(c4);
    row_major float4x4 _17_u_Model : packoffset(c8);
    float2 _17_u_UVOffset : packoffset(c12);
};


static float4 gl_Position;
static float2 TexCoord;
static float2 a_TexCoord0;
static float3 Normal;
static float3 Pos;
static float3 a_Pos;
static float3 a_Normal;
static float3 a_Tangent;
static float4 a_Color;

struct SPIRV_Cross_Input
{
    float3 a_Pos : POSITION;
    float3 a_Normal : NORMAL;
    float3 a_Tangent : TEXCOORD0;
    float4 a_Color : TEXCOORD1;
    float2 a_TexCoord0 : TEXCOORD2;
};

struct SPIRV_Cross_Output
{
    float2 TexCoord : TEXCOORD1;
    float3 Normal : TEXCOORD2;
    float3 Pos : TEXCOORD3;
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    TexCoord = a_TexCoord0 + _17_u_UVOffset;
    Normal = 0.0f.xxx;
    Pos = a_Pos;
    gl_Position = mul(float4(a_Pos, 1.0f), mul(_17_u_Model, mul(_17_u_View, _17_u_Projection)));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    a_TexCoord0 = stage_input.a_TexCoord0;
    a_Pos = stage_input.a_Pos;
    a_Normal = stage_input.a_Normal;
    a_Tangent = stage_input.a_Tangent;
    a_Color = stage_input.a_Color;
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    stage_output.TexCoord = TexCoord;
    stage_output.Normal = Normal;
    stage_output.Pos = Pos;
    return stage_output;
}
