luna_sdk_module_target("VariantUtils")
    add_headerfiles("*.hpp", {prefixdir = "Luna/VariantUtils"})
    add_headerfiles("Source/**.hpp", {install = false})
    add_files("Source/**.cpp")
    add_deps("Runtime")
target_end()