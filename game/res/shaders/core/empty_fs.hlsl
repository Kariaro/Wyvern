static float4 o_Albedo;
static float4 o_Normal;
static float3 Normal;
static float4 o_Position;
static float3 Pos;
static float4 o_RoughnessMetallic;
static float2 TexCoord;

struct SPIRV_Cross_Input
{
    float2 TexCoord : TEXCOORD1;
    float3 Normal : TEXCOORD2;
    float3 Pos : TEXCOORD3;
};

struct SPIRV_Cross_Output
{
    float4 o_Albedo : SV_Target0;
    float4 o_Normal : SV_Target1;
    float4 o_Position : SV_Target2;
    float4 o_RoughnessMetallic : SV_Target3;
};

void frag_main()
{
    o_Albedo = 0.0f.xxxx;
    o_Normal = float4(Normal, 1.0f);
    o_Position = float4(Pos, 1.0f);
    o_RoughnessMetallic = 1.0f.xxxx;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    Normal = stage_input.Normal;
    Pos = stage_input.Pos;
    TexCoord = stage_input.TexCoord;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.o_Albedo = o_Albedo;
    stage_output.o_Normal = o_Normal;
    stage_output.o_Position = o_Position;
    stage_output.o_RoughnessMetallic = o_RoughnessMetallic;
    return stage_output;
}
