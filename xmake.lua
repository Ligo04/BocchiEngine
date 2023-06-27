set_project("Bocchi Engine")
set_xmakever("2.7.0")
set_version("0.0.1")

add_rules("plugin.vsxmake.autoupdate")
add_toolchains("clang")
add_rules("mode.release", "mode.debug")
option("enable_unity_build")
set_values(true, false)
set_default(true)
set_showmenu(true)
option_end()


if is_arch("x64", "x86_64", "arm64") then
	-- disable ccache in-case error
	set_policy("build.ccache", true)
	includes("xmake_func.lua")
    includes("thirdparty")
    
    target("BocchiEngine")
        set_group("BocchiEngine")
        -- set bin dir
        set_targetdir("bin")
        _config_project({
            project_kind = "binary",
            enable_exception = true,
            batch_size=8
        })
        add_deps("spdlog","glfw","imgui","nvrhi","vkm")
        add_includedirs("source")
        add_headerfiles("source/**.h")
        add_files("source/**.cpp")
    target_end()
end 
