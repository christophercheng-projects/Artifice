#version 450 core
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 v_TexCoords;


layout(set = 0, binding = 0) uniform samplerCube u_Texture;

layout(push_constant) uniform PushConstant
{
    mat4 InverseViewProjection;
    float TextureLod;
} pc;


layout(location = 0) out vec4 o_Color;


void main()
{
	vec3 color = textureLod(u_Texture, v_TexCoords, pc.TextureLod).rgb;
#if 0
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
#endif
    o_Color = vec4(color, 1.0);
}