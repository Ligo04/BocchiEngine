rule("engine_basic_settings")
on_config(function(target)
	local _, cc = target:tool("cxx")
	if is_plat("linux") then
		-- Linux should use -stdlib=libc++
		-- https://github.com/LuisaGroup/LuisaCompute/issues/58
		if (cc == "clang" or cc == "clangxx" or cc == "gcc" or cc == "gxx") then
			target:add("cxflags", "-stdlib=libc++", {
				force = true
			})
		end
	end
end)
on_load(function(target)
	local _get_or = function(name, default_value)
		local v = target:values(name)
		if v == nil then
			return default_value
		end
		return v
	end
	local project_kind = _get_or("project_kind", "phony")
	target:set("kind", project_kind)
	local c_standard = target:values("c_standard")
	local cxx_standard = target:values("cxx_standard")
	if type(c_standard) == "string" and type(cxx_standard) == "string" then
		target:set("languages", c_standard, cxx_standard)
	else
		target:set("languages", "clatest", "cxx20")
	end

	local enable_exception = _get_or("enable_exception", nil)
	if enable_exception then
		target:set("exceptions", "cxx")
	else
		target:set("exceptions", "no-cxx")
	end
	if is_mode("debug") then
		-- target:add("defines", "DEBUG","_DEBUG")
		-- target:set("symbols", "debug")
		target:set("runtimes", "MDd")
		target:set("optimize", "none")
		target:set("warnings", "none")
		target:add("cxflags", "/GS", "/Gd", {
			tools = {"clang_cl", "cl"}
		})
		target:add("cxflags", "/Zc:preprocessor", {
			tools = "cl"
		});
	else
		target:set("runtimes", "MD")
		target:set("optimize", "aggressive")
		target:set("warnings", "none")
		target:add("cxflags", "/GS-", "/Gd", {
			tools = {"clang_cl", "cl"}
		})
		target:add("cxflags", "/Zc:preprocessor", {
			tools = "cl"
		})
	end
	if is_plat("windows") then
		target:add("defines","UNICODE","_UNICODE","_WIN32","_WINDOWS")
	end
end)
rule_end()
-- In-case of submod, when there is override rules, do not overload
if _config_rules == nil then
	_config_rules = {"engine_basic_settings"}
end
if _disable_unity_build == nil then
	_disable_unity_build = not get_config("enable_unity_build")
end

if _configs == nil then
	_configs = {}
end
function _config_project(config)
	if type(_configs) == "table" then
		for k, v in pairs(_configs) do
			set_values(k, v)
		end
	end
	if type(_config_rules) == "table" then
		add_rules(_config_rules)
	end
	if type(config) == "table" then
		for k, v in pairs(config) do
			set_values(k, v)
		end
	end
	local batch_size = config["batch_size"]
	if type(batch_size) == "number" and batch_size > 1 and (not _disable_unity_build) then
		add_rules("c.unity_build", {
			batchsize = batch_size
		})
		add_rules("c++.unity_build", {
			batchsize = batch_size
		})
	end
end