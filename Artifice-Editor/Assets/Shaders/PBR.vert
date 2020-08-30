#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;


layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 ViewProjection;
    vec3 CameraPos;
} ubo;

layout(push_constant) uniform PushConstant
{
    mat4 Model;
    vec4 Albedo;
    float Metallic;
    float Roughness;
    float AO;
} pc;


layout(location = 0) out VertexOutput
{
    vec2 TexCoord;
    vec3 WorldPos;
    vec3 Normal;
    vec3 CameraPos;
} vs_out;


void main()
{
    vs_out.TexCoord = a_TexCoord;
    vs_out.WorldPos = vec3(pc.Model * vec4(a_Position, 1.0));
    vs_out.Normal = mat3(pc.Model) * a_Normal;  
    vs_out.CameraPos = ubo.CameraPos; 

    gl_Position =  ubo.ViewProjection * vec4(vs_out.WorldPos, 1.0);
}