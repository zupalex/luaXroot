local socket = {}

socket._activesockets = {}

function socket.ListActiveSockets()
  print("----- Active Sockets -----")
  for k, v in pairs(socket._activesockets) do
    print("  * ID = ", v.sockfd)
    print("  * type = ", v.type)
    print("  * address = ", v.address)
    print("  * port = ", v.port)
    print("-------------------------")
  end
end

local SocketObject = LuaClass("SocketObject", "PipeObject", function(self, data)
    self.type = data and data.type or "client"
    self.sockfd = data and data.sockfd or nil
    self.clientsfd = data and data.clientsfd or nil
    self.address = data and data.address or nil
    self.port = data and data.port or nil

    self.fd = self.sockfd

    function self:Send(data, size, listeners)
      if type(size) == "table" then
        listeners = size
        size = nil
      end

      for i, v in ipairs(listeners ~= nil and listeners or self.clientsfd) do
        return SocketSend({sockfd=v, data=data, size=size})
      end
    end

    function self:Receive(size, wait)
      self.last_rec = SocketReceive({sockfd=self.sockfd, size=size, flags=(wait and MSG_WAITALL or 0)})
      return self.last_rec
    end

    function self:SendResponse(data, size)
      return SocketSend({sockfd=self.sockfd, data=data, size=size})
    end

    function self:ReadResponse(fd, size)
      self.last_rec = SocketReceive({sockfd=fd, size=size})
      return self.last_rec
    end

    if self.type == "client" then
      function self:WaitAndReceive(size, wait)
        BlockUntilReadable(self.fd)

        return self:Receive(size, wait)
      end
    else
      function self:WaitAndReadResponse(fd, size)
        BlockUntilReadable(fd)

        return self:ReadResponse(fd, size)
      end
    end

    if self.type == "client" then
      function self:WaitAndSendResponse(data, size)
        BlockUntilWritable(self.fd)

        return self:SendResponse(data, size)
      end
    else
      function self:WaitAndSend(data, size)
        BlockUntilWritable(self.clientsfd)

        return self:Send(data, size)
      end
    end
  end)

local function SeparateAddressAndPort(full_address)
  local portSep = full_address:find(":")

  local port = nil

  if portSep ~= nil then
    port = full_address:sub(portSep+1)
    full_address = full_address:sub(1, portSep-1)
  end

  return full_address, port
end

function PrintSocketHelp()
  print("To create a socket connection:")
  print("The host should invoke socket.CreateHost(type, address, [maxqueue])")
  print("The client should invoke socket.CreateClient(type, address)")
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

function socket.CreateHost(type, address, maxqueue)
  if type == nil then
    return PrintSocketHelp()
  end

  if maxqueue == nil then maxqueue = 1 end
  local hfd, port

  if type == "local" then
    hfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
  elseif type == "IPV4" or type == "ipv4" or type == "network" or type == "net" then
    hfd = NewSocket({domain=AF_INET, type=SOCK_STREAM, protocol=0})
    address, port = SeparateAddressAndPort(address)
  elseif type == "IPV6" or type == "ipv6" then
    hfd = NewSocket({domain=AF_INET6, type=SOCK_STREAM, protocol=0})
    address, port = SeparateAddressAndPort(address)
  else
    print("Invalid socket type", type)
    print("Type socket.CreateHost() to get detailed help")
    return nil, nil
  end

  sockobj = SocketObject({type="host", sockfd=hfd, address=address, port=port})

  socket._activesockets[address] = sockobj

  print("Socket ("..hfd..") connection created at "..address..(port ~= nil and (":"..port) or "").." ... Waiting for connection(s) ...")

  if not SocketBind({sockfd=hfd, address=address, port=port}) then
    print("Socket binding failed...")
    return
  end

  if SocketListen({sockfd=hfd, maxqueue=maxqueue}) ~= 0 then
    print("Socket listen failed...")
    return
  end

  local dfd, errno = SocketAccept({sockfd=hfd})

  if dfd == nil then
    print("There was an issue while accepting the socket connection:", errno)
    return nil
  end

  sockobj.clientsfd= {dfd}

  return sockobj
end

function socket.CreateClient(type, address)
  if type == nil then
    return PrintSocketHelp()
  end

  local cfd, port

  if type == "local" then
    cfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
  elseif type == "IPV4" or type == "ipv4" or type == "network" or type == "net" then
    cfd = NewSocket({domain=AF_INET, type=SOCK_STREAM, protocol=0})
    address, port = SeparateAddressAndPort(address)
  elseif type == "IPV6" or type == "ipv6" then
    cfd = NewSocket({domain=AF_INET6, type=SOCK_STREAM, protocol=0})  
    address, port = SeparateAddressAndPort(address)
  else
    print("Invalid socket type", type)
    print("Type socket.CreateClient() to get detailed help")
    return nil, nil
  end

  sockobj = SocketObject({type="client", sockfd=cfd, address=address, port=port})

  socket._activesockets[address] = sockobj
  local success, errmsg = SocketConnect({sockfd=cfd, address=address, port=port})

  if not success then
    print(errmsg)
    return nil
  end

  return sockobj
end

return socket
