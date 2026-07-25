workspace "Sloth"
    architecture "x86_64"
    startproject "SandboxDust"

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
IncludeDir["miniaudio"] = "vendor/miniaudio"
IncludeDir["GameNetworkingSockets"] = "vendor/vcpkg/vcpkg_installed/x64-windows-static-md/include"

VcpkgLibDir = {}
VcpkgLibDir["Release"] = "vendor/vcpkg/vcpkg_installed/x64-windows-static-md/lib"
VcpkgLibDir["Debug"] = "vendor/vcpkg/vcpkg_installed/x64-windows-static-md/debug/lib"

-- GameNetworkingSockets pulls in protobuf + abseil + OpenSSL as static libs;
-- glob them instead of hand-listing ~100 .lib names that shift with vcpkg updates.
-- libprotobuf-lite is excluded: GNS links the full libprotobuf, and having both
-- in the same link produces duplicate-symbol (LNK4006) warnings.
function LinkVcpkgLibs(libdir)
    local libs = {}
    for _, file in ipairs(os.matchfiles(libdir .. "/*.lib")) do
        local name = path.getbasename(file)
        if name ~= "libprotobuf-lite" and name ~= "libprotobuf-lited" then
            table.insert(libs, name)
        end
    end
    return libs
end

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
        "%{IncludeDir.stb}",
        "%{IncludeDir.miniaudio}",
        "%{IncludeDir.GameNetworkingSockets}"
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
        links { "opengl32.lib", "ws2_32.lib", "crypt32.lib", "winmm.lib", "Iphlpapi.lib", "user32.lib" }
        vectorextensions "AVX2"

    filter "configurations:Debug"
        defines { "SLOTH_DEBUG", "JPH_ENABLE_ASSERTS", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }
        runtime "Debug"
        symbols "On"
        optimize "Off"
        libdirs { VcpkgLibDir.Debug }
        links (LinkVcpkgLibs(VcpkgLibDir.Debug))

    filter "configurations:Release"
        defines { "SLOTH_RELEASE", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }
        runtime "Release"
        symbols "On"
        optimize "On"
        libdirs { VcpkgLibDir.Release }
        links (LinkVcpkgLibs(VcpkgLibDir.Release))

    filter "configurations:Dist"
        defines { "SLOTH_DIST" }
        runtime "Release"
        symbols "Off"
        optimize "Full"
        libdirs { VcpkgLibDir.Release }
        links (LinkVcpkgLibs(VcpkgLibDir.Release))

project "Dust"
    location "src/dust"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/dust/src/**.h",
        "src/dust/src/**.cpp"
    }

    includedirs
    {
        "src/dust/src",
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

project "Tower"
    location "src/tower"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/tower/src/**.h",
        "src/tower/src/**.cpp"
    }

    includedirs
    {
        "src/tower/src",
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

project "SandboxDust"
    location "src/sandbox/dust"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/sandbox/dust/src/**.h",
        "src/sandbox/dust/src/**.cpp"
    }

    includedirs
    {
        "src/dust/src",
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}"
    }

    links
    {
        "Dust",
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

project "SandboxTower"
    location "src/sandbox/tower"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/sandbox/tower/src/**.h",
        "src/sandbox/tower/src/**.cpp"
    }

    includedirs
    {
        "src/tower/src",
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}"
    }

    links
    {
        "Tower",
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

project "SandboxTowerServer"
    location "src/sandbox/towerserver"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/sandbox/towerserver/src/**.h",
        "src/sandbox/towerserver/src/**.cpp"
    }

    includedirs
    {
        "src/tower/src",
        "src/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.GLM}"
    }

    links
    {
        "Tower",
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
