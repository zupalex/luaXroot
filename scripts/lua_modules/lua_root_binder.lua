
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

    local _NewBranch = self.NewBranch
    function self:NewBranch(name, var_type)
      if type(name) == "table" then
        return _NewBranch(self, name)
      else
        return _NewBranch(self, {name=name, type=var_type})
      end
    end

    local _SetBranch = self.SetBranch
    function self:SetBranch(name, value)
      if type(name) == "table" then
        return _SetBranch(self, name)
      else
        return _SetBranch(self, {name=name, value=value})
      end
    end
  end)
