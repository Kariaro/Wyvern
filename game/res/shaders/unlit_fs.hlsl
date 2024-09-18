Texture2D<float4> u_Albedo : register(t0);
SamplerState _u_Albedo_sampler : register(s0);

static float4 o_Albedo;
static float2 TexCoord;
static float4 o_Normal;
static float3 Normal;
static float4 o_Position;
static float3 Pos;
static float4 o_RoughnessMetallic;

struct SPIRV_Cross_Input
{
    float2 TexCoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 Pos : TEXCOORD2;
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
    o_Albedo = u_Albedo.Sample(_u_Albedo_sampler, TexCoord);
    o_Normal = float4(Normal, 1.0f);
    o_Position = float4(Pos, 1.0f);
    o_RoughnessMetallic = 1.0f.xxxx;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    TexCoord = stage_input.TexCoord;
    Normal = stage_input.Normal;
    Pos = stage_input.Pos;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.o_Albedo = o_Albedo;
    stage_output.o_Normal = o_Normal;
    stage_output.o_Position = o_Position;
    stage_output.o_RoughnessMetallic = o_RoughnessMetallic;
    return stage_output;
}
