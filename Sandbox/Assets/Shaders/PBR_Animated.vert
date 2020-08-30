#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;


layout(set = 0, binding = 0) uniform UniformBufferGlobal
{
    mat4 ViewProjection;
    mat4 View;
    vec3 CameraPos;
} ubo;

layout(set = 2, binding = 0) uniform UniformBufferDynamicMesh
{
    mat4 BoneTransform[100];
} ubo_per_mesh;

layout(set = 3, binding = 0) uniform UniformBufferDynamicSubmesh
{
    mat4 Transform;
    mat4 NormalTransform;
} ubo_per_submesh;

layout(push_constant) uniform PushConstant
{
    vec4 Albedo;
    float Metalness;
    float Roughness;
    
    bool AlbedoTexToggle;
    bool NormalTexToggle;
    bool MetalnessTexToggle;
    bool RoughnessTexToggle;
} pc;


layout(location = 0) out VertexOutput
{
    mat3 TBN;
    vec2 TexCoord;
    vec3 WorldPos;
    vec3 Normal;
    vec3 CameraPos;
} vs_out;


void main()
{
    mat4 bone_transform = ubo_per_mesh.BoneTransform[a_BoneIndices[0]] * a_BoneWeights[0];
	bone_transform += ubo_per_mesh.BoneTransform[a_BoneIndices[1]] * a_BoneWeights[1];
	bone_transform += ubo_per_mesh.BoneTransform[a_BoneIndices[2]] * a_BoneWeights[2];
	bone_transform += ubo_per_mesh.BoneTransform[a_BoneIndices[3]] * a_BoneWeights[3];

    vec4 world_pos = ubo_per_submesh.Transform * bone_transform * vec4(a_Position, 1.0);
    vec3 N = a_Normal;

    mat3 normal_transform = mat3(ubo_per_submesh.NormalTransform);

    vs_out.Normal = normal_transform * mat3(bone_transform) * N;  
    vs_out.TexCoord = a_TexCoord;

    if (pc.NormalTexToggle)
    {
        mat3 transform = normal_transform * mat3(bone_transform);
        
        vec3 T = normalize(transform * a_Tangent);
        N = normalize(transform * N);
        T = normalize(T - dot(T, N) * N);
        
        vec3 B = cross(N, T);

        mat3 TBN = transpose(mat3(T, B, N));
        vs_out.TBN = TBN;
        vs_out.WorldPos = TBN * vec3(world_pos);
        vs_out.CameraPos = TBN * ubo.CameraPos;
    }
    else
    {
        vs_out.WorldPos = vec3(world_pos);
        vs_out.CameraPos = ubo.CameraPos;
    }

    gl_Position =  ubo.ViewProjection * world_pos;
}