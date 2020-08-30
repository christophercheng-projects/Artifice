#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in float v_TexIndex;
layout(location = 3) in float v_TilingFactor;

layout(set = 0, binding = 0) uniform sampler2D u_Textures[16];

layout(location = 0) out vec4 o_Color;

void main()
{
	vec4 tex_color = v_Color;
	switch(int(v_TexIndex))
	{
		case 0: tex_color *= texture(u_Textures[0], v_TexCoord * v_TilingFactor); break;
		case 1: tex_color *= texture(u_Textures[1], v_TexCoord * v_TilingFactor); break;
		case 2: tex_color *= texture(u_Textures[2], v_TexCoord * v_TilingFactor); break;
		case 3: tex_color *= texture(u_Textures[3], v_TexCoord * v_TilingFactor); break;
		case 4: tex_color *= texture(u_Textures[4], v_TexCoord * v_TilingFactor); break;
		case 5: tex_color *= texture(u_Textures[5], v_TexCoord * v_TilingFactor); break;
		case 6: tex_color *= texture(u_Textures[6], v_TexCoord * v_TilingFactor); break;
		case 7: tex_color *= texture(u_Textures[7], v_TexCoord * v_TilingFactor); break;
		case 8: tex_color *= texture(u_Textures[8], v_TexCoord * v_TilingFactor); break;
		case 9: tex_color *= texture(u_Textures[9], v_TexCoord * v_TilingFactor); break;
		case 10: tex_color *= texture(u_Textures[10], v_TexCoord * v_TilingFactor); break;
		case 11: tex_color *= texture(u_Textures[11], v_TexCoord * v_TilingFactor); break;
		case 12: tex_color *= texture(u_Textures[12], v_TexCoord * v_TilingFactor); break;
		case 13: tex_color *= texture(u_Textures[13], v_TexCoord * v_TilingFactor); break;
		case 14: tex_color *= texture(u_Textures[14], v_TexCoord * v_TilingFactor); break;
		case 15: tex_color *= texture(u_Textures[15], v_TexCoord * v_TilingFactor); break;
	}
	o_Color = tex_color;
}