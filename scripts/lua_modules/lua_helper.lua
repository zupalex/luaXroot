
function shallowcopy(orig, copyNested, ignores, trackCopy)
	if trackCopy == nil then
		trackCopy = {}
	end

    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in pairs(orig) do
			if ignores == nil or not ignores[orig_key] then 
				local key_copy
			
				if type(orig_key) == "table" and copyNested then
					if trackCopy[orig_key] == nil then
						key_copy = {}
						trackCopy[orig_key] = key_copy
						key_copy = shallowcopy(orig_key, true, ignores, trackCopy)
					else
						key_copy = trackCopy[orig_key]
					end
				else
					key_copy = orig_key
				end
					
				if type(orig_value) == "table" and copyNested then
					if trackCopy[orig_value] == nil then
						local val_copy = {}
						trackCopy[orig_value] = val_copy
						val_copy = shallowcopy(orig_value, true, ignores, trackCopy)
						copy[key_copy] = val_copy
					else
						copy[key_copy] = trackCopy[orig_value]
					end
				else
					copy[key_copy] = orig_value
				end
			end
        end
    else -- number, string, boolean, etc
        copy = orig
    end
	
    return copy
end

function deepcopy(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in next, orig, nil do
            copy[deepcopy(orig_key)] = deepcopy(orig_value)
        end
        setmetatable(copy, deepcopy(getmetatable(orig)))
    else -- number, string, boolean, etc
        copy = orig
    end
    return copy
end

function printtable(tbl, printNested, ignores, level, maxlevel)
	if level == nil then
		level = 1
	end
	
	if ignores == nil then ignores = {} end
	
	local tabulation = "     "
	
	for i=1,level-1 do tabulation = tabulation.."-----" end
	
	if level == 1 then print(tabulation.."Printing table "..tostring(tbl)) end
	
	for k, v in pairs(tbl) do
		print(tabulation.."-> "..tostring(k).." = "..tostring(v))
		if printNested and ((maxlevel ~= nil and level <= maxlevel) or (maxlevel == nil)) and type(v) == "table" and not ignores[k] then printtable(v, printNested, ignores, level+1, maxlevel) end
	end
end

function findintable(tbl, tofind, checksubtable, maxlevel)
	if maxlevel == nil then maxlevel = -1 end

	if tbl ~= nil and tofind ~= nil then
		-- print("checking table "..tostring(tbl))
		
		if checksubtable ~= nil then
			-- print("   -> exclude table ("..tostring(checksubtable)..") contains:")
			for k, v in pairs(checksubtable) do
				-- print("   ->"..tostring(v))
			end
		end
		
		for k, v in pairs(tbl) do
			if v == tofind then
				-- print("   ===> Found it!")
				return v
			end
			
			if maxlevel ~= 0 and checksubtable ~= nil and type(v) == "table" then
				-- print("   --->checking subtable "..tostring(v))
				if findintable(checksubtable, v) == nil then
					table.insert(checksubtable, v)
					local subt = findintable(v, tofind, checksubtable, maxlevel-1)
					if subt ~= nil then return subt end
				else
					-- print("The subtable should be skipped as it is a reference to a parent table")
				end
			end
		end
	else
		return nil
	end
	
	return nil
end

function isint(x)
	return x == math.floor(x)
end

function InitTable(size, default)
	local tbl = {}

	for i= 1,size do
		tbl[i] = default
	end
	
	return tbl
end

structs = {}

function RegisterStruct(tbl, name, size)
	structs[name] = {}
	structs[name].members = {}
	structs[name].size = size ~= nil and size or 0
	
	for k, v in pairs(tbl) do
		structs[name].members[k] = v
	end
end

---------------------------------------- LuaClass Functions ----------------------------------------

luaClasses = {}
classPostInits = {}

function RegisterLuaClass(class)
--	print("Registering " .. class.classname)
	luaClasses[class.classname] = class
end

function LuaClass(name, base, init)
	local c = {}    -- a new class instance
	
	if not init and type(base) == "function" then
		init = base
		base = nil
	elseif type(base) == "string" then
		if luaClasses[base] then base = luaClasses[base] else base = nil end
	end
	
	if type(base) == "table" then
		-- our new class is a shallow copy of the base class!
		for k,v in pairs(base) do
			c[k] = v
		end
		c.parentclass = base
	end
	
	c.classname = name
	-- c._data = {}
	
	-- the class will be the metatable for all its objects,
	-- and they will look up their methods in it.
	c.__index = c

	-- expose a constructor which can be called by <classname>(<args>)
	local mt = {}
	mt.__call = function(class_tbl, data)
		local obj = {}
		setmetatable(obj,c)
		
		obj:init(data)
		-- obj._data = data
	
		return obj
	end
	
	c.init = function(self, data)
		if base and base.init then
			base.init(self, data)
		end
		
		if init then
			init(self, data)
		end
		
		if classPostInits[name] ~= nil then
			-- print("Post Init for "..name)
			for k, v in pairs(classPostInits[name]) do
				v(self, data)
			end
		end
	end
	
	c.is_a = function(self, klass)
		local m = getmetatable(self)
		if type(klass) == "string" then klass = luaClasses[klass] end
		while m do
			if m == klass then return true end
				m = m.parentclass
			end
			return false
	end
	
	setmetatable(c, mt)
	
	RegisterLuaClass(c)
	
	return c
end

function NewObject(classname, data)
	if luaClasses[classname] ~= nil then
		local newobj = luaClasses[classname](data)
		newobj._data = data
		return newobj
	else
		
		return nil
	end
end

function TryGetPar(tabl_, par_, default_)
	if tabl_ ~= nil and tabl_[par_] ~= nil then 
		return tabl_[par_]
	else 
		return default_
	end
end

function AddToClassInit(classname, fn, identifier)
	if classPostInits[classname] == nil then classPostInits[classname] = {} end

	if identifier then
		classPostInits[classname].identifier = fn
	else
		table.insert(classPostInits[classname], fn)
	end
end

---------------------------------------- Base "Class" Object ----------------------------------------

local LuaObject = LuaClass("LuaObject", function(self, data)
	self.name = TryGetPar(data, "name", "lua_object")
	self.title = TryGetPar(data, "title", "Lua Object")

	self.serializable = TryGetPar(data, "serializable", {})
end)