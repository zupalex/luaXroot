----------------------- SEMAPHORES --------------------------------

local sem = {}

sem._activesems = {}

local function PrintSemaphoresHelp()
  print("To create a Semaphores set:")
  print("sem.CreateSemSet(path, nsems, flags, id)")
  print("To get an existing Semaphores set:")
  print("mmap.GetSemSet(path)")
  print("")
  print("  -> path : a string, path to the semaphores set file")
  print("  -> nsem : an integer, how many semaphores in the set")
  print("  -> flags : a string, creation mode")
  print("       * \"recreate\" = if the semaphore set file already exists, it is deleted and regenerated")
  print("       * \"protected\" = if the semaphore file already exists, the creation will fail")
  print("       * \"open\" = if the semaphore file already exists, it will simply be opened")
  print("  -> id (mostly useless): an integer, should be left blank")
end

local SemaphoreObject = LuaClass("SemaphoreObject", function(self, data)
    self.path = data and data.path or "/tmp/semset"
    self.fd = data and data.fd or -1
    self.key = data and data.key or -1
    self.id = data and data.id or -1
    self.nsem = data and data.nsem or 0

    function self:SetValue(semnum, val)
      return SemCtl({semid=self.id, semnum=semnum, val=val, cmd=SETVAL})
    end


    function self:SetAllValue(vals)
      return SemCtl({semid=self.id, semnum=0, val=vals, cmd=SETALL})
    end

    function self:GetValue(semnum)
      return SemCtl({semid=self.id, semnum=semnum, cmd=GETVAL})
    end

    function self:GetAllValue()
      return SemCtl({semid=self.id, semnum=0, cmd=GETALL})
    end

    function self:Operate(semnums, ops)
      if type(semnums) == "table" then
        return SemOp({semid=self.id, semnum=semnums, sop=ops})
      else
        return SemOp({semid=self.id, semnum={semnums}, sop={ops}})
      end
    end
  end)

function sem.CreateSemSet(path, nsem, flag, id)
  local openfile_flags = "O_CREAT | O_RDWR | O_NONBLOCK"
  local semfileid = SysOpen({name=path, flags=openfile_flags})

  local semkey = SysFtok({pathname=path, id=id})

  if flag == nil or flag == "recreate" then 
    flag = "IPC_CREAT | 0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "open" then
    flag = "0666"
  end

  local semid = SemGet({key=semkey, nsem=nsem, flag=flag})

  local semobj = SemaphoreObject({path=path, nsem=nsem, id=semid, fd=semfileid, key=semkey})
  sem._activesems[semkey] = semobj

  return semobj
end

function sem.GetSemSet(path, id)
  local openfile_flags = "O_RDWR | O_NONBLOCK"
  local semfileid = SysOpen({name=path, flags=openfile_flags})

  local semkey = SysFtok({pathname=path, id=id})

  local semid = SemGet({key=semkey, nsem=0, flag="0666"})

  local seminfo = SemCtl({semid=semid, semnum=0, cmd=IPC_STAT})

  local semobj = SemaphoreObject({path=path, nsem=seminfo.nsem, id=semid, fd=semfileid, key=semkey})
  sem._activesems[semkey] = semobj

  return semobj
end

----------------------- SHARED MEMORY --------------------------------

local shmem = {}

shmem._activeshmems = {}

----------------------- MEMORY MAPPED FILES --------------------------------

local mmap = {}

mmap._activemmaps = {}

local function PrintMemoryMappedFileHelp()
  print("To create a memory mapped file:")
  print("mmap.CreateMMap(path, buffer, prot, flags, offset)")
  print("To attach to an existing memory mapped file:")
  print("mmap.AttachToMMap(path, buffer, prot, flags)")
  print("")
  print("  -> path : a string, path to the memory mapped file")
  print("  -> buffer : a userdata, will be the interface to read from and write to the memory mapped file")
  print("  -> prot : a string, protection mode")
  print("       * \"read\" = read authorization only on the mmap")
  print("       * \"write\" = write authorization only on the mmap")
  print("       * \"full\" = read and write authorization to the mmap")
  print("  -> flags (default = \"MAP_SHARE\"): a string, type of memory mapped file")
  print("       * \"MAP_SHARED\" = the mmap can be accessed by other processes")
  print("       * \"MAP_PRIVATE\" = the mmap can be accessed by this process only")
  print("  -> offset (default = 0) : an integer, offset from the beginning of the mmap")
end

local MMapObject = LuaClass("MMapObject", function(self, data)
    self.path = data and data.path or "/tmp/mmap"
    self.size = data and data.size or 0
    self.fd = data and data.fd or -1
    self.buffer = data and data.buffer or nil
    self.current_offset = data and data.offset or 0

    function self:SetAddress(buffer)
      AssignMMap({mapid=self.fd, buffer=buffer})
      self.buffer = buffer
    end

    function self:SetByteOffset(offset)
      if offset and offset ~= self.current_offset and offset <= self.size-self.buffer.sizeof then
        self.current_offset = AssignMMap({mapid=self.fd, buffer=self.buffer, offset=offset})
      end
    end

    function self:SetOffset(offset)
      local dest = offset and offset*self.buffer.sizeof
      if dest and dest ~= self.current_offset and dest <= self.size-self.buffer.sizeof then
        self.current_offset = AssignMMap({mapid=self.fd, buffer=self.buffer, offset=offset*self.buffer.sizeof})
      end
    end

    function self:Advance(n)
      local dest = n and self.current_offset+n*self.buffer.sizeof
      if dest <= self.size-self.buffer.sizeof then
        self.buffer:ShiftAddress(n)
        return true
      end
    end

    function self:Next()
      self:Advance(1)
    end

    function self:Previous()
      self:Advance(-1)
    end

    function self:Read(at)
      if at and at ~= self.current_offset/self.buffer.sizeof then
        if self:Advance(at - self.current_offset/self.buffer.sizeof) then
          self.current_offset = at*self.buffer.sizeof
        end
      end

      return self.buffer:Get(), self.current_offset+self.buffer.sizeof
    end

    function self:SetValue(data, at)
      if at and at ~= self.current_offset/self.buffer.sizeof then
        if self:Advance(at - self.current_offset/self.buffer.sizeof) then
          self.current_offset = at*self.buffer.sizeof
        end
      end

      self.buffer:Set(data)

      return self.current_offset+self.buffer.sizeof
    end
  end)

function PrintMMFileHelp()
  return PrintMemoryMappedFileHelp()
end

function mmap.CreateMMap(path, buffer, prot, flags, offset)
  if offset == nil then offset = 0 end
  if prot == nil then prot = "read" end
  if flags == nil then flags = "MAP_SHARED" end

  local size

  if type(buffer) == "number" then 
    size = buffer
  else
    size = buffer.sizeof
  end

  local openfile_flags = "O_CREAT | O_RDWR | O_NONBLOCK"
  local mmfile_prot = ""

  if prot == "read" then
    mmfile_prot = "PROT_READ"
  elseif prot == "write" then
    mmfile_prot = "PROT_WRITE"
  elseif prot == "full" then
    mmfile_prot = "PROT_READ | PROT_WRITE"
  end

  mmfid = SysOpen({name=path, flags=openfile_flags})

  SysFtruncate({fd=mmfid, size=size})

  NewMMap({mapid = mmfid, prot=mmfile_prot, flags=flags, size=size, offset=offset})

  local mmapobj = MMapObject({fd=mmfid, path=path, size = size})
  mmap._activemmaps[mmfid] = mmapobj

  if type(buffer) == "userdata" then 
    AssignMMap({mapid=mmfid, buffer=buffer})

    if prot == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only memory mapped file") end
      buffer.PushBack = function() print("Userdata associated to a read-only memory mapped file") end
    end

    mmapobj.buffer = buffer
  end

  return mmapobj
end

function mmap.AttachToMMap(path, buffer, prot, flags)
  if prot == nil then prot = "read" end
  if flags == nil then flags = "MAP_SHARED" end

  local size

  if type(buffer) == "number" then 
    size = buffer
  else
    size = buffer.sizeof
  end

  local mmfid

  if type(path) == "number" then
    mmfid = path
  else
    local openfile_flags = "O_RDWR | O_NONBLOCK"
    local mmfile_prot = ""

    if prot == "read" then
      mmfile_prot = "PROT_READ"
    elseif prot == "write" then
      mmfile_prot = "PROT_WRITE"
    elseif prot == "full" then
      mmfile_prot = "PROT_READ | PROT_WRITE"
    end

    mmfid = SysOpen({name=path, flags=openfile_flags})

    NewMMap({mapid = mmfid, prot=mmfile_prot, flags=flags, size=size, offset=offset})
  end

  local mmapobj = MMapObject({fd=mmfid, path=path, size = size})
  mmap._activemmaps[mmfid] = mmapobj

  if type(buffer) == "userdata" then 
    AssignMMap({mapid=mmfid, buffer=buffer})

    if prot == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only memory mapped file") end
      buffer.PushBack = function() print("Userdata associated to a read-only memory mapped file") end
    end

    mmapobj.buffer = buffer
  end

  return mmapobj
end

return {sem, shmem, mmap}