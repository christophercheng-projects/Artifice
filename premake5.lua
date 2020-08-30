workspace "Artifice"
    architecture "x86_64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release"
    }

    flags
	{
		"MultiProcessorCompile"
	}

appname = "Sandbox"

MacDir = {}
MacDir["Workspace"] = "~/Development/Projects/Artifice"
MacDir["Editor_Debug"] = "bin/Debug-macosx-x86_64/Artifice-Editor/Artifice-Editor.app/Contents"
MacDir["Editor_Release"] = "bin/Release-macosx-x86_64/Artifice-Editor/Artifice-Editor.app/Contents"
MacDir["App_Debug"] = "bin/Debug-macosx-x86_64/" .. appname .. "/" .. appname .. ".app/Contents"
MacDir["App_Release"] = "bin/Release-macosx-x86_64/" .. appname .. "/" .. appname .. ".app/Contents"
MacDir["Artifice_Debug"] = "bin/Debug-macosx-x86_64/Artifice/libArtifice.dylib"
MacDir["Artifice_Release"] = "bin/Release-macosx-x86_64/Artifice/libArtifice.dylib"

MacDir["VulkanSDK"] = "../../Libraries/vulkansdk-macos-1.2.141.1/macOS"
MacDir["Assimp"] = "../../Libraries/assimp"


WinDir = {}
WinDir["Workspace"] = "C:/Development/Artifice"
WinDir["App_Debug"] = "bin/Debug-windows-x86_64/" .. appname
WinDir["App_Release"] = "bin/Release-windows-x86_64/" .. appname
WinDir["Editor_Debug"] = "bin/Debug-windows-x86_64/Artifice-Editor"
WinDir["Editor_Release"] = "bin/Release-windows-x86_64/Artifice-Editor"


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Artifice/Vendor/GLFW/include"
IncludeDir["ImGui"] = "Artifice/Vendor/imgui"
IncludeDir["stb_image"] = "Artifice/Vendor/stb_image"
if os.target() == "macosx" then
    IncludeDir["Vulkan"] = "%{MacDir.VulkanSDK}/include"
    IncludeDir["Assimp"] = "%{MacDir.Assimp}/include"
end
if os.target() == "windows" then
    IncludeDir["Vulkan"] = "C:/VulkanSDK/1.1.130.0/Include"
end


-- GLFW and imgui premake
include "Artifice/Vendor/GLFW"
include "Artifice/Vendor/imgui"

project "Artifice"
    location "Artifice"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "On"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/Source/**.h",
        "%{prj.name}/Source/**.cpp",

        -- Libraries compiled as part of Artifice
        "%{prj.name}/Vendor/stb_image/**.h",
        "%{prj.name}/Vendor/stb_image/**.cpp",
        -- "%{prj.name}/Vendor/stb_image_write/**.h",
        -- "%{prj.name}/Vendor/stb_image_write/**.cpp",
        -- "%{prj.name}/Vendor/tiny_obj_loader/**.cpp",
        -- "%{prj.name}/Vendor/tiny_obj_loader/**.h",
        -- "%{prj.name}/Vendor/vma/**.cpp",
        -- "%{prj.name}/Vendor/vma/**.h",
        -- "%{prj.name}/Vendor/spirv_reflect/**.c",
        -- "%{prj.name}/Vendor/spirv_reflect/**.h",
        -- "%{prj.name}/Vendor/meshoptimizer/**.cpp",
        -- "%{prj.name}/Vendor/meshoptimizer/**.h",
    }

    includedirs
    {
        "%{prj.name}/Source"
    }

    sysincludedirs
    {
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.Assimp}",
    }

    links
    {
        "GLFW",
        "ImGui",
    }

    filter "system:windows"
        links { "vulkan-1" }
        libdirs { "C:/VulkanSDK/1.1.130.0/Lib" }

    filter "system:macosx"
        -- If building as dynamic library:
        -- Artifice expects that Apps locate it at their (the App's) @rpath
        --xcodebuildsettings { ["LD_DYLIB_INSTALL_NAME"] = "@rpath/libArtifice.dylib" }

        -- Sandbox will search in {appname}.app/Contents/Frameworks for linked libraries
        --xcodebuildsettings { ["LD_RUNPATH_SEARCH_PATHS"] = "@rpath" }

        prelinkcommands
        {
            "{CHDIR} %{MacDir.Workspace}",
            "install_name_tool -id '@rpath/libvulkan.1.dylib' %{MacDir.VulkanSDK}/lib/libvulkan.1.dylib"
        }
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "AR_DEBUG"
        runtime "Debug"
        symbols "On"
        
        filter "configurations:Release"
        defines "AR_RELEASE"
        runtime "Release"
        optimize "On"


project "Artifice-Editor"
    location "Artifice-Editor"
    filter "system:macosx" kind "WindowedApp"
    filter "system:windows" kind "ConsoleApp"
    filter {}
    language "C++"
    cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.cpp"
	}

	sysincludedirs
	{
        "Artifice/Source",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.stb_image}",

    }
    links
    {
        "Artifice"
    }
    filter "system:macosx"
        libdirs
        {
            "%{MacDir.VulkanSDK}/lib",
            "%{MacDir.Assimp}/build/bin",
        }

        links
        {
            "assimp.5",
            "IrrXML",
            "vulkan.1",
            "Cocoa.framework",
            "IOKit.framework",
            "QuartzCore.framework",
        }

    filter "system:windows"
        debugdir "$(OutDir)"
        systemversion "latest"

        defines
        {
            "AR_PLATFORM_WINDOWS"
        }

	filter "system:macosx"

		defines
		{
            "AR_PLATFORM_MAC"
        }

        -- Use our own pre-made Info.plist file
        xcodebuildsettings { ["INFOPLIST_FILE"] = "Info.plist" }

        -- Artifice-Editor will search in Artifice-Editor.app/Contents/Frameworks for linked libraries
        xcodebuildsettings { ["LD_RUNPATH_SEARCH_PATHS"] = "@executable_path/../Frameworks" }

        -- Make sure code signing if off
        xcodebuildsettings { ["CODE_SIGN_IDENTITY"] = "" }

    filter { "system:macosx", "configurations:Debug" }
        prebuildcommands
        {
            "{CHDIR} %{MacDir.Workspace}",

            -- Create Frameworks and Resources directories at Artifice-Editor.app/Contents
            "{MKDIR} %{MacDir.Editor_Debug}/Frameworks",
            "{MKDIR} %{MacDir.Editor_Debug}/Resources",
            
            -- If building artifice as dynamic library
            -- Copy libArtifice.dylib to Artifice-Editor.app/Contents/Frameworks
            --"{COPY}  %{MacDir.Artifice_Debug} %{MacDir.Editor_Debug}/Frameworks",

            -- Copy Assimp dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.Assimp}/build/bin/libassimp.5.dylib %{MacDir.Editor_Debug}/Frameworks",
            "{COPY}  %{MacDir.Assimp}/build/bin/libIrrXML.dylib %{MacDir.Editor_Debug}/Frameworks",

            -- Copy Vulkan dylib to Artifice-Editor.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libvulkan.1.dylib %{MacDir.Editor_Debug}/Frameworks",
            -- Copy Vulkan folder (icd and layer information) to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice-Editor/Resources/vulkan %{MacDir.Editor_Debug}/Resources",

            -- Copy libMoltenVK.dylib to Artifice-Editor.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libMoltenVK.dylib %{MacDir.Editor_Debug}/Frameworks",
            
            -- Copy validation layers to Artifice-Editor.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libVkLayer_khronos_validation.dylib %{MacDir.Editor_Debug}/Frameworks",


            -- Copy Shaders folder (internal engine shader files) to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{MacDir.Editor_Debug}/Resources",
            -- Copy Textures folder (internal engine texture files) to Artifice-Editor.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{MacDir.Editor_Debug}/Resources",
            -- Copy Models folder (internal engine model files) to Artifice-Editor.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{MacDir.Editor_Debug}/Resources",
            
            -- Copy engine fonts
            "{COPY}  Artifice/Resources/Fonts %{MacDir.Editor_Debug}/Resources",

            -- Copy Editor assets folder to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice-Editor/Assets %{MacDir.Editor_Debug}/Resources",

        }

    filter { "system:macosx", "configurations:Release" }
        
        xcodebuildsettings { ["GCC_FAST_MATH"] = "YES" }

        prelinkcommands
        {
            "{CHDIR} %{MacDir.Workspace}",

            -- Create Frameworks and Resources directories at Artifice-Editor.app/Contents
            "{MKDIR} %{MacDir.Editor_Release}/Frameworks",
            "{MKDIR} %{MacDir.Editor_Release}/Resources",

            -- Copy libArtifice.dylib to Artifice-Editor.app/Contents/Frameworks
            --"{COPY}  %{MacDir.Artifice_Release} %{MacDir.Editor_Release}/Frameworks",

            -- Copy Assimp dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.Assimp}/build/bin/libassimp.5.dylib %{MacDir.Editor_Release}/Frameworks",
            "{COPY}  %{MacDir.Assimp}/build/bin/libIrrXML.dylib %{MacDir.Editor_Release}/Frameworks",

            -- Copy Vulkan dylib to Artifice-Editor.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libvulkan.1.dylib %{MacDir.Editor_Release}/Frameworks",

            -- Copy Vulkan folder (icd and layer information) to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice-Editor/Resources/vulkan %{MacDir.Editor_Release}/Resources",

            -- Copy Shaders folder (internal engine shader files) to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{MacDir.Editor_Release}/Resources",
            -- Copy Textures folder (internal engine texture files) to Artifice-Editor.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{MacDir.Editor_Release}/Resources",
            -- Copy Models folder (internal engine model files) to Artifice-Editor.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{MacDir.Editor_Release}/Resources",

            -- Copy engine fonts
            "{COPY}  Artifice/Resources/Fonts %{MacDir.Editor_Release}/Resources",

            -- Copy libMoltenVK.dylib to Artifice-Editor.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libMoltenVK.dylib %{MacDir.Editor_Release}/Frameworks",

            -- No validation layers in release mode

            -- Copy Editor assets folder to Artifice-Editor.app/Contents/Resources
            "{COPY}  Artifice-Editor/Assets %{MacDir.Editor_Release}/Resources",
        }


    filter { "system:windows", "configurations:Debug" }
        prelinkcommands
        {
            "{CHDIR} %{WinDir.Workspace}",

            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{WinDir.Editor_Debug}/Shaders",

            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{WinDir.Editor_Debug}/Textures",

            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{WinDir.Editor_Debug}/Models",
        }

    filter { "system:windows", "configurations:Release" }
        prelinkcommands
        {
            "{CHDIR} %{WinDir.Workspace}",

            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{WinDir.Editor_Release}/Shaders",

            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{WinDir.Editor_Release}/Textures",

            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{WinDir.Editor_Release}/Models",
        }


	filter "configurations:Debug"
		defines "AR_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "AR_RELEASE"
		runtime "Release"
		optimize "On"

        
project "Sandbox"
    location "Sandbox"
    filter "system:macosx" kind "WindowedApp"
    filter "system:windows" kind "ConsoleApp"
    filter {}
    language "C++"
    cppdialect "C++17"
	staticruntime "On"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/Source/**.h",
		"%{prj.name}/Source/**.cpp"
	}

	sysincludedirs
	{
        "Artifice/Source",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.Vulkan}",
        "%{IncludeDir.stb_image}",

    }
    links
    {
        "Artifice"
    }
    filter "system:macosx"
        libdirs
        {
            "%{MacDir.Assimp}/build/bin",
            "%{MacDir.VulkanSDK}/lib",
        }

        links
        {
            "assimp.5",
            "IrrXML",
            "vulkan.1",
            "Cocoa.framework",
            "IOKit.framework",
            "QuartzCore.framework",
        }

    filter "system:windows"
        debugdir "$(OutDir)"
        systemversion "latest"

        defines
        {
            "AR_PLATFORM_WINDOWS"
        }

	filter "system:macosx"

		defines
		{
            "AR_PLATFORM_MAC"
        }

        -- Use our own pre-made Info.plist file
        xcodebuildsettings { ["INFOPLIST_FILE"] = "Info.plist" }

        -- Sandbox will search in Sandbox.app/Contents/Frameworks for linked libraries
        xcodebuildsettings { ["LD_RUNPATH_SEARCH_PATHS"] = "@executable_path/../Frameworks" }

        -- Make sure code signing if off
        xcodebuildsettings { ["CODE_SIGN_IDENTITY"] = "" }

    filter { "system:macosx", "configurations:Debug" }
        prebuildcommands
        {
            "{CHDIR} %{MacDir.Workspace}",

            -- Create Frameworks and Resources directories at Sandbox.app/Contents
            "{MKDIR} %{MacDir.App_Debug}/Frameworks",
            "{MKDIR} %{MacDir.App_Debug}/Resources",
            
            -- If building artifice as dynamic library
            -- Copy libArtifice.dylib to Sandbox.app/Contents/Frameworks
            --"{COPY}  %{MacDir.Artifice_Debug} %{MacDir.App_Debug}/Frameworks",

            -- Copy Assimp dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.Assimp}/build/bin/libassimp.5.dylib %{MacDir.App_Debug}/Frameworks",
            "{COPY}  %{MacDir.Assimp}/build/bin/libIrrXML.dylib %{MacDir.App_Debug}/Frameworks",
            -- Copy Vulkan dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libvulkan.1.dylib %{MacDir.App_Debug}/Frameworks",
            -- Copy Vulkan folder (icd and layer information) to Sandbox.app/Contents/Resources
            "{COPY}  Sandbox/Resources/vulkan %{MacDir.App_Debug}/Resources",

            -- Copy libMoltenVK.dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libMoltenVK.dylib %{MacDir.App_Debug}/Frameworks",
            
            -- Copy validation layers to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libVkLayer_khronos_validation.dylib %{MacDir.App_Debug}/Frameworks",


            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{MacDir.App_Debug}/Resources",
            -- Copy engine fonts
            "{COPY}  Artifice/Resources/Fonts %{MacDir.App_Debug}/Resources",
            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{MacDir.App_Debug}/Resources",
            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{MacDir.App_Debug}/Resources",
            
            -- Copy Sandbox assets folder to Sandbox.app/Contents/Resources
            "{COPY}  Sandbox/Assets %{MacDir.App_Debug}/Resources",
        }

    filter { "system:macosx", "configurations:Release" }
        
        xcodebuildsettings { ["GCC_FAST_MATH"] = "YES" }

        prelinkcommands
        {
            "{CHDIR} %{MacDir.Workspace}",

            -- Create Frameworks and Resources directories at Sandbox.app/Contents
            "{MKDIR} %{MacDir.App_Release}/Frameworks",
            "{MKDIR} %{MacDir.App_Release}/Resources",


            -- Copy libArtifice.dylib to Sandbox.app/Contents/Frameworks
            --"{COPY}  %{MacDir.Artifice_Release} %{MacDir.App_Release}/Frameworks",

            -- Copy Assimp dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.Assimp}/build/bin/libassimp.5.dylib %{MacDir.App_Release}/Frameworks",
            "{COPY}  %{MacDir.Assimp}/build/bin/libIrrXML.dylib %{MacDir.App_Release}/Frameworks",
            -- Copy Vulkan dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libvulkan.1.dylib %{MacDir.App_Release}/Frameworks",

            -- Copy Vulkan folder (icd and layer information) to Sandbox.app/Contents/Resources
            "{COPY}  Sandbox/Resources/vulkan %{MacDir.App_Release}/Resources",

            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{MacDir.App_Release}/Resources",
            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{MacDir.App_Release}/Resources",
            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{MacDir.App_Release}/Resources",
            -- Copy engine fonts
            "{COPY}  Artifice/Resources/Fonts %{MacDir.App_Release}/Resources",


            -- Copy libMoltenVK.dylib to Sandbox.app/Contents/Frameworks
            "{COPY}  %{MacDir.VulkanSDK}/lib/libMoltenVK.dylib %{MacDir.App_Release}/Frameworks",

            -- No validation layers in release mode

            -- Copy Sandbox texture assets folder to Sandbox.app/Contents/Resources
            "{COPY}  Sandbox/Assets %{MacDir.App_Release}/Resources",
        }


    filter { "system:windows", "configurations:Debug" }
        prelinkcommands
        {
            "{CHDIR} %{WinDir.Workspace}",

            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{WinDir.App_Debug}/Shaders",

            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{WinDir.App_Debug}/Textures",

            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{WinDir.App_Debug}/Models",
        }

    filter { "system:windows", "configurations:Release" }
        prelinkcommands
        {
            "{CHDIR} %{WinDir.Workspace}",

            -- Copy Shaders folder (internal engine shader files) to Sandbox.app/Contents/Resources
            "{COPY}  Artifice/Resources/Shaders %{WinDir.App_Release}/Shaders",

            -- Copy Textures folder (internal engine texture files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Textures %{WinDir.App_Release}/Textures",

            -- Copy Models folder (internal engine model files) to Sandbox.app/Contents/Resources
            --"{COPY}  Artifice/Resources/Models %{WinDir.App_Release}/Models",
        }


	filter "configurations:Debug"
		defines "AR_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines "AR_RELEASE"
		runtime "Release"
		optimize "On"
