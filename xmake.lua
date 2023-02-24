--define toolchains
toolchain("myClang")
    set_toolset("cc","clang")
    set_toolset("cxx","clang","clang++")
toolchain_end()


add_rules("mode.debug", "mode.release")


--add vulkan sdk
add_requires("glfw","vulkansdk")

add_requires("imgui","spdlog")

target("BocchiEngine")
    set_kind("binary")
    set_languages("cxx17")
    add_packages("glfw","vulkansdk")
    add_packages("imgui","spdlog")
    add_headerfiles("Engine/**.h")
    add_files("Engine/**.cpp")
    set_toolchains("myClang")

    --add hear search 
    add_includedirs("Engine")

