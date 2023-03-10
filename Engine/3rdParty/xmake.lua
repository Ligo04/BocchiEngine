
--spdlog
BuildProject({
        projectName = "spdlog",
        projectType = "static",
        exception = true,
        UseUnityBuild=true,
        unityBuildBatch=32
})
set_group("ThirdParty")
add_includedirs("spdlog/include", {
	    public = true
})
add_headerfiles("spdlog/include/**.h")
add_files("spdlog/src/**.cpp")
add_defines("SPDLOG_COMPILED_LIB")

--vulkan
BuildProject({
        projectName = "vulkan",
        projectType = "headeronly",
        exception = true,
        UseUnityBuild=true,
        unityBuildBatch=32
})
set_group("ThirdParty")
add_includedirs("VulkanSDK/include", {public = true})
if(is_os("windows")) then 
        add_linkdirs("VulkanSDK/lib/Win32",{public=true})
        add_links("vulkan-1",{public=true})
end 

--vkm
BuildProject({
        projectName = "vkm",
        projectType = "headeronly",
        exception = true,
        UseUnityBuild=true,
        unityBuildBatch=32
})
set_group("ThirdParty")
add_includedirs("vulkanmemoryallocator/include", {
	    public = true
})
add_headerfiles("vulkanmemoryallocator/include/**.h")

--glfw

glfw_src_path="glfw/src/"
BuildProject({
        projectName = "glfw",
        projectType = "static",
        exception = true,
        UseUnityBuild=true,
        unityBuildBatch=32
})
set_group("ThirdParty")
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


--IMGUI
BuildProject({
        projectName = "imgui",
        projectType = "static",
        exception = true,
        UseUnityBuild=true,
        unityBuildBatch=32
})
set_group("ThirdParty")
add_includedirs("imgui", {public = true})
add_headerfiles("imgui/**.h")
add_files("imgui/**.cpp")
add_deps("glfw","vulkan")

