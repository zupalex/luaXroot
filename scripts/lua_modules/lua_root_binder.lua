
-- Use this function to add stuffs to the metatable of a C++ Class --

function AddPostInit(class, fn)
  local _ctor = _G[class]
  _G[class] = function(...)
    local obj = _ctor(...)

    fn(obj)

    return obj
  end
end


AddPostInit("TTree", function(self)
    local _Draw = self.Draw
    function self:Draw(exp, cond, opts)
      if type(exp) == "table" then
        return _Draw(self, exp)
      else
        return _Draw(self, {exp=exp, cond=cond, opts=opts})
      end
    end

    local _GetEntry = self.GetEntry
    function self:GetEntry(entry, getall)
      if type(entry) == "table" then
        return _GetEntry(self, entry)
      else
        return _GetEntry(self, {entry=entry, getall=getall})
      end
    end

    local _NewBranch = self.NewBranch
    function self:NewBranch(name, var_type)
      if type(name) == "table" then
        return _NewBranch(self, name)
      else
        return _NewBranch(self, {name=name, type=var_type})
      end
    end

    local _GetBranch = self.GetBranch
    function self:GetBranch(name)
      if type(name) == "table" then
        return _GetBranch(self, name)
      else
        return _GetBranch(self, {name=name})
      end
    end

    function self:GetBranchList()
      local blist = {}
      for k, v in ipairs(self.branches_list) do
        blist[k] = v.userdata
      end
      
      return blist
    end
  end)
