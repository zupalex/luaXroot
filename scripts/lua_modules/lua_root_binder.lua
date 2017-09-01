
local function LuaBranchesSetupImp(tree, leaves_table, placement)
	local newStructAddr = tree.newStructAddr
	local activeBranch = tree.activeBranch
	local leafsList = tree.leafsList
	local branchSize = tree.branchSize
	
	for bname, btype in pairs(leaves_table) do
		if type(btype) == "string" then
			if placement == nil then
				local nptr = NewLightUserData(btype)
				AddBranchToTTree(tree, bname .. "/" .. btype, nptr )
				tree.linktable[bname] = { data = nptr , type = btype }
			elseif placement == 0 then
				local nptr, dsize = NewLightUserData(btype)
				table.insert(newStructAddr, nptr)
				branchSize[#branchSize] = branchSize[#branchSize] + dsize
				placement = nptr
				leafsList[#leafsList] = leafsList[#leafsList] .. tostring(bname) .. "/" .. tostring(GetLeafSymbol(btype)) .. ":"
				tree.linktable[activeBranch[#activeBranch] .. "." .. bname] =  { data = nptr , type = btype }
			elseif placement ~= nil then
				local nptr, dsize = NewLightUserData(btype, placement)
				branchSize[#branchSize] = branchSize[#branchSize] + dsize
				placement = nptr
				leafsList[#leafsList] = leafsList[#leafsList] .. tostring(bname) .. "/" .. tostring(GetLeafSymbol(btype)) .. ":"
				tree.linktable[activeBranch[#activeBranch] .. "." .. bname] =  { data = nptr , type = btype }
			end
		elseif type(btype) == "table" then
			table.insert(activeBranch, bname)
			table.insert(leafsList, "")
			table.insert(branchSize, 0)
			LuaBranchesSetupImp(tree, btype, 0)
			leafsList[#leafsList] = leafsList[#leafsList]:sub(0, -2)
			AddBranchToTTree(tree, bname .. "/struct", newStructAddr[#newStructAddr], leafsList[#leafsList] )
			table.remove(activeBranch)
			table.remove(leafsList)
			table.remove(branchSize)
			table.remove(newStructAddr)
		else
		
		end
	end
end

local function FillTree(self, tbl, parent)
	for k, v in pairs(tbl) do
		local linkKey = k
		
		if parent ~= nil and type(parent) == "string" then
			linkKey = parent .. "." .. tostring(k)
		end
		
		if type(v) ~= "table" then
			local dptr = self.linktable[linkKey].data
			local dtype = self.linktable[linkKey].type
			
			print("Setting light userdata " .. tostring(linkKey))
			
			SetLightUserData(dptr, dtype, v)
		elseif self.linktable[linkKey] ~= nil then
			local dptr = self.linktable[linkKey].data
			local dtype = self.linktable[linkKey].type
			
			for i, vval in ipairs(v) do
				print("Setting light userdata " .. tostring(linkKey) .. "[" .. tostring(i) .. "]")
			
				SetLightUserData(dptr, dtype, vval, i)
			end
		else
			FillTree(self, v, linkKey)
		end
	end
	
	if parent == nil then InvokeROOTFill(self) end
end

function InitTTree(name, title, tbl)
	local root_tree = TTree(name, title)
	
	root_tree.linktable = {}
	
	root_tree.newStructAddr = {}
	root_tree.activeBranch = {}
	root_tree.leafsList = {}
	root_tree.branchSize = {}
	
	LuaBranchesSetupImp(root_tree, tbl)
	
	root_tree.Fill = FillTree
	
	return root_tree
end
