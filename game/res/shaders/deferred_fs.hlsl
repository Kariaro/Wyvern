Texture2D<float4> u_Normal : register(t1);
SamplerState _u_Normal_sampler : register(s1);
Texture2D<float4> u_Albedo : register(t0);
SamplerState _u_Albedo_sampler : register(s0);
Texture2D<float4> u_Position : register(t2);
SamplerState _u_Position_sampler : register(s2);
Texture2D<float4> u_RoughnessMetallic : register(t3);
SamplerState _u_RoughnessMetallic_sampler : register(s3);

static float2 TexCoord;
static float4 FragColor;

struct SPIRV_Cross_Input
{
    float2 TexCoord : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 FragColor : SV_Target0;
};

void frag_main()
{
    float over = (1.0f - floor(TexCoord.x)) * (1.0f - floor(TexCoord.y));
    float x = TexCoord.x * over;
    float y = TexCoord.y * over;
    float3 normal = u_Normal.Sample(_u_Normal_sampler, TexCoord).xyz;
    float shading = 1.0f;
    if (length(normal) > 0.100000001490116119384765625f)
    {
        float shadingDot = dot(normal, float3(0.57735025882720947265625f, 0.57735025882720947265625f, -0.57735025882720947265625f));
        shading = max(0.0f, (shadingDot * 0.5f) + 0.5f);
    }
    FragColor = float4(u_Albedo.Sample(_u_Albedo_sampler, TexCoord).xyz * shading, 1.0f);
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    TexCoord = stage_input.TexCoord;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.FragColor = FragColor;
    return stage_output;
}
