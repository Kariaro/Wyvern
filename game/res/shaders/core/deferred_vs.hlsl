static float4 gl_Position;
static float2 TexCoord;
static float2 a_TexCoord0;
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
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    TexCoord = a_TexCoord0;
    gl_Position = float4(a_Pos, 1.0f);
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
    return stage_output;
}
