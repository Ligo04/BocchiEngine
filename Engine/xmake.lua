
includes("3rdParty")

--add_requires("glfw")
UseUnityBuild=true 
BuildProject({
    projectName = "BocchiEngine",
    projectType = "binary",
    debugEvent = function()
        add_defines("_DEBUG")
    end,
    releaseEvent = function()
        add_defines("NDEBUG")
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
