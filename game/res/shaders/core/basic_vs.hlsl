cbuffer UbInstanceData : register(b1)
{
    row_major float4x4 _20_u_Projection : packoffset(c0);
    row_major float4x4 _20_u_View : packoffset(c4);
    row_major float4x4 _20_u_Model : packoffset(c8);
};


static float4 gl_Position;
static float2 TexCoord;
static float2 a_TexCoord0;
static float3 Normal;
static float3 a_Normal;
static float3 Pos;
static float3 a_Pos;
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
    float2 TexCoord : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 Pos : TEXCOORD2;
    float4 gl_Position : SV_Position;
};

// Returns the determinant of a 2x2 matrix.
float SPIRV_Cross_Det2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Returns the inverse of a matrix, by using the algorithm of calculating the classical
// adjoint and dividing by the determinant. The contents of the matrix are changed.
float3x3 SPIRV_Cross_Inverse(float3x3 m)
{
    float3x3 adj;	// The adjoint matrix (inverse after dividing by determinant)

    // Create the transpose of the cofactors, as the classical adjoint of the matrix.
    adj[0][0] =  SPIRV_Cross_Det2x2(m[1][1], m[1][2], m[2][1], m[2][2]);
    adj[0][1] = -SPIRV_Cross_Det2x2(m[0][1], m[0][2], m[2][1], m[2][2]);
    adj[0][2] =  SPIRV_Cross_Det2x2(m[0][1], m[0][2], m[1][1], m[1][2]);

    adj[1][0] = -SPIRV_Cross_Det2x2(m[1][0], m[1][2], m[2][0], m[2][2]);
    adj[1][1] =  SPIRV_Cross_Det2x2(m[0][0], m[0][2], m[2][0], m[2][2]);
    adj[1][2] = -SPIRV_Cross_Det2x2(m[0][0], m[0][2], m[1][0], m[1][2]);

    adj[2][0] =  SPIRV_Cross_Det2x2(m[1][0], m[1][1], m[2][0], m[2][1]);
    adj[2][1] = -SPIRV_Cross_Det2x2(m[0][0], m[0][1], m[2][0], m[2][1]);
    adj[2][2] =  SPIRV_Cross_Det2x2(m[0][0], m[0][1], m[1][0], m[1][1]);

    // Calculate the determinant as a combination of the cofactors of the first row.
    float det = (adj[0][0] * m[0][0]) + (adj[0][1] * m[1][0]) + (adj[0][2] * m[2][0]);

    // Divide the classical adjoint matrix by the determinant.
    // If determinant is zero, matrix is not invertable, so leave it unchanged.
    return (det != 0.0f) ? (adj * (1.0f / det)) : m;
}

void vert_main()
{
    TexCoord = a_TexCoord0;
    Normal = normalize(mul(a_Normal, transpose(SPIRV_Cross_Inverse(float3x3(_20_u_Model[0].xyz, _20_u_Model[1].xyz, _20_u_Model[2].xyz)))));
    Pos = a_Pos;
    gl_Position = mul(float4(a_Pos, 1.0f), mul(_20_u_Model, mul(_20_u_View, _20_u_Projection)));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    a_TexCoord0 = stage_input.a_TexCoord0;
    a_Normal = stage_input.a_Normal;
    a_Pos = stage_input.a_Pos;
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
