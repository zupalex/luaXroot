local socket = {}

local SocketObject = LuaClass("SocketObject", function(self, data)
    self.type = data.type or "client"
    self.sockfd = data.sockfd or nil
    self.clientsfd = data.clientsfd or nil
    self.address = data.address or nil
    self.port = data.port or nil

    function self:Send(data, size, listeners)
      if type(size) == "table" then
        listeners = size
        size = nil
      end

      for i, v in ipairs(listeners ~= nil and listeners or self.clientsfd) do
        return SocketSend({sockfd=v, data=data, size=size})
      end
    end

    function self:Receive(size)
      return SocketReceive({sockfd=self.sockfd, size=size})
    end
  end, nil, true)

local function SeparateAddressAndPort(full_address)
  local portSep = full_address:find(":")

  local port = nil

  if portSep ~= nil then
    port = full_address:sub(portSep+1)
    full_address = full_address:sub(1, portSep-1)
  end

  return full_address, port
end

function socket.CreateHost(type, address, maxqueue)
  if maxqueue == nil then maxqueue = 1 end
  local hfd, port

  if type == "local" then
    hfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
    os.remove(address)
  elseif type == "IPV4" or type == "ipv4" or type == "network" or type == "net" then
    hfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
  elseif type == "IPV6" or type == "ipv6" then
    hfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
  else
    print("Invalid socket type", type)
    print("Valid types are", "local", "net", "ipv4", "ipv6")
    return nil, nil
  end

  address, port = SeparateAddressAndPort(address)

  SocketBind({sockfd=hfd, name=address, address=address, port=port})
  SocketListen({sockfd=hfd, maxqueue=maxqueue})
  local dfd = SocketAccept({sockfd=hfd})

  return SocketObject({type="host", sockfd=hfd, clientsfd={dfd}, address=address, port=port})
end

function socket.CreateClient(type, address)
  if maxqueue == nil then maxqueue = 1 end
  local cfd, port

  if type == "local" then
    cfd = NewSocket({domain=AF_UNIX, type=SOCK_STREAM, protocol=0})
  elseif type == "IPV4" or type == "ipv4" or type == "network" or type == "net" then
    cfd = NewSocket({domain=AF_INET, type=SOCK_STREAM, protocol=0})
  elseif type == "IPV6" or type == "ipv6" then
    cfd = NewSocket({domain=AF_INET6, type=SOCK_STREAM, protocol=0})
  else
    print("Invalid socket type", type)
    print("Valid types are", "local", "net", "ipv4", "ipv6")
    return nil, nil
  end

  address, port = SeparateAddressAndPort(address)

  SocketConnect({sockfd=cfd, name=address, address=address, port=port})

  return SocketObject({type="client", sockfd=cfd, address=address, port=port})
end

return socket
