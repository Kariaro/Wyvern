#if GL_ES 
precision mediump float;
#endif

layout(binding = 0) uniform sampler2D u_Albedo;

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec3 Pos;

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_Normal;
layout(location = 2) out vec4 o_Position;
layout(location = 3) out vec4 o_RoughnessMetallic;

void main()
{
    o_Albedo = texture( u_Albedo, TexCoord );
    o_Normal = vec4( 0.0 );
    o_Position = vec4( Pos, 1.0 );
    o_RoughnessMetallic = vec4( 1.0 );
}