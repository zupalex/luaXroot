----------------------- MEMORY MAPPED FILES --------------------------------

local function PrintMemoryMappedFileHelp()
  print("To create a memory mapped file:")
  print("")
  print("")
  print("  -> type : a string giving the type of connection")
  print("       * \"local\" = socket connection between 2 process on the same machine")
  print("       * \"net\" = socket connection over the network (ipv4)")
  print("       * \"ipv6\" = socket connection over the network (ipv6)")
  print("")
  print("  -> address : a string giving the address to establish the socket connection")
  print("       * if type is \"local\" => address should be the path to a file on the machine (e.g. \"/tmp/mysocket\")")
  print("       * if type is \"net\" => address should be the ip:port of the host (e.g. \"192.168.13.110:1234\")")
  print("")
  print("  -> maxqueue (optional) : a number stating how many connection the host may accept")
  print("")
  print("These function will return an object that can then be use to send and receive data")
  print("Example:")
  print("On the host machine: sender = socket.CreateHost(\"local\", \"/tmp/mysocket\", 1)")
  print("On the client machine: receiver = socket.CreateClient(\"local\", \"/tmp/mysocket\")")
  print("On the host machine: sender:Send(\"Hello Client\")")
  print("On the client machine: receiver:Receive() => will print \"Hello Client\"")
end

function PrintMMFileHelp()
  return PrintMemoryMappedFileHelp()
end

function CreateMemoryMappedFile(path, buffer, prot, flags, offset)
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

  if type(buffer) == "userdata" then 
    AssignMMap({mapid=mmfid, buffer=buffer})

    if prot == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only memory mapped file") end
      buffer.PushBack = function() print("Userdata associated to a read-only memory mapped file") end
    end
  end

  return mmfid
end

function CreateMMFile(path, buffer, prot, flags, offset)
  return CreateMemoryMappedFile(path, buffer, prot, flags, offset)
end

function AttachToMemoryMappedFile(path, buffer, prot, flags)
  if type(buffer) ~= "userdata" then 
    print("Memory Mapped File assignment error: userdata expected, got", type(buffer))
    return
  end

  if prot == nil then prot = "read" end
  if flags == nil then flags = "MAP_SHARED" end

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

    NewMMap({mapid = mmfid, prot=mmfile_prot, flags=flags, size=buffer.sizeof, offset=offset})
  end

  AssignMMap({mapid=mmfid, buffer=buffer})

  if prot == "read" then
      buffer.__Set = buffer.Set
      buffer.__PushBack = buffer.PushBack
      buffer.Set = function() print("Userdata associated to a read-only memory mapped file") end
      buffer.PushBack = function() print("Userdata associated to a read-only memory mapped file") end
  end
end

function AttachToMMFile(path, buffer, prot, flags)
  return AttachToMemoryMappedFile(path, buffer, prot, flags)
end