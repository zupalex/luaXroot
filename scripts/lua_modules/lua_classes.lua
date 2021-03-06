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

function MakeCppClassCtor(classname)
  _G[classname] = function()
    return New(classname)
  end
end

luaClasses = {}
luaClassesPostInits = {}

function RegisterLuaClass(class)
--	print("Registering " .. class.classname)
  luaClasses[class.classname] = class
end

function LuaClass(name, base, init, skipregister)
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

    if luaClassesPostInits[name] ~= nil then
      -- print("Post Init for "..name)
      for k, v in pairs(luaClassesPostInits[name]) do
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

  if not skipregister then RegisterLuaClass(c) end

  return c
end

function Instantiate(classname, data)
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

--------------------------------------------------------------------------------------------
------------------Constructors and C++ Classes Method Calls Utilities-----------------------
--------------------------------------------------------------------------------------------

function MakeEasyMethodCalls(obj)
  if obj.methods == nil or obj.Call == nil then return end

  local method_prefix = obj.type.."::"

  for i, v in ipairs(obj.methods) do
    obj[v] = function(self, ...)
      return obj:Call(method_prefix..v, ...)
    end
  end
end

function MakeEasyConstructors(classname)
  _G[classname] = function(...)
    local obj = _ctor(classname, ...)

--    if classesPostInits[classname] then
--      for i, pifn in ipairs(classesPostInits[classname]) do
--        pifn(obj)
--      end
--    end

    return obj
  end
end

function New(classname, ...)
  if _G[classname] then
    return _G[classname](...)
  else
    return _ctor(classname, ...)
  end
end

-- Use this function to add stuffs to the metatable of a C++ Class --
classesPostInits = {}

function AddPostInit(class, fn)
  if luaClasses[class] then
    if not luaClassesPostInits[class] then luaClassesPostInits[class] = {} end

    table.insert(luaClassesPostInits[class], fn)
  elseif _G[class] then
    if not classesPostInits[class] then classesPostInits[class] = {} end

    table.insert(classesPostInits[class], fn)

--    local constructor = _G[class]

--    _G[class] = function(...)
--      local obj = constructor(...)

--      fn(obj)

--      return obj
--    end
  else
    print("Attempt to add a postinit to a class which does not exists... =>", class)
  end
end

function CallPostInits(obj)
  if classesPostInits[obj.type] then
    for i, pifn in ipairs(classesPostInits[obj.type]) do
      pifn(obj)
    end
  end
end

function SetupCommonMetatable(obj)
  obj.Set = _setterfns[obj.type]
  obj.Get = _getterfns[obj.type]

  function obj:Reset() 
    obj:Set(nil)
  end
end

function SetupStandardMetatable(obj)
  SetupCommonMetatable(obj)
end

function SetupVectorMetatable(obj)
  SetupCommonMetatable(obj)

  obj.PushBack = _pushbackfns[obj.type]

  function obj:GetSize(index)
    return #(obj:Get(index))
  end
end

function SetupArrayMetatable(obj)
  SetupCommonMetatable(obj)
end

function SetupMetatable(obj)  
  if obj.type:sub(1, 6) =="vector" then
    SetupVectorMetatable(obj)

    obj._iscollection = true
    obj._elem_type = obj.type:sub(8, obj.type:len()-1)
  else
    local array_pos = obj.type:find("%[")

    if array_pos then
      SetupArrayMetatable(obj)

      obj._iscollection = true
      obj._elem_type = obj.type:sub(1, array_pos-1)
    else
      SetupStandardMetatable(obj)
    end
  end
end

---------------------------------------- Base "Class" Object ----------------------------------------

local LuaObject = LuaClass("LuaObject", function(self, data)
    self.name = TryGetPar(data, "name", "lua_object")
    self.title = TryGetPar(data, "title", "Lua Object")

    self.serializable = TryGetPar(data, "serializable", {})
  end)