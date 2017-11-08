
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

--------------------------------------------------------------------------------------------
----------------------------------Table Utilities-------------------------------------------
--------------------------------------------------------------------------------------------

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

function newtable()
  local tbl = {}

  function tbl:insert(stuff)
    table.insert(tbl, stuff)
  end

  return tbl
end

function InitTable(size, default)
  local tbl = {}

  for i= 1,size do
    tbl[i] = default
  end

  return tbl
end

function SplitTableKeyValue(tbl)
  local keys = newtable()
  local values = newtable()

  for k, v in pairs(tbl) do
    keys:insert(k)
    values:insert(v)
  end

  return keys, values
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
  return _G[classname](...)
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

--------------------------------------------------------------------------------------------
----------------------------------Misc. Utilities-------------------------------------------
--------------------------------------------------------------------------------------------

function isint(x)
  return x == math.floor(x)
end

local PipeObject = LuaClass("PipeObject", function(self, data)
    self.type = data.type or 1
    self.fd = data.fd or nil

    function self:Write(data, size)
      if self.type == 0 or self.type == 2 then
        return SysWrite({fd=self.fd, data=data, size=size})
      else
        print("Attempting to write on the recieving end of a pipe...")
        return
      end
    end

    function self:Read(size)
      if self.type >= 1 then
        return SysRead({fd=self.fd, size=size})
      else
        print("Attempting to read from the input end of a pipe...")
        return
      end
    end
  end, nil, true)

function AttachOutput(file, command, args_table)
  local fds = {}

  local function prefn()
    fds.input, fds.output = MakePipe()
  end

  if type(command) == "table" then
    args_table = command
    command = file
  end

  local function execfn(...)
    SysDup2(fds.input, 1);
    SysClose(fds.output);

    SysExec({file=file, args={command, ...}})
  end

  SysFork({fn=execfn, args=args_table, preinit=prefn})

  return PipeObject({type = 1, fd = fds.output})
end
