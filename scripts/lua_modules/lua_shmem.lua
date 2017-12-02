---------------------- MESSAGES QUEUE -----------------------------

local msgq = {}

msgq._activemsgqs = {}

function msgq.ListActiveMsgqs()
  print("----- Active Messages Queues -----")
  for k, v in pairs(msgq._activemsgqs) do
    print("  * id = ", v.id)
    print("  * nsems = ", v.nsem)
    print("  * key = ", v.key)
    print("  * file descriptor = ", v.fd)
    print("-------------------------")
  end
end

function PrintMessagesQueuesHelp()
  print("To create a Messages Queue:")
  print("msgq.CreateMsgq([path or key], flags)")
  print("To get an existing Messages Queue:")
  print("msgq.GetMsgq([path or key])")
  print("")
  print("  -> path : a string, path to the Messages Queue file")
  print("  -> key : an integer, the key to the Messages Queue")
  print("  -> flags : a string, creation mode")
  print("       * \"recreate\" = if the Messages Queue file already exists, it is deleted and regenerated")
  print("       * \"protected\" = if the Messages Queue file already exists, the creation will fail")
  print("       * \"open\" = if the Messages Queue file already exists, it will simply be opened")
end

local MsgqObject = LuaClass("MsgqObject", function(self, data)
    self.path = data and data.path or "/tmp/msgq"
    self.fd = data and data.fd or -1
    self.key = data and data.key or -1
    self.id = data and data.id or -1
    self.size = data and data.size or 0
    self.owner = data and data.owner or nil

    function self:Receive(format, mtype, flags)
      local msgsize = 0

      for i, v in ipairs(format) do
        msgsize = msgsize + SizeOf(v)
      end

      if mtype == nil then mtype = 0 end

      self.last_msg = MsgRcv({format=format, msgqid=self.id, mtype=mtype, size=msgsize, flags=flags})
      return self.last_msg
    end

    function self:Send(format, message, mtype, flags)
      local msgsize = 0

      for i, v in ipairs(format) do
        msgsize = msgsize + SizeOf(v)
      end

      if mtype == nil then mtype = 1 end

      return MsgSnd({data={values=message, format=format}, msgqid=self.id, mtype=mtype, size=msgsize, flags=flags})
    end

    function self:GetLast()
      return self.last_msg
    end
  end)

function msgq.CreateMsgq(path, flag)
  local msgkey

  if type(path) == "string" then
    local openfile_flags = "O_CREAT | O_RDWR | O_NONBLOCK"
    local msgfileid = SysOpen({name=path, flags=openfile_flags})

    local maxsize = 4096

    SysFtruncate({fd=msgfileid, size=maxsize})

    msgkey = SysFtok({pathname=path})
  else
    msgkey = path
  end

  if flag == nil or flag == "recreate" then 
    flag = "IPC_CREAT | 0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "open" then
    flag = "0666"
  end

  local msgid = MsgGet({key=msgkey, flags=flag})

  local msgqobj = MsgqObject({path=path, size=maxsize, id=msgid, fd=msgfileid, key=msgkey, owner=true})
  msgq._activemsgqs[msgkey] = msgqobj

  return msgqobj
end

function msgq.GetMsgq(path, flag)
  local msgkey 

  if type(path) == "string" then
    msgkey = SysFtok({pathname=path})
  else
    msgkey = path
  end

  if flag == nil or flag == "open" then 
    flag = "0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "recreate" then
    flag = "IPC_CREAT | 0666"
  end

  local msgid = MsgGet({key=msgkey, flags=flag})

--  local msgqstats = MsgCtl({msgqid=msgid, cmd=IPC_STAT})

--  local maxsize = msgqstats.max_size
  local maxsize = 4096

  local msgqobj = MsgqObject({path=path, size=maxsize, id=msgid, key=msgkey})
  msgq._activemsgqs[msgkey] = msgqobj

  return msgqobj
end

----------------------- SEMAPHORES --------------------------------

local sem = {}

sem._activesems = {}

function sem.ListActiveSemaphores()
  print("----- Active Semaphores -----")
  for k, v in pairs(sem._activesems) do
    print("  * id = ", v.id)
    print("  * nsems = ", v.nsem)
    print("  * key = ", v.key)
    print("  * file descriptor = ", v.fd)
    print("-------------------------")
  end
end

function PrintSemaphoresHelp()
  print("To create a Semaphores set:")
  print("sem.CreateSemSet([path or key], nsems, flags, id)")
  print("To get an existing Semaphores set:")
  print("mmap.GetSemSet([path or key])")
  print("")
  print("  -> path : a string, path to the semaphores set file")
  print("  -> key : an integer, the key to the semaphores set")
  print("  -> nsem : an integer, how many semaphores in the set")
  print("  -> flags : a string, creation mode")
  print("       * \"recreate\" = if the semaphore set file already exists, it is deleted and regenerated")
  print("       * \"protected\" = if the semaphore file already exists, the creation will fail")
  print("       * \"open\" = if the semaphore file already exists, it will simply be opened")
end

local SemaphoreObject = LuaClass("SemaphoreObject", function(self, data)
    self.path = data and data.path or "/tmp/semset"
    self.fd = data and data.fd or -1
    self.key = data and data.key or -1
    self.id = data and data.id or -1
    self.nsem = data and data.nsem or 0
    self.owner = data and data.owner or nil

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

function sem.CreateSemSet(path, nsem, flag)
  local semkey

  if type(path) == "string" then
    local openfile_flags = "O_CREAT | O_RDWR | O_NONBLOCK"
    local semfileid = SysOpen({name=path, flags=openfile_flags})

    semkey = SysFtok({pathname=path})
  else
    semkey = path
  end

  if flag == nil or flag == "recreate" then 
    flag = "IPC_CREAT | 0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "open" then
    flag = "0666"
  end

  local semid = SemGet({key=semkey, nsem=nsem, flags=flag})

  local semobj = SemaphoreObject({path=path, nsem=nsem, id=semid, fd=semfileid, key=semkey, owner=true})
  sem._activesems[semkey] = semobj

  return semobj
end

function sem.GetSemSet(path)
  local semkey

  if type(path) == "string" then
    semkey = SysFtok({pathname=path})
  else
    semkey = path
  end

  local semid = SemGet({key=semkey, nsem=0, flag="0666"})

  local seminfo = SemCtl({semid=semid, semnum=0, cmd=IPC_STAT})

  local semobj = SemaphoreObject({path=path, nsem=seminfo.nsem, id=semid, key=semkey})
  sem._activesems[semkey] = semobj

  return semobj
end

----------------------- SHARED MEMORY --------------------------------

local shmem = {}

shmem._activeshmems = {}

function shmem.ListActiveShMem()
  print("----- Active Shared Memory Segments -----")
  for k, v in pairs(shmem._activeshmems) do
    print("  * path = ", v.path)
    print("  * id = ", v.id)
    print("  * size = ", v.size)
    print("  * key = ", v.key)
    print("-------------------------")
  end
end

function PrintSharedMemoryHelp()
  print("To create a shared memory segment:")
  print("shmem.CreateShMem([path or key], [size or buffer], flag)")
  print("To get an existing memory mapped file:")
  print("shmem.GetShMem([path or key], buffer, flags)")
  print("")
  print("  -> path : a string, path to the shared memory segment \"file\"")
  print("  -> key : an integer, the key to the shared memory segment")
  print("  -> size : an integer, the size of the shared memory segment. If called with size, user need to attach a userdata afterward to use the segment")
  print("  -> buffer : a userdata, will be the interface to read from and write to the shared memory segment")
  print("  -> flag : a string, protection mode")
  print("       * \"recreate\" = if the shared memory support file already exists, it is deleted and regenerated")
  print("       * \"protected\" = if the shared memory support file already exists, the creation will fail")
  print("       * \"open\" = if the shared memory support file already exists, it will simply be opened")
end

local ShMemObject = LuaClass("ShMemObject", function(self, data)
    self.path = data and data.path or "/tmp/shmem"
    self.fd = data and data.fd or -1
    self.key = data and data.key or -1
    self.id = data and data.id or -1
    self.size = data and data.size or 0
    self.buffer = data and data.buffer or nil
    self.struct = data and data.struct or nil
    self.current_offset = data and data.current_offset or 0
    self.owner = data and data.owner or nil

    function self:AutoGet()
      if self.struct == nil then
        return self.buffer:Get()
      elseif self.struct then
        return ShmGetMem({shmid=self.id, output=self.buffer and self.buffer or self.struct})
      end
    end

    function self:AutoSet(data)
      if self.struct == nil then
        return self.buffer:Set(data)
      elseif self.struct then
        return ShmSetMem({shmid=self.id, input=data, format=self.struct})
      end
    end

    function self:GetStepSize()
      if self.struct == nil then
        return self.buffer.sizeof
      elseif self.struct then
        return self.struct.sizeof
      end
    end

    function self:SetAddress(buffer, offset)
      AssignShmem({shmid=self.id, buffer=buffer, offset=offset})
      self.buffer = buffer
      self.struct = nil
    end

    function self:SetStruct(struct)
      if type(struct) == "table" and #struct > 0 then
        if type(struct[1]) == "userdata" then
          self.buffer = struct
          self.struct = {}
        elseif type(struct[1]) == "string" then
          self.struct = struct
          self.buffer = nil
        end        

        local structsize = 0
        for i, v in ipairs(struct) do
          structsize = structsize + SizeOf(v)

          if self.buffer then self.struct[i] = v.type end
        end

        self.struct.sizeof = structsize
      end
    end

    function self:SetOffset(offset)
      local dest = offset and offset*self:GetStepSize()
      if dest and dest ~= self.current_offset and dest <= self.size-self:GetStepSize() then
        AssignShmem({shmid=self.id, buffer=self.buffer})
        self:Advance(offset)
        self.current_offset = dest
      end
    end

    function self:Advance(n)
      local dest = n and self.current_offset+n*self:GetStepSize()
      if dest <= self.size-self:GetStepSize() then
        self.buffer:ShiftAddress(n)
        self.current_offset = dest
        return dest
      end
    end

    function self:Next()
      return self:Advance(1)
    end

    function self:Previous()
      return self:Advance(-1)
    end

    function self:Read(at)
      local result
      if at and at >= 0 and at ~= self.current_offset/self:GetStepSize() then
        local shift = at - self.current_offset/self:GetStepSize()
        if self:Advance(shift) then
          result = self:AutoGet()
          self:Advance(-shift)
        else
          return nil
        end
      else
        result = self:AutoGet()
      end

      return result
    end

    function self:SetValue(data, at)
      if at and at >= 0 and at ~= self.current_offset/self:GetStepSize() then
        local shift = at - self.current_offset/self:GetStepSize()
        if self:Advance(shift) then
          self:AutoSet(data)
          self:Advance(-shift)
        else
          return nil
        end
      else
        self:AutoSet(data)
      end

      return true
    end

    function self:RawRead(size, offset)
      return ShmRawRead({shmid=self.id, size=size, offset=offset})
    end
  end)

function shmem.CreateShMem(path, buffer, flag)
  local shmkey 

  if type(path) == "string" then
    local openfile_flags = "O_CREAT | O_RDWR | O_NONBLOCK"
    local shmfileid = SysOpen({name=path, flags=openfile_flags})

    shmkey = SysFtok({pathname=path})
  else
    shmkey = path
  end

  if flag == nil or flag == "recreate" then 
    flag = "IPC_CREAT | 0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "open" then
    flag = "0666"
  end

  local size
  if type(buffer) == "number" then 
    size = buffer
  else
    size = buffer.sizeof
  end

  local shmid = ShmGet({key=shmkey, size=size, flags=flag})
  ShmAt({shmid=shmid})

  local shmobj = ShMemObject({path=path, size=size, id=shmid, fd=shmfileid, key=shmkey, owner=true})
  shmem._activeshmems[shmkey] = semobj

  if type(buffer) == "userdata" then 
    AssignShMem({shmid=shmid, buffer=buffer})

    if flag == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only shared memory segment") end
      buffer.PushBack = function() print("Userdata associated to a read-only shared memory segment") end
    end

    shmobj.buffer = buffer
  end

  return shmobj
end

function shmem.GetShMem(path, buffer, flag)
  local shmkey

  if type(path) == "string" then
    shmkey = SysFtok({pathname=path})
  else
    shmkey = path
  end

  if flag == nil or flag == "open" then 
    flag = "0666"
  elseif flag == "protected" then
    flag = "IPC_CREAT | IPC_EXCL | 0666"
  elseif flag == "recreate" then
    flag = "IPC_CREAT | 0666"
  end

  local shmid = ShmGet({key=shmkey, size=1, flags=flag})

  local shmstat = ShmCtl({shmid=shmid, cmd=IPC_STAT})

  local size = shmstat.size

  shmid = ShmGet({key=shmkey, size=size, flags="0666"})
  ShmAt({shmid=shmid})

  local shmobj = ShMemObject({path=path, size=size, id=shmid, key=shmkey})
  shmem._activeshmems[shmkey] = shmobj

  if type(buffer) == "userdata" then 
    AssignShmem({shmid=shmid, buffer=buffer})

    if flag == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only shared memory segment") end
      buffer.PushBack = function() print("Userdata associated to a read-only shared memory segment") end
    end

    shmobj.buffer = buffer
  end

  return shmobj
end

----------------------- MEMORY MAPPED FILES --------------------------------

local mmap = {}

mmap._activemmaps = {}

function mmap.ListActiveMMap()
  print("----- Active Memory Mapped Files -----")
  for k, v in pairs(mmap._activemmaps) do
    print("  * path = ", v.path)
    print("  * size = ", v.size)
    print("  * file descriptor = ", v.fd)
    print("-------------------------")
  end
end

function PrintMemoryMappedFileHelp()
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
    self.owner = data and data.owner or nil

    function self:SetAddress(buffer)
      AssignMMap({mapid=self.fd, buffer=buffer})
      self.buffer = buffer
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
        self.current_offset = dest
        return self.current_offset
      end
    end

    function self:Next()
      return self:Advance(1)
    end

    function self:Previous()
      return self:Advance(-1)
    end

    function self:Read(at)
      local result
      if at and at >= 0 and at ~= self.current_offset/self.buffer.sizeof then
        local shift = at - self.current_offset/self.buffer.sizeof
        if self:Advance(shift) then
          result = self.buffer:Get()
          self:Advance(-shift)
        else
          return nil
        end
      else
        result = self.buffer:Get()
      end

      return result
    end

    function self:SetValue(data, at)
      if at and at >= 0 and at ~= self.current_offset/self.buffer.sizeof then
        local shift = at - self.current_offset/self.buffer.sizeof
        if self:Advance(shift) then
          self.buffer:Set(data)
          self:Advance(-shift)
        else
          return nil
        end
      else
        self.buffer:Set(data)
      end

      return true
    end

    function self:RawRead(size, offset)
      return MMapRawRead({mapid=self.fd, size=size, offset=offset})
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

  NewMMap({mapid = mmfid, prot=mmfile_prot, flags=flags, size=size, offset=offset, owner=true})

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

return {msgq, sem, shmem, mmap}