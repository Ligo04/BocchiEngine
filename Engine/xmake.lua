
includes("3rdParty")

--add_requires("glfw")
UseUnityBuild=true 
BuildProject({
    projectName = "BocchiEngine",
    projectType = "binary",
    debugEvent = function()
        add_defines("_DEBUG")
        set_symbols("debug")
    end,
    releaseEvent = function()
        add_defines("NDEBUG")
        set_symbols("hidden")
        set_strip("all")
    end,
    exception = true,
    unityBuildBatch=8
})

set_group("Engine")
add_defines("UNICODE")
add_deps("spdlog","vulkan","vkm","glfw")
add_headerfiles("source/**.h")
add_files("source/**.cpp")
add_includedirs("source")
