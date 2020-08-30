#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;


layout(push_constant) uniform PushConstant
{ 
	mat4 ViewProjection;
} pc;


layout(location = 0) out vec4 v_Color;

void main()
{
	v_Color = a_Color;
	gl_Position = pc.ViewProjection * vec4(a_Position, 1.0);
}