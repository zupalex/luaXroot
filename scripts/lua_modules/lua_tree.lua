require("lua_helper")
local binreader = require("binaryreader")

local IntegerToBytes = binreader.IntegerToBytes
local ReadBytes = binreader.ReadBytes
local BytesToBool = binreader.BytesToBool
local BytesToUInt = binreader.BytesToUInt
local BytesToInt = binreader.BytesToInt
local BytesToULLInt = binreader.BytesToULLInt
local BytesToLLInt = binreader.BytesToLLInt
local BytesToUShort = binreader.BytesToUShort
local BytesToShort = binreader.BytesToShort
local FloatToBytes = binreader.FloatToBytes

local size_table = 
{
	boolean = 1,
	integer = 4,
	number = 8,
	string = 0,
	table = 0,
	array = 0,
}

local type_table = {
	boolean = { id = 1, size = 1 },
	integer = { id = 2, size = 4 },
	number = { id = 3, size = 8 },
	string = { id = 4, size = 0 },
	table = { id = 5, size = 0 },
	userclass = { id = 6, size = math.huge },
}

local invert_type_table = {}

for k, v in pairs(type_table) do
	invert_type_table[v.id] = k
end

local function GetFloatingPointValue(int_part, float_part, precision)
	return int_part + float_part * 10^(-precision)
end

local function GetTableCount(tbl)
	local count = 0
	
	for k, v in pairs(tbl) do
		count = count +1
	end
	
	return count
end

local LuaTree = LuaClass("LuaTree", "LuaObject", function(self, data)
	self.name = TryGetPar(data, "name", "lua_tree")
	self.title = TryGetPar(data, "title", "Lua Tree")
	
	self.structure = TryGetPar(data, "structure", {})
	self.buffer =  TryGetPar(data, "buffer", {})
	self.bindata = TryGetPar(data, "data", {})
	
	self.data = {}
	
	self.defaults = {}
	
	self.header = TryGetPar(data, "header", {})
	self.streamers = TryGetPar(data, "streamer", {})
	
	self.precision = TryGetPar(data, "precision", 4)
	self.evtnbr = 0
	
	self.outputfile = TryGetPar(data, "output", nil)
	self.inputfile =  TryGetPar(data, "input", nil)
	
	self._header_initialized = nil
	self._header_recorded = nil
	self._databegin = nil
	self._adjust = nil
end)

function LuaTree:SetOutput(file)
	self.outputfile = io.open(file, "wb")
end

function LuaTree:SetInput(file, load)
	self.inputfile = io.open(file, "rb")
	
	if load == nil or load == true then 
		self:ReadAndLoadFileHeader() 
	end
end

local function GetSizeAdjust(self)
	if self.precision <= 2 then
		return 1
	elseif self.precision <= 4 then
		return 2
	elseif self.precision <= 7 then
		return 3
	elseif self.precision <= 9 then
		return 4
	elseif self.precision <= 12 then
		return 5
	elseif self.precision <= 14 then
		return 6
	elseif self.precision <= 16 then
		return 7
	elseif self.precision <= 19 then
		return 8
	else
		print("ERROR: precision for floating points set too high. Fall back to 19")
		self.precision = 19
		return 8
	end
end

function LuaTree:MakeStreamers(tbl)
	if type(tbl) ~= "table" then
		return
	end

	if self.streamers[tbl] == nil then
		-- print("Preaparing streamer for " .. tostring(tbl))
		self.streamers[tbl] = {}
		
		local ssize = 0
		local has_variable_array = false
		
		for k, v in pairs(tbl) do
			if type(v) == "table" and v.value ~= nil then
				local typename_, typeid_
			
				if type(v.value) ~= "table" then										
					local effective_size = type_table[v.type].size or 0
					
					if v.type == "number" then			
						effective_size = effective_size + self._adjust
					end
					
					if v.array and v.fixedsize ~= nil then
						effective_size = effective_size*v.fixedsize
					elseif v.array and v.fixedsize == nil then
						effective_size = 0
						has_variable_array = true
					end
					
					table.insert(self.streamers[tbl], { name = k, typename = v.type, typeid = type_table[v.type].id, is_array = v.array and true or false,
														streamer = v.value, size = effective_size })
					
					ssize = ssize + effective_size
				else
					if self.streamers[v.value] == nil then
						self:MakeStreamers(v.value)
					end
					
					local tsize = self.streamers[v.value].size or 0
					
					local effective_size = tsize
					
					if effective_size == 0 then
						has_variable_array = true
					else
						if v.array and v.fixedsize ~= nil then
							effective_size = effective_size*v.fixedsize
						elseif v.array and v.fixedsize == nil then
							has_variable_array = true
							effective_size = 0
							has_variable_array = true
						end
					end
					
					table.insert(self.streamers[tbl], { name = k, typename = v.type, typeid = type_table[v.type].id, is_array = v.array and true or false, streamer = self.streamers[v.value], size = effective_size })
					
					ssize = ssize + effective_size
				end
			end
		end
		
		self.streamers[tbl].size = has_variable_array and 0 or ssize
	end
end

function LuaTree:MakeMainHeader(tbl)
	if tbl == nil then 
		tbl = self.structure
	end
	
	self:MakeStreamers(tbl)
	self.header = self.streamers[tbl]
	
	self._header_initialized = true
end

function LuaTree:ConstructDefaults(tbl, recreate)
	if recreate == nil or recreate == true then
		self.defaults = {}
	end

	local defs = {}
	
	for k, v in pairs(tbl) do
		if type(v) ~= "table" then
			defs[k] = v
		elseif v.is_a ~= nil and v.is_a("LuaObject") then 
			LuaTree.ConstructDefaults(self, v.serializable, false)
		elseif type(v) == "table" then
			if v._data == nil then
				LuaTree.ConstructDefaults(self, v, false)
			end
		end
	end
	
	local empty = true
	
	for k, v in pairs(defs) do
		if v ~= nil then
			empty = false
			break
		end
	end
	
	if not empty then self.defaults[tbl] = defs end
end

function LuaTree:WriteFileHeader()
	if self.outputfile == nil then
		print("ERROR in LuaTree:WriteHeader() -> no output file associated to tree")
		return
	end
	
	self.outputfile:write("<FileHeader>")
	
	self.outputfile:write("<TreeInfo>")
	
	-- Write first the name of the tree --
	self.outputfile:write(IntegerToBytes(self.name:len(), 2))
	self.outputfile:write(self.name)
	
	-- Then the title of the tree --
	self.outputfile:write(IntegerToBytes(self.title:len(), 2))
	self.outputfile:write(self.title)
	
	-- Record the precision used --
	self.outputfile:write(IntegerToBytes(self.precision, 1))
	
	self.outputfile:write("</TreeInfo>")
	
	self.outputfile:write("<Streamers>")
	
	-- Sort the streamers
	local ordered_streamers = {}
	
	for k, streamer in pairs(self.streamers) do
		table.insert(ordered_streamers, streamer)
	end
	
	-- Write how many streamers we have
	self.outputfile:write(IntegerToBytes(#ordered_streamers, 2))
	
	for i, streamer in ipairs(ordered_streamers) do
		self.outputfile:write("<Streamer#" .. IntegerToBytes(i, 2) .. ">")
		-- Write the streamer size
		self.outputfile:write(IntegerToBytes(streamer.size, 2))
		
		for _, v in ipairs(streamer) do
			-- Write the name of the element
			self.outputfile:write(IntegerToBytes(v.name:len(), 2))
			self.outputfile:write(v.name)
			
			-- Write the typeid of the element
			self.outputfile:write(IntegerToBytes(v.typeid, 1))
			
			-- Write if element is an array or not
			self.outputfile:write(IntegerToBytes(v.is_array and 1 or 0, 1))
			
			-- Write the effective size of the element
			self.outputfile:write(IntegerToBytes(v.size, 2))
			
			-- Write the streamer (default value in case of a number or boolean, other streamer number in case of a table)
			if v.typename == "table" then
				for j, s in ipairs(ordered_streamers) do
					if v.streamer == s then
						self.outputfile:write(IntegerToBytes(j, 2))
						break
					end
				end
			elseif v.typename == "boolean" then
				self.outputfile:write(IntegerToBytes(v.streamer and 1 or 0, 1))
			elseif v.typename == "integer" then
				self.outputfile:write(IntegerToBytes(v.streamer, 4))
			elseif v.typename == "number" then
				local int_part, float_part = FloatToBytes(v.streamer, self.precision)
				self.outputfile:write(int_part)
				self.outputfile:write(float_part)
			end
		end
		self.outputfile:write("</Streamer#" .. IntegerToBytes(i, 2) .. ">")
	end
	
	self.outputfile:write("</Streamers>")
	
	-- Then serialize the structure --
	self.outputfile:write("<TreeStructure>")
	
	-- Write which streamer correspond to the root of our
	for i, s in ipairs(ordered_streamers) do
		if s == self.header then
			self.outputfile:write(IntegerToBytes(i, 2))
		end
	end
	
	self.outputfile:write("</TreeStructure>")	
	self.outputfile:write("</FileHeader>")
	
	self._header_recorded = true
end

function LuaTree:ReadString(field)
	if self.inputfile == nil then
		print("ERROR in LuaTree:LoadName() -> no input file associated to tree")
		return
	end
	
	local buffer = ReadBytes(2, self.inputfile)
	local field_length = BytesToUShort(buffer)
	
	print(tostring(field) .. " length = " .. tostring(field_length))
	
	buffer = self.inputfile:read(field_length)
	
	print(tostring(field) .. " value = " .. tostring(buffer))
	
	if field ~= nil then self[field] = buffer end
	
	return buffer
end

local function BreakDownBranchPath(bname)
	local bpath = {}
	
	for element in bname:gmatch("([^%.]*)") do
		table.insert(bpath, element)
	end
	
	if #bpath == 0 then
		table.insert(bpath, bname)
	end
	
	return bpath
end

function LuaTree:RebuildBaseTables(init)
	if init then self._basetables = {} end
	
	print("Rebuilding base tables........")
	print("Streamers count: " .. #self._ordered_streamers)
	
	while GetTableCount(self._basetables) < #self._ordered_streamers do
		for _, streamer in ipairs(self._ordered_streamers) do
			if self._basetables[streamer] == nil then
				local create_new = true
				
				for i, v in ipairs(streamer) do
					if v.typename == "table" and self._basetables[self._ordered_streamers[v.streamer]] == nil then
						create_new = false
					end
				end
				
				if create_new then
					print("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%")
					print("Found a base table to rebuild:")
					local base_table = {}
														
					for i, v in ipairs(streamer) do
						local fixsize = nil
					
						if v.typename ~= "table" then
							if v.is_array and v.size ~= 0 then
								fixsize = math.floor(v.size/(type_table[v.typename].size + (v.typename == "number" and self._adjust or 0)))
								fixsize = fixsize ~= 0 and fixsize or nil
							end
						
							base_table[v.name] = { type = v.typename, value = v.streamer, array = v.is_array and true or false, fixedsize = fixsize}
						else
							local base = self._basetables[self._ordered_streamers[v.streamer]]
							
							base_table[v.name] = { type = v.typename, value = base, array = v.is_array and true or false}
						end
						print("Var name = " .. tostring(v.name) .. " => type = " .. tostring(v.typename) .. " / value = " .. tostring(base_table[v.name].value))
					end
					
					print("......................")
					
					self._basetables[streamer] = base_table
				end
			end
		end
	end
	
	print("=======================================================================")
	print("Checking to replace the streamer index by the appropriate table")
	for k, streamer in pairs(self.streamers) do
		print("Checking streamer " .. tostring(streamer))
		for _, v in ipairs(streamer) do
			if v.typename == "table" then
				print("Found element " .. tostring(v.name) .. " requiring a fix for the streamer: " .. tostring(v.streamer) .. " => " .. tostring(self._ordered_streamers[v.streamer]))
				v.streamer = self._ordered_streamers[v.streamer]
				
				if v.is_array and v.size ~= 0 and v.streamer.size ~= 0 then
					self._basetables[streamer][v.name].fixedsize = math.floor(v.size/v.streamer.size)
				end
			end
		end
		
		print("Switching the key of the streamers table from " .. tostring(k) .. " to " .. tostring(self._basetables[streamer]))
		k = self._basetables[streamer]
	end
	
	printtable(self._basetables, true, nil, nil, 10)
end

function LuaTree:ReloadStreamers()
	-- Read how many streamers we have
	local buffer = ReadBytes(2, self.inputfile)
	local nstreamers = BytesToUShort(buffer)
	
	self._ordered_streamers = {}
	
	for i=1,nstreamers do
		local new_streamer = {}
		
		buffer = self.inputfile:read(10)
		
		if buffer ~= "<Streamer#" then
			print("Malformed streamer header: missing streamer#... aborting...")
			return false
		end
		
		buffer = ReadBytes(2, self.inputfile)
		local streamer_nbr = BytesToUShort(buffer)
		
		print("*************************************************")
		print("Reading Streamer #" .. tostring(streamer_nbr))
		
		buffer = self.inputfile:read(1)
		
		if buffer ~= ">" then
			print("Malformed streamer header after reading streamer #... aborting...")
			return false
		end
		
		-- Read the streamer size
		buffer = ReadBytes(2, self.inputfile)
		local streamer_size = BytesToUShort(buffer)
		new_streamer.size = streamer_size
		
		print("Streamer size: " .. tostring(streamer_size))
		
		buffer = self.inputfile:read(11)
		
		local proceed = buffer ~= "</Streamer#"
		
		while proceed do
			self.inputfile:seek("cur", -11)
			
			local new_streamer_element = {}
			
			-- Read the var name length and var name
			buffer = ReadBytes(2, self.inputfile)
			local var_name_len = BytesToUShort(buffer)
			local var_name = self.inputfile:read(var_name_len)
			print("------------------------------")
			print("Element name: " .. tostring(var_name))
			
			new_streamer_element.name = var_name
			
			-- Read the var size
			buffer = ReadBytes(1, self.inputfile)
			local var_typeid = BytesToUShort(buffer)
			
			print("Element type id: " .. tostring(var_typeid))
			
			new_streamer_element.typeid = var_typeid
			
			-- Read if the var is an array
			buffer = ReadBytes(1, self.inputfile)
			local var_is_array = BytesToBool(buffer)
			
			print("Element is an array: " .. tostring(var_is_array))
			
			new_streamer_element.is_array = var_is_array
			
			-- Read the var effective size
			buffer = ReadBytes(2, self.inputfile)
			local var_eff_size = BytesToUShort(buffer)
			new_streamer_element.size = var_eff_size
			
			print("Element effective size: " .. tostring(var_eff_size))
			
			local var_type = invert_type_table[var_typeid]
			new_streamer_element.typename = var_type
			
			print("Element type: " .. tostring(var_type))
			
			local var_streamer
			
			-- Read the var streamer (default value in case of a number or boolean, other streamer number in case of a table)
			if var_type == "table" then
				buffer = ReadBytes(2, self.inputfile)
				var_streamer = BytesToUShort(buffer)
			elseif var_type == "boolean" then
				buffer = ReadBytes(1, self.inputfile)
				var_streamer = BytesToBool(buffer)
			elseif var_type == "integer" then
				buffer = ReadBytes(4, self.inputfile)
				var_streamer = BytesToInt(buffer)
			elseif var_type == "number" then
				buffer = ReadBytes(8, self.inputfile)
				local var_streamer_intp = BytesToLLInt(buffer)
				buffer = ReadBytes(self._adjust, self.inputfile)
				local var_streamer_floatp = BytesToULLInt(buffer)

				print("Element streamer integer part: " .. tostring(var_streamer_intp))
				print("Element streamer float part: " .. tostring(var_streamer_floatp))
				
				var_streamer = GetFloatingPointValue(var_streamer_intp, var_streamer_floatp, self.precision)
			end
			
			new_streamer_element.streamer =  var_streamer
			
			print("Element streamer: " .. tostring(var_streamer))
			
			table.insert(new_streamer, new_streamer_element)
			
			buffer = self.inputfile:read(11)
		
			proceed = buffer ~= "</Streamer#"
		end
		
		local tbl_key = {}
		
		self.streamers[tbl_key] = new_streamer
		table.insert(self._ordered_streamers, new_streamer)
		
		buffer = ReadBytes(2, self.inputfile)
		local streamer_nbr_footer = BytesToUShort(buffer)
		
		if streamer_nbr_footer~= streamer_nbr then
			print("Malformed streamer... aborting...")
			return
		end
		
		buffer = self.inputfile:read(1)
	end
end

function LuaTree:ReadAndLoadFileHeader()
	if self.inputfile == nil then
		print("ERROR in LuaTree:ReadFileHeader() -> no input file associated to tree")
		return false
	end
	
	local buffer = self.inputfile:read(12)
	
	if buffer ~= "<FileHeader>" then
		print("Malformed file header... aborting...")
		return false
	end
	
	buffer = self.inputfile:read(10)
	
	if buffer ~= "<TreeInfo>" then
		print("Malformed file header: missing tree info... aborting...")
		return false
	end
	
	-- Read the name of the tree and the title
	self:ReadString("name")
	self:ReadString("title")
	
	-- Read the precision
	buffer = ReadBytes(1, self.inputfile)
	self.precision = BytesToUShort(buffer)
	
	print("Precision = " .. self.precision)
	
	self._adjust = GetSizeAdjust(self)
	
	buffer = self.inputfile:read(11)
	
	if buffer ~= "</TreeInfo>" then
		print("Malformed file header: malformed tree info... aborting...")
		return false
	end
	
	buffer = self.inputfile:read(11)
	
	if buffer ~= "<Streamers>" then
		print("Malformed file header: missing streamers... aborting...")
		return false
	end
	
	self.streamers = {}
	self:ReloadStreamers()
	
	buffer = self.inputfile:read(12)
	
	if buffer ~= "</Streamers>" then
		print("Malformed file header: malformed streamers... aborting...")
		return false
	end
	
	buffer = self.inputfile:read(15)
	
	if buffer ~= "<TreeStructure>" then
		print("Malformed file header: missing tree structure... aborting...")
		return false
	end

	buffer = ReadBytes(2, self.inputfile)
	local header_streamer_link = BytesToUShort(buffer)
	
	buffer = self.inputfile:read(16)
	
	if buffer ~= "</TreeStructure>" then
		print("Malformed file header: malformed tree structure... aborting...")
		return false
	end

	buffer = self.inputfile:read(13)

	if buffer ~= "</FileHeader>" then
		print("Malformed file header: missing file header footer... aborting...")
		return false
	end
	
	self:RebuildBaseTables(true)
	
	self.header = self._ordered_streamers[header_streamer_link]
	
	self.structure = self._basetables[self.header]
	
	self:SetupBuffer()
	
	self._databegin = self.inputfile:seek("cur")
	
	-- self:ConstructDefaults(self.buffer)
	
	self._header_initialized = true
	
	return true
end

function LuaTree:SetStructure(struct)
	self.structure = struct
	
	self._adjust = GetSizeAdjust(self)
	
	for k, v in pairs(self.structure) do
		self:MakeStreamers(v.value)
	end
	
	self:SetupBuffer()
	
	self:MakeMainHeader()
end

function LuaTree:ResetValues(tbl)
	if tbl._data ~= nil then
		tbl._data = {}
	else
		for k, v in pairs(tbl) do
			if type(v) ~= "table" then
				if self.defaults[tbl] then
					tbl[k] = self.defaults[tbl][k]
				end
			elseif v.is_a ~= nil and v.is_a("LuaObject") then 
				LuaTree.ResetValues(self, v.serializable)
			else
				LuaTree.ResetValues(self, v)
			end
		end
	end
end

local function MakeVectorLink(tree, buffer, key)
	if tree._vectorlinks == nil then
		tree._vectorlinks = {}
	end
	
	-- tree._vectorlinks[]
end

local function HandleBufferCopy(tree, buffer, key, val)
	if type(key) ~= "string" then
		print("Invalid key in tree structure: " .. tostring(key))
		return nil
	end

	if type(val) == "table" and val.value ~= nil then
		if val.array then
			buffer[key] = {}
			
			if val.fixedsize ~= nil then
				for i=1,val.fixedsize do
					if type(val.value) ~= "table" then
						table.insert(buffer[key], val.value)
					else
						local cpy = {}
			
						for k, v in pairs(val.value) do
							HandleBufferCopy(tree, cpy, k, v)
						end
						
						table.insert(buffer[key], cpy)
					end
				end
			else
				
			end
		else
			if type(val.value) ~= "table" then
				buffer[key] = val.value
			else
				local cpy = {}
			
				for k, v in pairs(val.value) do
					HandleBufferCopy(tree, cpy, k, v)
				end
				
				buffer[key] = cpy
				
				-- if val.type == "array" then			
					-- buffer[key] = {}
					
					-- cpy["_data"] = buffer[key]
					-- buffer["_" .. tostring(key)] = cpy
				-- else
					-- buffer[key] = cpy
				-- end
			end
		end
	elseif val.is_a ~= nil and val.is_a("LuaObject") then
		local cpy = {}
		
		for k, v in pairs(val.serializable) do
			HandleBufferCopy(tree, cpy, k, v)
		end
		
		buffer[key] = cpy	
	else
		print("Invalid value in tree structure: " .. tostring(val))
		return nil
	end
end

function LuaTree:SetupBuffer()
	self.buffer = {}
	
	for k, v in pairs(self.structure) do
		print("Copying key " .. tostring(k) .. " => " .. tostring(v))
		HandleBufferCopy(self, self.buffer, k, v)
	end
end

function LuaTree:LoadFile(file)

end

local tabulation_ = ""

function LuaTree:SerializeTable(tbl, src, dest)
	local table_size = 0

	-- print("|-" .. tabulation_ .. "> Serializing " .. tostring(tbl))
	for i, v in ipairs(tbl) do
		local streamer = v.streamer
		local array_size = v.is_array and #(src[v.name]) or 1
		
		local data_size_pos
		local data_size
		
		if v.size == 0 then 
			table.insert(dest, "") 
			table_size = table_size + 2
			data_size_pos = #dest
			data_size = table_size
		end
		
		table.insert(dest, IntegerToBytes(array_size, 1))
		table_size = table_size + 1
		
		if type(streamer) ~= "table" then				
			local serialized_data
			
			for i=1,array_size do
				local data
				
				if v.is_array then
					data = src[v.name][i]
				else
					data = src[v.name]
				end
				
				-- print("|-" .. tabulation_ .. "> Writing value(s) of " .. v.name .. " ( " .. ((v.is_array and "") or "not an ") .. "array ) => " .. tostring(data))
				
				if v.typeid == 1 then
					serialized_data = IntegerToBytes(data and 1 or 0, 1)
					table_size = table_size+1
				elseif v.typeid == 2 then
					serialized_data = IntegerToBytes(data, 4)
					table_size = table_size+4
				elseif v.typeid == 3 then
					local int_part = math.floor(data)
					local float_part = math.floor( (data - math.floor(data)) * (10^self.precision) )
					serialized_data = IntegerToBytes(int_part, 8)	.. IntegerToBytes(float_part, self._adjust)
					table_size = table_size+8+self._adjust
				else
					print("ERROR in LuaTree:SerializeTable -> type non serializable")
				end
				
				table.insert(dest, serialized_data)
				
				if v.size ~= 0 then -- either it's not an array or it is an array with a fxed size. In this case we restore the default value
					data = streamer 
				end
			end
			
			if v.size == 0 then -- it's an array of variable size
			
			end
		else
			for i=1,array_size do
				if v.is_array then
					data = src[v.name][i]
				else
					data = src[v.name]
				end
				
				-- print("|-" .. tabulation_ .. "> Writing value(s) of " .. v.name .. " ( " .. ((v.is_array and "") or "not an ") .. "array ) => " .. tostring(data))
				
				local prev_tabulation_ = tabulation_
				tabulation_ = tabulation_ .. "----"
				
				table_size = table_size + self:SerializeTable(streamer, data, dest)
				
				tabulation_ = prev_tabulation_
			end
			
			if v.size == 0 then -- it's an array of variable size
			
			end
		end
		
		if data_size_pos ~= nil then
			data_size = table_size - data_size
			dest[data_size_pos] = IntegerToBytes(data_size, 2)
			-- print("|-" .. tabulation_ .. "> Writen array of variable size: " .. tostring(data_size))
		end
	end
	
	return table_size
end

function LuaTree:SerializeTree(dest)
	return self:SerializeTable(self.header, self.buffer, dest)
end

function LuaTree:Fill()
	table.insert(self.bindata, "") -- placeholder for the size of the event
	local evt_size_pos = #self.bindata
	
	local evt_size = self:SerializeTree(self.bindata)
	self.bindata[evt_size_pos] = IntegerToBytes(evt_size, 4)
	
	-- self:ResetValues(self.buffer)
	
	self.evtnbr = self.evtnbr + 1
end

function LuaTree:Flush()
	if self.outputfile == nil then
		print("ERROR in LuaTree:Flush() -> no output file associated to tree")
		return
	end
	
	if not self._header_recorded then 
		self:WriteFileHeader()
	end
	
	self.outputfile:write(table.concat(self.bindata))
	
	self.bindata = {}
end

function LuaTree:Close()
	print("Closing the tree")

	if self.outputfile ~= nil then
		print("Total entries: " .. tostring(math.floor(self.evtnbr)))
		self.outputfile:write("<nentries>")
		self.outputfile:write(IntegerToBytes(math.floor(self.evtnbr), 8))
		self.outputfile:write("</nentries>")
		self.outputfile:close() 
	end
	
	if self.inputfile ~= nil then 
		self.inputfile:close() 
	end
end

local function GetEntryLength(tree)
	local buffer = ReadBytes(4, tree.inputfile)
	return BytesToUInt(buffer)
end

function LuaTree:SkipEntry()
	if self.inputfile == nil then
		print("ERROR in LuaTree:ReadFileHeader() -> no input file associated to tree")
		return false
	end
	
	local evt_length = GetEntryLength(self)
	
	local success, err = self.inputfile:seek("cur", evt_length)
	
	if success == nil then
		print("Failed to skip entry: " .. tostring(err))
	end
end

local function ReadEntry(tree, src, dest)
	-- tree:ResetValues(tree.buffer)
		
	for i, v in ipairs(src) do
		-- print("Reading data for " .. tostring(v.name) .. " / type = " .. v.typename .. " / typeid = " .. v.typeid)
	
		local buffer
		local datasize

		if v.size == 0 then
			buffer = ReadBytes(2, tree.inputfile)
			datasize = BytesToUShort(buffer)
			dest[v.name] = {}
		end
		
		buffer = ReadBytes(1, tree.inputfile)
		local arraysize = BytesToUShort(buffer)
		
		-- print("Read array size: " .. tostring(arraysize))
		
		local value
		
		for i=1,arraysize do
			if v.typeid == 1 then
				buffer = ReadBytes(1, tree.inputfile)
				value = BytesToBool(buffer)
				if v.is_array then
					dest[v.name][i] = value
				else
					dest[v.name] = value
				end
			elseif v.typeid == 2 then
				buffer = ReadBytes(4, tree.inputfile)
				value = BytesToInt(buffer)
				if v.is_array then
					dest[v.name][i] = value
				else
					dest[v.name] = value
				end
			elseif v.typeid == 3 then
				buffer = ReadBytes(8, tree.inputfile)
				local val_int = BytesToLLInt(buffer)
				
				buffer = ReadBytes(tree._adjust, tree.inputfile)
				local val_float = BytesToULLInt(buffer)
				
				value = GetFloatingPointValue(val_int, val_float, tree.precision)
				
				if v.is_array then
					dest[v.name][i] = value
				else
					dest[v.name] = value
				end
			elseif v.typeid == 5 then
				-- print("Streamer is a table. Filling next level...")
				-- printtable(v.streamer, true, nil, nil, 10)
				
				if v.is_array then
					if v.size == 0 then
						dest[v.name][i] = {}
					end
				elseif dest[v.name] == nil then
					dest[v.name] = {}
				end
				
				ReadEntry(tree, v.streamer, v.is_array and dest[v.name][i] or dest[v.name])
			end
			
			-- if value ~= nil then print("Read value: " .. tostring(value)) end
		end
	end
end

function LuaTree:GetEntry(entry)
	if self.inputfile == nil then
		print("ERROR in LuaTree:ReadFileHeader() -> no input file associated to tree")
		return false
	end
	
	if self._header_initialized == nil then
		self:ReadAndLoadFileHeader()
	end
	
	self.inputfile:seek("beg", self._databegin)
	
	for i=1,entry do
		self:SkipEntry()
	end
	
	local evt_length = GetEntryLength(tree)
	ReadEntry(self, self.header, self.buffer)
end

function LuaTree:Next()
	if self._header_initialized == nil then
		self:ReadAndLoadFileHeader()
	end
	
	local evt_length = GetEntryLength(self)
	ReadEntry(self, self.header, self.buffer)
end

function LuaTree:GetEntries()
	if self.inputfile == nil then
		print("ERROR in LuaTree:ReadFileHeader() -> no input file associated to tree")
		return false
	end
	
	local current = self.inputfile:seek("cur")
	
	self.inputfile:seek("end", -29)
	local buffer = self.inputfile:read(10)
	
	if buffer ~= "<nentries>" then
		print("ERROR while looking for \"<nentries>\": malformed file footer")
		return
	end
	
	buffer = ReadBytes(8, self.inputfile)
	local nentries = BytesToULLInt(buffer)
	
	buffer = self.inputfile:read(11)
	
	if buffer ~= "</nentries>" then
		print("ERROR while looking for \"</nentries>\": malformed file footer")
		return
	end
	
	self.inputfile:seek("set", current)
	
	return nentries
end

function LuaTree:Draw(args)
	if self._header_initialized == nil then
		self:ReadAndLoadFileHeader()
	end
	
	local nentries = self:GetEntries()
	
	if args.plot == nil then
		print("ERROR in LuaTree:Draw => argument table must contains the variable to draw (plot = [something])")
	end
	
	if args.output == nil then
		print("ERROR in LuaTree:Draw => argument table must contains the info about the output histogram (hist = {xmin = [xmin], xmax = [xmax], nbinsx = [nbinsx]})")
	end
	
	local var = args.plot
	
	local bpath = BreakDownBranchPath(args.plot)
	
	local root_tbl = self.buffer
	
	if #bpath > 1 then
		for i = 1,#bpath-1 do
			root_tbl = root_tbl[bpath[i]]
		end
		
		var = bpath[#bpath]
	end
	
	THist({name = args.hist.name or "hist", title = args.hist.title or "hist", xmin = args.hist.xmin, xmax = args.hist.xmax, nbinsx = args.hist.nbinsx})
	
	for ev=1,nentries do
		
	end
end