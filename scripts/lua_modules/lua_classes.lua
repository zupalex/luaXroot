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
classPostInits = {}

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

  for i, v in ipairs(obj.methods) do
    obj[v] = function(self, ...)
      return obj:Call(v, ...)
    end
  end
end

function MakeEasyConstructors(classname)
  _G[classname] = function(...)
    return _ctor(classname, ...)
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
function AddPostInit(class, fn)
  local constructor = _G[class]
  _G[class] = function(...)
    local obj = constructor(...)

    fn(obj)

    return obj
  end
end

---------------------------------------- Base "Class" Object ----------------------------------------

local LuaObject = LuaClass("LuaObject", function(self, data)
    self.name = TryGetPar(data, "name", "lua_object")
    self.title = TryGetPar(data, "title", "Lua Object")

    self.serializable = TryGetPar(data, "serializable", {})
  end)