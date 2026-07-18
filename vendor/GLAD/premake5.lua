project "GLAD"
    kind "StaticLib"
    language "C"
    staticruntime "off"

    targetdir ("../../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../../bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "include/glad/gl.h",
        "include/KHR/khrplatform.h",
        "src/gl.c"
    }

    includedirs
    {
        "include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        runtime "Release"
        symbols "On"
        optimize "On"

    filter "configurations:Dist"
        runtime "Release"
        symbols "Off"
        optimize "Full"
