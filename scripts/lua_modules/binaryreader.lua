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
    return tonumber(fmt:sub(fmt:find("c")+1))
  elseif fmt:find("z") then
    return
  else
    return string.packsize(fmt)
  end
end

function ReadBytes(file, fmt, tohexa)
  if file then
    local size = GetPackSize(fmt)

    if fmt:find("c") then
--      print("reading string of length", size, "starting at", file:seek("cur"))

      if size then
        local str = file:read(size)
        return str
      end
    elseif fmt:find("z") then
      local char = file:read(1)
      local str = {}

      while char ~= '\0' do
        table.insert(str, char)
        char = file:read(1)
      end

      return table.concat(str)
    else
--      print("reading data of length", size, "starting at", file:seek("cur"))

      if size then
        local bdata = file:read(size)
        local data = string.unpack(fmt, bdata)

        if tohexa and math.floor(data) == data then
          return PrintHexa(data, size)
        else
          return data
        end
      end
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

function DecodeBytes(str, fmt, first, tohexa)
  if not str or not fmt then return end

  if not first then first = 1 end

  local decoded, offset = string.unpack(fmt, str:sub(first))

  if tohexa then 
    return PrintHexa(decoded, offset-first), (first+offset-1)
  end

  return decoded, (first+offset-1)
end


-- Before having string.pack and unpack from lua 5.3

--local binreader = {}

--function binreader.OpenBinaryFile(fname)
--	local binfile	
--	binfile = assert(io.open(fname, "rb"))

--	return binfile
--end

--function binreader.ExtractBytes(input, first, last)	
--	return table.pack(input:byte(first,last))
--end

--function binreader.ReadBytes(size, input)
--	local str = input:read(size)

--	if str == nil then return nil end

--	return binreader.ExtractBytes(str, 1, size)
--end

--function binreader.InvertBytes(data, size)
--	for i=1, math.floor(#data / 2) do
--		data[i], data[#data - i + 1] = data[#data - i + 1], data[i]
--	end

--	return data

--	-- local newdata = InvertEndianness(data, size)
--	-- return newdata
--end

----------------------------------------------------------------

--function binreader.ToBytes(x, size, ordering)
--	if ordering == nil then
--		ordering = 1 -- 1 == strong byte first / 0 == weak byte first
--	end

--	local output = {}

--	if type(x) == "boolean" then
--		x = x and 1 or 0
--	elseif type(x) == "string" then
--		return x
--	elseif type(x) ~= "number" then
--		print("ERROR: cannot convert " .. type(x) .. " to bytes...")
--		return nil
--	end

--	for i=1,size do
--		if ordering then 
--			-- output[size-i+1] = x%256
--			output[size-i+1] = (x >> (8*(i-1))) & 0xFF
--		else
--			-- output[i] = x%256
--			output[i] = (x >> (8*(i-1))) & 0xFF
--		end
--	end

--	local str = string.char(table.unpack(output))

--	return str
--end

--function binreader.BoolToBytes(x, ordering)
--	x = x and 1 or 0

--	return string.char(x)
--end

--function binreader.IntegerToBytes(x, size, ordering)
--	if ordering == nil then
--		ordering = 1 -- 0 == strong byte first / 1 == weak byte first
--	end

--	if x > 0x1FFFFFFFFFFFFF then
--		print("ERROR while converting value to bytes => value too large : " .. tostring(x))
--		return
--	end

--	local output = {}

--	for i=1,size do
--		if not ordering then 
--			-- output[size-i+1] = x%256
--			output[size-i+1] = (x >> (8*(i-1))) & 0xFF
--		else
--			-- output[i] = x%256
--			output[i] = (x >> (8*(i-1))) & 0xFF
--		end
--	end

--	-- print(table.unpack(output))

--	return string.char(table.unpack(output))
--end

--local function GetSize(precision)
--	if precision <= 2 then
--		return 1
--	elseif precision <= 4 then
--		return 2
--	elseif precision <= 7 then
--		return 3
--	elseif precision <= 9 then
--		return 4
--	elseif precision <= 12 then
--		return 5
--	elseif precision <= 14 then
--		return 6
--	elseif precision <= 16 then
--		return 7
--	elseif precision <= 19 then
--		return 8
--	else
--		print("ERROR: precision for floating points set too high. Fall back to 19")
--		precision = 19
--		return 8
--	end
--end

--function binreader.FloatToBytes(x, precision, ordering)
--	if ordering == nil then
--		ordering = 1 -- 0 == strong byte first / 1 == weak byte first
--	end

--	if x > 0x1FFFFFFFFFFFFF then
--		print("ERROR while converting value to bytes => value too large : " .. tostring(x))
--		return
--	end

--	local int_part = binreader.IntegerToBytes( math.floor(x), 8, ordering )
--	local float_part = binreader.IntegerToBytes( math.floor((x - math.floor(x)) * 10^precision), GetSize(precision), ordering )

--	return int_part, float_part
--end

--function binreader.FromBytes(src, type, size, ordering)
--	if type == "string" then return src end

--	local buff = table.pack(src:byte(1, size))

--	if type == "boolean" then
--		return buff[1] and true or false
--	elseif type == "integer" then
--		local result = (buff[4] + (buff[3] << 8) + (buff[2] << 16) + (buff[1] << 24))
--		return (result < 0x7fffffff) and result or -(result - (result & 0x7fffffff)*2)
--	elseif type == "number" then
--		return (buff[8] + (buff[7] << 8) + (buff[6] << 16) + (buff[5] << 24) + (buff[4] << 32) + (buff[3] << 40) + (buff[2] << 48) + (buff[1] << 56))
--	end
--end

--function binreader.BytesToBool(src)
--	return src[1] > 0
--end

--function binreader.BytesToShort(src)
--	local result = ((src[1] or 0) + ((src[2] or 0) << 8))
--	return (result < 0x7fff) and result or -(result - (result & 0x7fff)*2)
--end

--function binreader.BytesToUShort(src)
--	return ((src[1] or 0) + ((src[2] or 0) << 8))
--end

--function binreader.BytesToInt(src)
--	local result = ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24))
--	return (result < 0x7fffffff) and result or -(result - (result & 0x7fffffff)*2)
--end

--function binreader.BytesToUInt(src)
--	return ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24))
--end

--function binreader.BytesToLInt(src)
--	local result = ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24))
--	return (result < 0x7fffffff) and result or -(result - (result & 0x7fffffff)*2)
--end

--function binreader.BytesToULInt(src)
--	return ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24))
--end

--function binreader.BytesToLLInt(src)
--	local result = ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24) + ((src[5] or 0) << 32) + ((src[6] or 0) << 40) + ((src[7] or 0) << 48) + ((src[8] or 0) << 56))
--	return (result < 0x7fffffffffffffff) and result or -(result - (result & 0x7fffffffffffffff)*2)
--end

--function binreader.BytesToULLInt(src)
--	return ((src[1] or 0) + ((src[2] or 0) << 8) + ((src[3] or 0) << 16) + ((src[4] or 0) << 24) + ((src[5] or 0) << 32) + ((src[6] or 0) << 40) + ((src[7] or 0) << 48) + ((src[8] or 0) << 56))
--end

--function binreader.ReadBinData(src, size, invert)	
--	local data = binreader.ReadBytes(size, src)

--	if data == nil or #data == 0 then return nil end

--	if invert then 
--		data = binreader.InvertBytes(data, size)
--	end

--	if size == 2 then
--		return binreader.BytesToUShort(data)
--	elseif size == 4 then
--		return binreader.BytesToUInt(data)
--	elseif size == 8 then
--		return binreader.BytesToULLInt(data)
--	else	
--		return nil
--	end
--end

---- function BytesToFloat(src)
--	-- local sign = 1
--	-- local mantissa = string.byte(x, 3) % 128
--	-- for i = 2, 1, -1 do mantissa = mantissa * 256 + string.byte(x, i) end
--	-- if string.byte(x, 4) > 127 then sign = -1 end
--	-- local exponent = (string.byte(x, 4) % 128) * 2 + math.floor(string.byte(x, 3) / 128)
--	-- if exponent == 0 then return 0 end
--	-- mantissa = (math.ldexp(mantissa, -23) + 1) * sign
--	-- return math.ldexp(mantissa, exponent - 127)
---- end

---- function BytesToDouble(src)
--	-- local sign = 1
--	-- local mantissa = string.byte(x, 7) % 15
--	-- for i = 6, 1, -1 do mantissa = mantissa * 256 + string.byte(x, i) end
--	-- if string.byte(x, 8) > 127 then sign = -1 end
--	-- local exponent = (string.byte(x, 8) % 128) * 2 + math.floor(string.byte(x, 7) / 15)
--	-- if exponent == 0 then return 0 end
--	-- mantissa = (math.ldexp(mantissa, -23) + 1) * sign
--	-- return math.ldexp(mantissa, exponent - 127)
---- end

----------------------------------------------------------------

--function binreader.DecodeBinary(data, startb, endb, bitshift)
--	local res

--	if endb <= 15 then
--		res =  binreader.BytesToUShort(data)
--	elseif endb <= 31 then
--		res =  binreader.BytesToUInt(data)
--	elseif endb <= 63 then
--		res =  binreader.BytesToULLInt(data)
--	else	
--		return 0
--	end

--	local bitmask

--	if endb <= 31 then
--		bitmask = 2^(endb+1) - 1
--		res = (res & bitmask) >> startb
--	else
--		bitmask = 2^32 - 1
--		local fst4b = res & bitmask
--		bitmask = 2^(endb-32) - 1
--		res = (fst4b + (((res >> 32) & bitmask) << 32)) >> startb
--	end

--	return (bitshift >= 0) and (res << bitshift) or (res >> bitshift)
--end

--function binreader.TestFunction(input, size, type, ordering)
--	if ordering == nil then
--		ordering = 1
--	end

--	local tobytes = binreader.IntegerToBytes(input, size)

--	print("Conversion of " .. tostring(input) .. " to bytes: " .. tostring(tobytes))

--	local bytes_tbl = table.pack(tobytes:byte(1,size))

--	if type == "bool" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToBool(bytes_tbl)))
--	elseif type == "unsigned short" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToUShort(bytes_tbl)))	
--	elseif type == "short" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToShort(bytes_tbl)))	
--	elseif type == "unsigned int" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToUInt(bytes_tbl)))	
--	elseif type == "int" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToInt(bytes_tbl)))	
--	elseif type == "unsigned long long int" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToULLInt(bytes_tbl)))
--	elseif type == "long long int" then
--		print("Backward conversion result: " .. tostring(binreader.BytesToLLInt(bytes_tbl)))		
--	end

--	return nil
--end

--return binreader