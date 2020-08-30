#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 v_Color;


layout(location = 0) out vec4 o_Color;

void main()
{
	o_Color = v_Color;
}