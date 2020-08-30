#version 450
#extension GL_ARB_separate_shader_objects : enable


layout (location = 0) in vec2 v_TexCoords;


layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform PushConstant
{
    float Exposure;
} pc;


layout (location = 0) out vec4 o_Color;


void main()
{
    float gamma = 2.2;

    vec3 color = texture(u_Texture, v_TexCoords).rgb;
    color = vec3(1.0) - exp(-color * pc.Exposure);
    color = pow(color, vec3(1.0/gamma));  

    o_Color = vec4(color, 1.0);
}