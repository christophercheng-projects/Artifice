PROGRAM=/Users/christophercheng/Development/Libraries/vulkansdk-macos-1.1.130.0/macOS/bin/glslangValidator

    cd Artifice/Resources/Shaders
    # Clean compiled shaders
    rm -f *.spv
    # Compile here
    $PROGRAM -V -o fullscreen.vert.spv fullscreen.vert
    $PROGRAM -V -o fullscreen.frag.spv fullscreen.frag

    $PROGRAM -V -o triangle.vert.spv triangle.vert
    $PROGRAM -V -o triangle.frag.spv triangle.frag

    $PROGRAM -V -o ImGui.vert.spv ImGui.vert
    $PROGRAM -V -o ImGui.frag.spv ImGui.frag

    $PROGRAM -V -o Renderer2D.vert.spv Renderer2D.vert
    $PROGRAM -V -o Renderer2D.frag.spv Renderer2D.frag

    $PROGRAM -V -o Line.vert.spv Line.vert
    $PROGRAM -V -o Line.frag.spv Line.frag
    
    $PROGRAM -V -o Transmittance.comp.spv Transmittance.comp
    $PROGRAM -V -o Scattering.comp.spv Scattering.comp


    cd ../../../Sandbox/Assets/Shaders
    # Clean compiled shaders
    rm -f *.spv
    # Compile here

    $PROGRAM -V -o PBR.vert.spv PBR.vert
    $PROGRAM -V -o PBR.frag.spv PBR.frag

    $PROGRAM -V -o Skybox.vert.spv Skybox.vert
    $PROGRAM -V -o Skybox.frag.spv Skybox.frag
    
    $PROGRAM -V -o EquirectangularToCubemap.comp.spv EquirectangularToCubemap.comp
    $PROGRAM -V -o EnvironmentIrradiance.comp.spv EnvironmentIrradiance.comp
    $PROGRAM -V -o EnvironmentPrefilter.comp.spv EnvironmentPrefilter.comp
    $PROGRAM -V -o BRDF.comp.spv BRDF.comp
    
    $PROGRAM -V -o PBR_Static.vert.spv PBR_Static.vert
    $PROGRAM -V -o PBR_Static.frag.spv PBR_Static.frag

    $PROGRAM -V -o PBR_Animated.vert.spv PBR_Animated.vert
    $PROGRAM -V -o PBR_Animated.frag.spv PBR_Animated.frag

    $PROGRAM -V -o Composite.vert.spv Composite.vert
    $PROGRAM -V -o Composite.frag.spv Composite.frag


    $PROGRAM -V -o Etna.comp.spv Etna.comp
    $PROGRAM -V -o Digits.comp.spv Digits.comp

    cd Atmosphere
    # Clean compiled shaders
    rm -f *.spv
    # Compile here

    $PROGRAM -V -o Transmittance.comp.spv Transmittance.comp
    $PROGRAM -V -o DirectIrradiance.comp.spv DirectIrradiance.comp
    $PROGRAM -V -o SingleScattering.comp.spv SingleScattering.comp
    $PROGRAM -V -o ScatteringDensity.comp.spv ScatteringDensity.comp
    $PROGRAM -V -o IndirectIrradiance.comp.spv IndirectIrradiance.comp
    $PROGRAM -V -o MultipleScattering.comp.spv MultipleScattering.comp
    
    $PROGRAM -V -o Sky.vert.spv Sky.vert
    $PROGRAM -V -o Sky.frag.spv Sky.frag
    
    $PROGRAM -V -o Cubemap.comp.spv Cubemap.comp