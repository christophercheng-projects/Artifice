#version 450 core
#extension GL_ARB_separate_shader_objects : enable


layout(push_constant) uniform PushConstant
{
    mat4 InverseViewProjection;
    float TextureLOD;
} pc;


layout(location = 0) out vec3 v_TexCoords;


void main()
{
	gl_Position = vec4(vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0, 0.0, 1.0);
	v_TexCoords = (pc.InverseViewProjection * gl_Position).xyz;
}