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

group "Dependencies"
    include "vendor/GLFW"
    include "vendor/GLAD"
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
        "%{IncludeDir.GLAD}"
    }

    links
    {
        "GLFW",
        "GLAD"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "SLOTH_PLATFORM_WINDOWS" }
        links { "opengl32.lib" }

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
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}"
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
