#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VertexOutput
{
    mat3 TBN;
    vec2 TexCoord;
    vec3 WorldPos;
    vec3 Normal;
    vec3 CameraPos;
} fs_input;


layout(set = 0, binding = 1) uniform UniformBuffer
{
    vec4 Positions[4];
    vec4 Colors[4];
} lights;

layout(set = 0, binding = 2) uniform samplerCube u_IrradianceMap;
layout(set = 0, binding = 3) uniform samplerCube u_PrefilterMap;
layout(set = 0, binding = 4) uniform sampler2D u_BRDFLUT;

layout(set = 1, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 1, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 1, binding = 2) uniform sampler2D u_MetalnessTexture;
layout(set = 1, binding = 3) uniform sampler2D u_RoughnessTexture;

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


layout(location = 0) out vec4 o_Color;


struct PBRParameters
{
	vec3 Albedo;
	vec3 Normal;
	float Metalness;
	float Roughness;

	vec3 View;
	float NdotV;
};
PBRParameters m_Params;

const float PI = 3.14159265359;
const float Epsilon = 0.0001;


float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
    float NdotH2 = clamp(NdotH * NdotH, 0.0, 1.0);

	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;

	return a2 / max(denom, Epsilon);
}

float GeometrySchlick(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	return NdotV / max(((1.0 - k) * NdotV + k), Epsilon);
}

float GeometrySmith(vec3 N, vec3 L, vec3 V, float roughness)
{
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);
	float ggx1 = GeometrySchlick(NdotL, roughness);
	float ggx2 = GeometrySchlick(NdotV, roughness);

	return ggx1 * ggx2;
}

vec3 FresnelSchlick(float HdotV, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

vec3 FresnelSchlickRoughness(float HdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - HdotV, 5.0);
}

void main()
{		
    m_Params.Albedo    = pc.AlbedoTexToggle ? texture(u_AlbedoTexture, fs_input.TexCoord).rgb : pc.Albedo.rgb;
	m_Params.Metalness = pc.MetalnessTexToggle ? texture(u_MetalnessTexture, fs_input.TexCoord).r : pc.Metalness;
	m_Params.Roughness = pc.RoughnessTexToggle ? texture(u_RoughnessTexture, fs_input.TexCoord).r : pc.Roughness;
	m_Params.Roughness = max(m_Params.Roughness, 0.05);
    
    m_Params.Normal = pc.NormalTexToggle ? normalize(texture(u_NormalTexture, fs_input.TexCoord).rgb * 2.0 - 1.0) : normalize(fs_input.Normal);

    m_Params.View = normalize(fs_input.CameraPos - fs_input.WorldPos);
    m_Params.NdotV = clamp(dot(m_Params.Normal, m_Params.View), 0.0, 1.0);
    
    vec3 R = reflect(-m_Params.View, m_Params.Normal); 


    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, m_Params.Albedo, m_Params.Metalness);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 light_pos = lights.Positions[i].xyz;

        vec3 L = normalize(light_pos - fs_input.WorldPos);
        vec3 H = normalize(m_Params.View + L);
        float NdotL = max(dot(m_Params.Normal, L), 0.0);

        float distance = length(light_pos - fs_input.WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lights.Colors[i].rgb * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(max(dot(m_Params.Normal, H), 0.0), m_Params.Roughness);   
        float G   = GeometrySmith(m_Params.Normal, L, m_Params.View, m_Params.Roughness);    
        vec3 F    = FresnelSchlick(clamp(dot(m_Params.View, H), 0.0, 1.0), F0);        
        
        vec3 nominator    = NDF * G * F;
        float denominator = max(4.0 * NdotL * m_Params.NdotV, Epsilon);
        vec3 specular = nominator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - m_Params.Metalness;	                
            
        // add to outgoing radiance Lo
        Lo += (kD * m_Params.Albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    // ambeint lighting

    vec3 F = FresnelSchlickRoughness(m_Params.NdotV, F0, m_Params.Roughness);
    
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - m_Params.Metalness);

    vec3 irradiance = texture(u_IrradianceMap, m_Params.Normal).rgb;
    vec3 diffuse      = irradiance * m_Params.Albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = textureQueryLevels(u_PrefilterMap) - 1.0;
    vec3 prefilteredColor = textureLod(u_PrefilterMap, R,  m_Params.Roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(u_BRDFLUT, vec2(m_Params.NdotV, m_Params.Roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = ambient + Lo;
#if 0
    // Tonemapping
    color = color / (color + vec3(1.0));
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));  
#endif
    o_Color = vec4(color, 1.0);
} 