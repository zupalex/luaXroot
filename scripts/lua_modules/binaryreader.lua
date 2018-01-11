function PrintHexa(data, size)
  if data then
    if size == 1 then
      return string.format("0x%02x", data)
    elseif size == 2 then
      return string.format("0x%04x", data)
    elseif size == 3 then
      return string.format("0x%06x", data)
    elseif size == 4 then
      return string.format("0x%08x", data)
    else
      return string.format("0x%x", data)
    end
  else
    return tostring(nil)
  end
end

function GetPackSize(fmt)
  if fmt:find("c") then
    return 
  elseif fmt:find("z") then
    return
  else
    return string.packsize(fmt)
  end
end

function ReadBytes(file, fmt, tohexa)
  if file then
    local special_case = fmd:match("[cz]")

    local size

    if special_case == nil then 
      size = string.packsize(fmt)

      --      print("reading data of length", size, "starting at", file:seek("cur"))

      if size then
        local bdata = file:read(size)

        local status, data = pcall(string.unpack, fmt, bdata)

        if not status then
          print("ERROR in ReadBytes:", data)
          return
        end

        if tohexa and math.floor(data) == data then
          return PrintHexa(data, size)
        else
          return data
        end
      end
    elseif special_case == 'c' then
      size = tonumber(fmt:sub(fmt:find("c")+1))
--      print("reading string of length", size, "starting at", file:seek("cur"))

      if size then
        local str = file:read(size)
        return str
      end
    elseif special_case == 'z' then
      local char = file:read(1)
      local str = {}

      while char ~= '\0' do
        table.insert(str, char)
        char = file:read(1)
      end

      return table.concat(str)
    end
  end
end

function GoToByte(file, b)
  if file then
    if b then
      file:seek("set", b)
    else
      file:seek("set")
    end
  end
end

function DecodeBytes(str, fmt, first)
  local decoded, offset = string.unpack(fmt, str, first)

  return decoded, offset
end

--function DecodeBytes(str, fmt, first, tohexa)
--  if not str or not fmt then return end

--  local status, decoded, offset = pcall(string.unpack, fmt, str, first)

--  if not status then
--    print("ERROR in DecodeBytes:", decoded)
--    return nil, first
--  end

--  if tohexa then 
--    return PrintHexa(decoded, offset-first), offset
--  end

--  return decoded, offset
--end