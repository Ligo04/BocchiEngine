rule("hlsl_shader_complier")
    set_extensions(".hlsl",".hlsli")
    on_buildcmd_files(function (target,batchcmds,sourcebatch, opt)
		import("lib.detect.find_program")
		dxc = find_program("dxc", fxcopt)
		
		spv_generated_dir=path.join(os.projectdir(),"bin\\shaders\\spirv")
		if not os.isdir(spv_generated_dir) then
			os.mkdir(spv_generated_dir)
		end
		
        for _,sourcefile_hlsl in ipairs(sourcebatch.sourcefiles) do 
            local ext = path.extension(sourcefile_hlsl)
            if ext==".hlsl" then 
                -- vulkan spriv
                local hlsl_basename = path.basename(sourcefile_hlsl)
                local hlsl_shader_type = string.sub(hlsl_basename,-2)
                local spv_output_path = path.join(spv_generated_dir,hlsl_basename..".spv")
				batchcmds:show_progress(opt.progress, "${color.build.object}compiling.hlsl %s", sourcefile_hlsl)
                if is_mode("debug") then  
					batchcmds:vrunv(dxc,{"-spirv","-T",hlsl_shader_type.."_6_3","-E","main","-fspv-debug=vulkan-with-source","-fspv-extension=SPV_KHR_non_semantic_info","-fspv-target-env=vulkan1.3",
                    "-fspv-extension=SPV_KHR_ray_tracing","-fspv-extension=SPV_KHR_multiview","-fspv-extension=SPV_KHR_shader_draw_parameters","-fspv-extension=SPV_EXT_descriptor_indexing",
                    sourcefile_hlsl,"-Fo ",spv_output_path})
                else 
					batchcmds:vrunv(dxc,{"-spirv","-T",hlsl_shader_type.."_6_3","-E","main"," -fspv-target-env=vulkan1.3",
                    "-fspv-extension=SPV_KHR_ray_tracing","-fspv-extension=SPV_KHR_multiview","-fspv-extension=SPV_KHR_shader_draw_parameters","-fspv-extension=SPV_EXT_descriptor_indexing",
                    sourcefile_hlsl,"-Fo ",spv_output_path})
                end 
            end
        end 
    end)
    
rule_end()