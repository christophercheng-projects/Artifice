#pragma once

#include <Artifice.h>


// An atmosphere layer of width 'width', and whose density is defined as
//   'exp_term' * exp('exp_scale' * h) + 'linear_term' * h + 'constant_term',
// clamped to [0,1], and where h is the altitude.
struct DensityProfileLayer
{
    float width;
    float exp_term;
    float exp_scale;
    float linear_term;
    float constant_term;
};

// An atmosphere density profile made of several layers on top of each other
// (from bottom to top). The width of the last layer is ignored, i.e. it always
// extend to the top atmosphere boundary. The profile values vary between 0
// (null density) to 1 (maximum density).
struct DensityProfile
{
    DensityProfileLayer layers[2];
};

struct AtmosphereParameters
{
    // The solar irradiance at the top of the atmosphere.
    vec4 solar_irradiance = {1.474f, 1.850f, 1.91198f, 0.0f};
    // The scattering coefficient of air molecules at the altitude where their
    // density is maximum (usually the bottom of the atmosphere), as a function of
    // wavelength. The scattering coefficient at altitude h is equal to
    // 'rayleigh_scattering' times 'rayleigh_density' at this altitude.
    vec4 rayleigh_scattering = {0.005802f, 0.013558f, 0.033100f, 0.0f};
    // The scattering coefficient of aerosols at the altitude where their density
    // is maximum (usually the bottom of the atmosphere), as a function of
    // wavelength. The scattering coefficient at altitude h is equal to
    // 'mie_scattering' times 'mie_density' at this altitude.
    vec4 mie_scattering = {0.003996f, 0.003996f, 0.003996f, 0.0f};
    // The extinction coefficient of aerosols at the altitude where their density
    // is maximum (usually the bottom of the atmosphere), as a function of
    // wavelength. The extinction coefficient at altitude h is equal to
    // 'mie_extinction' times 'mie_density' at this altitude.
    vec4 mie_extinction = {0.004440f, 0.004440f, 0.004440f, 0.0f};
    // The average albedo of the ground.
    vec4 ground_albedo = {0.1f, 0.1f, 0.1f, 0.0f};
    // The extinction coefficient of molecules that absorb light (e.g. ozone) at
    // the altitude where their density is maximum, as a function of wavelength.
    // The extinction coefficient at altitude h is equal to
    // 'absorption_extinction' times 'absorption_density' at this altitude.
    vec4 absorption_extinction = {6.5e-4f, 1.881e-3f, 8.5e-5f, 0.0f};
    // The sun's angular radius. Warning: the implementation uses approximations
    // that are valid only if this angle is smaller than 0.1 radians.
    float sun_angular_radius = 0.004675f;
    // The distance between the planet center and the bottom of the atmosphere.
    float bottom_radius = 6360.0f;
    // The distance between the planet center and the top of the atmosphere.
    float top_radius = 6420.0f;
    // The asymetry parameter for the Cornette-Shanks phase function for the
    // aerosols.
    float mie_phase_function_g = 0.8f;
    // The cosine of the maximum Sun zenith angle for which atmospheric scattering
    // must be precomputed (for maximum precision, use the smallest Sun zenith
    // angle yielding negligible sky light radiance values. For instance, for the
    // Earth case, 102 degrees is a good choice - yielding mu_s_min = -0.2).
    float mu_s_min = -0.207912f;
    
    // Texture size parameters
    int transmittance_texture_mu_size = 256;
    int transmittance_texture_r_size = 64;
    int scattering_texture_r_size = 32;
    int scattering_texture_mu_size = 128;
    int scattering_texture_mu_s_size = 32;
    int scattering_texture_nu_size = 8;
    int irradiance_texture_mu_s_size = 64;
    int irradiance_texture_r_size = 16;

    // The density profile of air molecules, i.e. a function from altitude to
    // dimensionless values between 0 (null density) and 1 (maximum density).
    DensityProfile rayleigh_density = {{
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, -0.125f, 0.0f, 0.0f},
    }};
    // The density profile of aerosols, i.e. a function from altitude to
    // dimensionless values between 0 (null density) and 1 (maximum density).
    DensityProfile mie_density = {{
        {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, -0.833333f, 0.0f, 0.0f},
    }};
    // The density profile of air molecules that absorb light (e.g. ozone), i.e.
    // a function from altitude to dimensionless values between 0 (null density)
    // and 1 (maximum density).
    DensityProfile absorption_density = {{
        {25.0f, 0.0f, 0.0f, 0.066667f, -0.666667f},
        {0.0f, 0.0f, 0.0f, -0.066667f, 2.666667f},
    }};
};

struct AtmosphereTextures
{
    RenderHandle TransmittanceTexture;
    RenderHandle ScatteringTexture;
    RenderHandle IrradianceTexture;
    RenderHandle Sampler;
};

class Atmosphere
{
private:
    AtmosphereParameters m_Params;
    uint32 m_ScatteringOrder = 4;

    RenderGraph::Statistics m_PrecomputeStats;
    bool m_NeedsPrecompute = true;

private:
    ShaderLibrary m_ShaderLibrary;

    ShaderInstance m_SkyShaderInstance;
    ShaderSystem m_SkyShaderSystem;

    ShaderInstance m_TransmittanceShaderInstance;
    ShaderInstance m_DirectIrradianceShaderInstance;
    ShaderInstance m_SingleScatteringShaderInstance;
    ShaderInstance m_ScatteringDensityShaderInstance;
    ShaderInstance m_IndirectIrradianceShaderInstance;
    ShaderInstance m_MultipleScatteringShaderInstance;

    ShaderSystem m_TransmittanceShaderSystem;
    ShaderSystem m_DirectIrradianceShaderSystem;
    ShaderSystem m_SingleScatteringShaderSystem;
    ShaderSystem m_ScatteringDensityShaderSystem;
    ShaderSystem m_IndirectIrradianceShaderSystem;
    ShaderSystem m_MultipleScatteringShaderSystem;

    RenderHandle m_Sampler;

    RenderHandle m_TransmittanceTexture;

    RenderHandle m_DeltaIrradianceTexture;

    RenderHandle m_DeltaRayleighTexture;
    RenderHandle m_DeltaMieTexture;
    RenderHandle m_ScatteringTexture;
    RenderHandle m_ScatteringReadTexture;

    RenderHandle m_DeltaMultipleScatteringTexture;
    RenderHandle m_ScatteringDensityTexture;

    RenderHandle m_IrradianceTexture;
    RenderHandle m_IrradianceReadTexture;

public:
    void Init(AtmosphereParameters params = AtmosphereParameters(), uint32 order = 4);
    void Shutdown();
    void Update(AtmosphereParameters params = AtmosphereParameters(), uint32 order = 4);

    AtmosphereParameters GetParameters() const { return m_Params; }
    // Retrieve textures for use in rendering
    // All guaranteed to be in ResourceState::SampledFragment()
    AtmosphereTextures GetTextures() const
    {
        return {m_TransmittanceTexture, m_ScatteringTexture, m_IrradianceTexture, m_Sampler};
    }

private:
    void Precompute();
    void LoadShaders();
};