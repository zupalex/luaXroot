
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

  function tbl:GetSize()
    return #tbl
  end

  function tbl:back()
    return tbl[#tbl]
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

local _SizeOf = SizeOf
function SizeOf(data)
  if type(data) == "string" then
    return _SizeOf(data)
  else
    return data.StrLength == nil and data.sizeof or data:StrLength()
  end
end

--------------------------------------------------------------------------------------------
----------------------------------Misc. Utilities-------------------------------------------
--------------------------------------------------------------------------------------------

local _GetClockTime = GetClockTime
function GetClockTime(format, clockid)
  if format == nil or type(format) == "number" then 
    local sec, nsec = _GetClockTime(format)
    return {clockid=format, sec=sec, nsec=nsec}
  end

  local sec, nsec = _GetClockTime(clockid)

  if format == "second" then
    return _utilities.roundnumber(sec+nsec*10^-9)
  elseif format == "millisecond" then
    return _utilities.roundnumber(sec*10^3+nsec*10^-6)
  elseif format == "microsecond" then
    return _utilities.roundnumber(sec*10^6+nsec*10^-3)
  elseif format == "nanosecond" then
    return _utilities.roundnumber(sec*10^9+nsec)
  end
end

function ClockTimeDiff(origin, format)
  local clockid = origin.clockid

  local clock_data = GetClockTime(clockid)

  clock_data.sec = clock_data.sec-origin.sec
  clock_data.nsec = clock_data.nsec-origin.nsec

  if format == nil then
    return clock_data
  elseif format == "second" then
    return _utilities.roundnumber(clock_data.sec+clock_data.nsec*10^-9)
  elseif format == "millisecond" then
    return _utilities.roundnumber(clock_data.sec*10^3+clock_data.nsec*10^-6)
  elseif format == "microsecond" then
    return _utilities.roundnumber(clock_data.sec*10^6+clock_data.nsec*10^-3)
  elseif format == "nanosecond" then
    return _utilities.roundnumber(clock_data.sec*10^9+clock_data.nsec)
  end
end

function isint(x)
  return x == math.floor(x)
end

_utilities = {}

function _utilities.roundnumber(num)
  local num_int = math.floor(num)
  local tenth = math.floor((num-num_int)*10)
  if tenth >= 5 then
    return num_int+1
  else
    return num_int
  end
end

function  _utilities.removetrailingspaces(str)
  return str:gsub("%s+", "")
end

function _utilities.stringtoflags(str)
  local flags = {}
  local ops = {}

  local orPos = str:find("|")
  local andPos = str:find("&")

  local sepPos = math.min(orPos or 0, andPos or 0)

  local flag_str, flag_nbr

  while orPos ~= nil or andPos ~= nil do
    local nextsep = math.min(orPos or math.huge, andPos or math.huge)
    flag_str = _utilities.removetrailingspaces(str:sub(1, nextsep-1))
    flag_nbr = tonumber(flag_str, 8)

    if flag_nbr == nil then
      if _G[flag_str] ~= nil then
        flag_nbr = tonumber(_G[flag_str])
      end
    end

    if flag_nbr == nil then
      print("Invalid flag part:", flag_str)
      return "0"
    end

    table.insert(flags, flag_nbr)
    table.insert(ops, nextsep == orPos and "|" or "&")

    str = str:sub(nextsep+1)

    if nextsep == orPos then
      orPos = str:find("|")
    else
      andPos = str:find("&") 
    end
  end

  flag_str = _utilities.removetrailingspaces(str)
  flag_nbr = tonumber(flag_str, 8)

  if flag_nbr == nil then
    if _G[flag_str] ~= nil then
      flag_nbr = tonumber(_G[flag_str])
    end
  end

  table.insert(flags, flag_nbr)

  for i=1,#flags do
    if ops[i] ~= nil then
      if ops[i] == "|" then
        flags[i+1] = flags[i] | flags[i+1]
      else
        flags[i+1] = flags[i] & flags[i+1]
      end
    else
      return flags[i]
    end
  end
end

function BlockUntilReadable(fd, verbose)
  if type(fd) ~= "table" then
    fd = {fd}  
  end

  local rfd
  while rfd == nil or rfd == 0 do 
    rfd, wfd = SysSelect({read=fd}) 
    if rfd == 0 and verbose then
      print("File descriptor not ready to be read... waiting...")
    end
  end

  return fd
end

function BlockUntilWritable(fd, verbose)
  if type(fd) ~= "table" then
    fd = {fd}  
  end

  local rfd, wfd
  while wfd == nil or rfd == 0 do 
    rfd, wfd = SysSelect({write=fd}) 
    if rfd == 0 and verbose then
      print("File descriptor not ready for writing... waiting...")
    end
  end

  return fd
end

local FileObject = LuaClass("FileObject", function(self, data)
    self.fd = data and data.fd or nil

    function self:Write(data, size)
      return SysWrite({fd=self.fd, data=data, size=size})
    end

    function self:Read(size)
      return SysRead({fd=self.fd, size=size})
    end

    function self:WaitAndRead(size, verbose)
      BlockUntilReadable(self.fd, verbose)

      return self:Read(size)
    end

    function self:WaitAndWrite(data, size, verbose)
      BlockUntilWritable(self.fd, verbose)

      return self:Write(data, size)
    end

    function self:Close()
      return SysClose(self.fd)
    end
  end)

function OpenFile(path, flags, mode)
  if flags == nil then
    flags = "O_RDWR | O_CREAT"  
  end

  if mode == nil then
    mode = "0666"
  end

  local fd = SysOpen({name=path, flags=flags, mode=mode})

  local fileobj = FileObject({fd=fd})

  function fileobj:Beg()
    return SysLSeek(self.fd, 0, SEEK_SET)
  end

  function fileobj:End()
    return SysLSeek(self.fd, 0, SEEK_END)
  end

  function fileobj:GetPosition()
    return SysLSeek(self.fd, 0, SEEK_CUR)
  end

  function fileobj:SetPosition(pos)
    if pos >= 0 then return SysLSeek(self.fd, pos, SEEK_SET)
    else return SysLSeek(self.fd, pos, SEEK_END) end 
  end

  function fileobj:Advance(pos)
    return SysLSeek(self.fd, pos, SEEK_CUR)
  end

  function fileobj:GetLength()
    local cur_pos = self:GetPosition()
    local length = SysLSeek(self.fd, 0, SEEK_END)
    SysLSeek(self.fd, cur_pos, SEEK_SET)
    return length
  end

  return fileobj
end

local PipeObject = LuaClass("PipeObject", "FileObject", function(self, data)
    self.type = data and data.type or 1

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
  end)

function ForkAndRedirect(fn, args_table)
  local fds = {}

  local function prefn()
    fds.input, fds.output = MakePipe()
  end

  local function execfn(...)
    SysDup2(fds.input, 1)
    SysClose(fds.output)

    fn(...)
  end

  SysFork({fn=execfn, args=args_table, preinit=prefn})

  return PipeObject({type = 1, fd = fds.output})
end

function AttachOutput(file, command, args_table, env_table)
  local fds = {}

  local function prefn()
    fds.input, fds.output = MakePipe()
  end

  if type(command) == "table" then
    env_table = args_table
    args_table = command
    command = file
  end

  local function execfn(...)
    SysDup2(fds.input, 1);
    SysClose(fds.output);

    SysExec({file=file, args={command, ...}, env=env_table})
  end

  SysFork({fn=execfn, args=args_table, preinit=prefn})

  return PipeObject({type = 1, fd = fds.output})
end
