project "JoltPhysics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    exceptionhandling "Off"
    rtti "Off"

    targetdir ("../../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../../bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "Jolt/**.h",
        "Jolt/**.cpp",
        "Jolt/**.inl"
    }

    includedirs
    {
        "."
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

    filter "system:windows"
        systemversion "latest"
        vectorextensions "AVX2"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines { "JPH_ENABLE_ASSERTS", "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }

    filter "configurations:Release"
        runtime "Release"
        symbols "On"
        optimize "On"
        defines { "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED" }

    filter "configurations:Dist"
        runtime "Release"
        symbols "Off"
        optimize "Full"
