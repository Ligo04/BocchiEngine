set_project("Bocchi Engine")
set_xmakever("2.7.0")
set_version("0.0.1")

add_rules("plugin.vsxmake.autoupdate")
add_toolchains("clang")
add_rules("mode.release", "mode.debug")

option("enable_unity_build")
set_values(true, false)
set_default(false)
set_showmenu(true)
option_end()

includes("shader_compiler.lua")
includes("xmake_func.lua")
includes("thirdparty")

if is_arch("x64", "x86_64", "arm64") then
	-- disable ccache in-case error
	set_policy("build.ccache", true)
    target("BocchiEngine")
        set_group("BocchiEngine")
        -- set bin dir
        set_targetdir("bin")
        _config_project({
            project_kind = "binary",
            enable_exception = true,
            --batch_size=8
        })
        --add defines
        add_defines("USE_VK")
        add_deps("spdlog","glfw","imgui","nvrhi","UGM")
        add_includedirs("source")
        add_headerfiles("source/**.h|source/**.hpp")
        add_files("source/**.cpp")
        add_headerfiles("source/shaders/**.hlsl")
        add_files("source/shaders/**.hlsl",{rule="hlsl_shader_complier"})
    target_end()
end 
