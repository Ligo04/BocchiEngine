
set_group("ThirdParty")
--spdlog
target("spdlog")
        _config_project({
                project_kind = "static",
                enable_exception = true,
                batch_size=8
        })
        add_defines("SPDLOG_COMPILED_LIB")
        add_includedirs("spdlog/include", {
                public = true
        })
        add_headerfiles("spdlog/include/**.h")
        add_files("spdlog/src/**.cpp")
target_end()


--vulkan
target("vulkan")
        _config_project({
                project_kind = "headeronly",
                enable_exception = true,
                batch_size=8
        })
        add_includedirs("VulkanSDK/include", {public = true})
        add_headerfiles("VulkanSDK/include//**.h","VulkanSDK/include//**.hpp")
        if(is_os("windows")) then 
                add_linkdirs("VulkanSDK/lib/Win32",{public=true})
                add_links("vulkan-1",{public=true})
        end 
target_end()


--vkm
target("vkm")
        _config_project({
                project_kind = "headeronly",
                enable_exception = true,
                batch_size=8
        })
        add_includedirs("vulkanmemoryallocator/include", {
	    public = true
        })
        add_headerfiles("vulkanmemoryallocator/include/**.h")
target_end()

--glfw
target("glfw")
        glfw_src_path="glfw/src/"
        _config_project({
                project_kind = "static",
                enable_exception = true,
                batch_size=8
        })
        add_includedirs("glfw/include/", {public = true})
        if(is_plat("windows")) then 
                add_defines("_GLFW_WIN32","UNICODE","UNICODE")
                add_headerfiles(glfw_src_path.."win32_platform.h",glfw_src_path.."win32_joystick.h",glfw_src_path.."internal.h",
                        glfw_src_path.."wgl_context.h", glfw_src_path.."egl_context.h", glfw_src_path.."osmesa_context.h",
                        glfw_src_path.."osmesa_context.h",glfw_src_path.."mappings.h")
                add_files(glfw_src_path.."win32_init.c",glfw_src_path.."win32_joystick.c",glfw_src_path.."win32_monitor.c",
                glfw_src_path.."win32_time.c",glfw_src_path.."win32_thread.c",glfw_src_path.."win32_window.c",
                glfw_src_path.."wgl_context.c",glfw_src_path.."egl_context.c",glfw_src_path.."osmesa_context.c",
                glfw_src_path.."window.c",glfw_src_path.."vulkan.c",glfw_src_path.."context.c",
                glfw_src_path.."init.c",glfw_src_path.."input.c",glfw_src_path.."monitor.c")
        end 

        if is_plat("macosx") then
                add_frameworks("Cocoa", "IOKit")
        elseif is_plat("windows") then
                add_syslinks("user32", "shell32", "gdi32")
        elseif is_plat("mingw") then
                add_syslinks("gdi32")
        elseif is_plat("linux") then
                -- TODO: add wayland support
                add_deps("libx11", "libxrandr", "libxrender", "libxinerama", "libxfixes", "libxcursor", "libxi", "libxext")
                add_syslinks("dl", "pthread")
                add_defines("_GLFW_X11")
        end
target_end()


--IMGUI
target("imgui")
        _config_project({
                project_kind = "static",
                enable_exception = true,
                batch_size=8
        })
        add_includedirs("imgui", {public = true})
        add_headerfiles("imgui/**.h")
        add_files("imgui/**.cpp")
        add_deps("glfw","vulkan")
target_end()
--dxc

target("dxc")
        _config_project({
                project_kind = "static",
                enable_exception = true,
                batch_size=8
        })
        add_includedirs("dxc/include", {public = true})
        add_headerfiles("dxc/**.h")
        add_linkdirs("dxc/lib/x64",{public=true})
        add_links("dxcompiler",{public=true})
target_end()

--nvrhi
target("nvrhi")
        _config_project({
                project_kind = "static",
                enable_exception = true,
                batch_size=8
        })
        add_deps("vulkan")
        add_includedirs("nvrhi/include", {public = true})
        add_headerfiles("nvrhi/**.h")
        add_files("nvrhi/**.cpp")
target_end()

