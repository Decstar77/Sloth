workspace "Sloth"
    architecture "x86_64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    multiprocessorcompile "On"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "vendor/GLFW/include"
IncludeDir["GLAD"] = "vendor/GLAD/include"
IncludeDir["GLM"] = "vendor"
IncludeDir["JoltPhysics"] = "vendor/JoltPhysics"
IncludeDir["stb"] = "vendor/stb"

group "Dependencies"
    include "vendor/GLFW"
    include "vendor/GLAD"
    include "vendor/JoltPhysics"
group ""

project "Engine"
    location "src/engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/engine/src/**.h",
        "src/engine/src/**.cpp"
    }

    includedirs
    {
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.JoltPhysics}",
        "%{IncludeDir.stb}"
    }

    defines
    {
        "JPH_USE_SSE4_1",
        "JPH_USE_SSE4_2",
        "JPH_USE_AVX",
        "JPH_USE_AVX2",
        "JPH_USE_LZCNT",
        "JPH_USE_TZCNT",
        "JPH_USE_F16C",
        "JPH_USE_FMADD",
        "JPH_OBJECT_STREAM"
    }

    links
    {
        "GLFW",
        "GLAD",
        "JoltPhysics"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "SLOTH_PLATFORM_WINDOWS" }
        links { "opengl32.lib" }
        vectorextensions "AVX2"

    filter "configurations:Debug"
        defines { "SLOTH_DEBUG", "JPH_ENABLE_ASSERTS", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "SLOTH_RELEASE", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }
        runtime "Release"
        symbols "On"
        optimize "On"

    filter "configurations:Dist"
        defines { "SLOTH_DIST" }
        runtime "Release"
        symbols "Off"
        optimize "Full"

project "Game"
    location "src/game"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/game/src/**.h",
        "src/game/src/**.cpp"
    }

    includedirs
    {
        "src/game/src",
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}",
        "%{IncludeDir.JoltPhysics}",
        "%{IncludeDir.stb}"
    }

    links
    {
        "Engine"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "SLOTH_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "SLOTH_DEBUG" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "SLOTH_RELEASE" }
        runtime "Release"
        symbols "On"
        optimize "On"

    filter "configurations:Dist"
        defines { "SLOTH_DIST" }
        runtime "Release"
        symbols "Off"
        optimize "Full"

project "Sandbox"
    location "src/sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/sandbox/src/**.h",
        "src/sandbox/src/**.cpp"
    }

    includedirs
    {
        "src/game/src",
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}"
    }

    links
    {
        "Game",
        "Engine"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "SLOTH_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "SLOTH_DEBUG" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "SLOTH_RELEASE" }
        runtime "Release"
        symbols "On"
        optimize "On"

    filter "configurations:Dist"
        defines { "SLOTH_DIST" }
        runtime "Release"
        symbols "Off"
        optimize "Full"
