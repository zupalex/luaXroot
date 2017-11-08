---------------------------------------------------------------------
------------------------------- TFile -------------------------------
---------------------------------------------------------------------

local function PrintTFileError()
  print("ERROR in TFile constructor")
  print("Required arguments:")
  print("  [name] -> a string")
  print("  [mode] -> the string handling how the file is opened (\"read\", \"update\" or \"recreate\")")
end

local _TFile = TFile
function TFile(args, ...)
  if type(args) == "table" then
    local name = args.name
    local mode = args.mode

    if name == nil or mode == nil then return PrintTFileError() end

    return _TFile(name, mode)
  elseif args == nil then
    return _TFile()
  else
    return _TFile(args, ...)
  end
end

---------------------------------------------------------------------
-------------------------------- TF1 --------------------------------
---------------------------------------------------------------------

local function PrintTF1Error()
  print("ERROR in TF1 constructor")
  print("Required arguments:")
  print("  [name] -> a string")
  print("  [formula] OR [fn] -> the formula of the function as a string OR directly the function itself")
  print("")
  print("if the field [fn] has been use then you also need")
  print("  [xmin] -> a number, lower bound of the function")
  print("  [xmax] -> a number, upper bound of the function")
  print("  [npars] -> a number, how many parameters for the function")
  print("")
  print("  (optional) [ndim] -> a number, how many dimensions for the function?")
end

local _TF1 = TF1
function TF1(args, ...)
  if type(args) == "table" then
    local name = args.name
    local formula = args.formula
    local fn = args.find
    local xmin = args.xmin
    local xmax = args.xmax
    local npars = args.npars
    local ndim = args.ndim

    if name == nil then return PrintTF1Error() end

    if formula ~= nil then
      if xmin ~= nil and xmax ~= nil then
        return _TF1(name, formula)
      else
        return _TF1(name, formula, xmin, xmax)
      end
    elseif fn ~= nil and npars ~= nil and xmin ~= nil and xmax ~= nil and type(fn) == "function" then
      local fn_id = _stdfunctions.unnamed+1
      local reg_name = "tf1fn_"..tostring(fn_id)
      RegisterTF1fn(reg_name, fn, npars)
      _stdfunctions.unnamed = fn_id

      if ndim ~= nil then
        return _TF1(name, reg_name, xmin, xmax, npars, ndim)
      else
        return _TF1(name, reg_name, xmin, xmax, npars)
      end
    else
      return PrintTF1Error()
    end
  elseif args == nil then
    return _TF1()
  else
    local other_args = table.pack(...)
    if type(other_args[1]) == "function" and other_args[4] ~= nil then
      local fn_id = _stdfunctions.unnamed+1
      local reg_name = "tf1fn_"..tostring(fn_id)
      RegisterTF1fn(reg_name, other_args[1], other_args[4])
      _stdfunctions.unnamed = fn_id

      if other_args[5] == nil then
        return _TF1(args, reg_name, other_args[2], other_args[3], other_args[4])
      else
        return _TF1(args, reg_name, other_args[2], other_args[3], other_args[4], other_args[5])
      end
    end

    return _TF1(args, ...)
  end
end

---------------------------------------------------------------------
---------------------------- TGraphErrors ---------------------------
---------------------------------------------------------------------

local function PrintTGraphErrorsError()
  print("ERROR in TGraph constructor")
  print("Required arguments:")
  print("  [n] -> a number, how many points in your graph")
  print("")
  print("  (optional) [xs] -> a 1D table, the abscissa. NEEDS TO CONTAIN EXACTLY n ELEMENTS")
  print("  (optional) [ys] -> a 1D table, the coordinates. NEEDS TO CONTAIN EXACTLY n ELEMENTS")
  print("  (optional) [dxs] -> a 1D table, the errors on the abscissa. NEEDS TO CONTAIN EXACTLY n ELEMENTS")
  print("  (optional) [dys] -> a 1D table, the errors on the coordinates. NEEDS TO CONTAIN EXACTLY n ELEMENTS")
  print("")
  print("Please note that if field [xs] is given, field [ys] is required (same goes for [dxs] and [dys])")
end

local _TGraph = TGraph
function TGraph(args, ...)
  if type(args) == "table" then
    local npoints = args.n
    local xs = args.xs
    local ys = args.ys
    local errxs = args.dxs
    local errys = args.dys

    if npoints ~= nil then
      if xs ~= nil and ys ~= nil then
        if dxs ~= nil and dys ~= nil then
          return _TGraph(n, xs, ys, dxs, dys)
        elseif dxs == nil and dys == nil then
          return _TGraph(n, xs, ys)
        else
          return PrintTGraphErrorsError()
        end
      elseif xs == nil and ys == nil and dxs == nil and dys == nil then
        return _TGraph(n)
      else
        return PrintTGraphErrorsError()
      end
    else
      return PrintTGraphErrorsError()
    end
  elseif args == nil then
    return _TGraph()
  else
    return _TGraph(args, ...)
  end
end

---------------------------------------------------------------------
-------------------------------- TH1 --------------------------------
---------------------------------------------------------------------

local function PrintTHErrorCommon()
  print("Required arguments:")
  print("  [name] -> a string, no space")
  print("  [title] -> a string")
  print("  [xmin] -> a number, lower bound of the X axis")
  print("  [xmax] -> a number, upper bound of the X axis")
  print("  [nbinsx] -> an integer, how many bins on the X axis")
end

local function PrintTH1DError()
  print("ERROR in TH1 constructor")
  PrintTHErrorCommon()
end

local _TH1 = TH1
function TH1(args, ...)
  if type(args) == "table" then
    local name = args.name
    local title = args.title
    local xmin = args.xmin
    local xmax = args.xmax
    local nbinsx = args.nbinsx

    if name ~= nil and title ~= nil and xmin ~= nil and xmax ~= nil and nbinsx ~= nil then
      return _TH1(name, title, nbinsx, xmin, xmax)
    else
      return PrintTH1DError()
    end
  elseif args == nil then
    return _TH1()
  else
    return _TH1(args, ...)
  end
end

AddPostInit("TH1", function(self)
    local _Fill = self.Fill
    function self:Fill(val, weight)
      if weight == nil then
        return _Fill(self, val, 1)
      else
        return _Fill(self, val, weight)
      end
    end

    local _Add = self.Add
    function self:Add(h, scale)
      if scale == nil then
        return _Fill(self, h, 1)
      else
        return _Fill(self, h, scale)
      end
    end
  end)

---------------------------------------------------------------------
-------------------------------- TH2 --------------------------------
---------------------------------------------------------------------

local function PrintTH2DError()
  print("ERROR in TH2 constructor")
  PrintTHErrorCommon()
  print("  [ymin] -> a number, lower bound of the Y axis")
  print("  [ymax] -> a number, upper bound of the Y axis")
  print("  [nbinsy] -> an integer, how many bins on the Y axis")
end

local _TH2 = TH2
function TH2(args, ...)
  if type(args) == "table" then
    local name = args.name
    local title = args.title
    local xmin = args.xmin
    local xmax = args.xmax
    local nbinsx = args.nbinsx
    local ymin = args.ymin
    local ymax = args.ymax
    local nbinsy = args.nbinsy

    if name ~= nil and title ~= nil and xmin ~= nil and xmax ~= nil and nbinsx ~= nil and ymin ~= nil and ymax ~= nil and nbinsy ~= nil then
      return _TH2(name, title, nbinsx, xmin, xmax, nbinsy, ymin, ymax)
    else
      return PrintTH2DError()
    end
  elseif args == nil then
    return _TH2()
  else
    return _TH2(args, ...)
  end
end

AddPostInit("TH2", function(self)
    local _Fill = self.Fill
    function self:Fill(valx, valy, weight)
      if weight == nil then
        return _Fill(self, valx, valy, 1)
      else
        return _Fill(self, valx, valy, weight)
      end
    end
  end)

---------------------------------------------------------------------
------------------------------- TTree -------------------------------
---------------------------------------------------------------------

local function PrintTTreeError()
  print("ERROR in TTree constructor")
  print("  [name] -> a string")
  print("  [title] -> a string")
end

local _TTree = TTree
function TTree(args, ...)
  if type(args) == "table" then
    local name = args.name
    local title = args.title

    if name ~= nil and title ~= nil then
      return _TTree(name, title)
    else
      return PrintTTreeError()
    end
  elseif args == nil then
    return _TTree()
  else
    return _TTree(args, ...)
  end
end

AddPostInit("TTree", function(self)
    self.branches_list = {}

    local _Draw = self.Draw
    function self:Draw(exp, cond, opts, nentries, firstentry)
      if type(exp) == "table" then
        local cond = exp.cond or ""
        local opts = exp.opts or ""
        local nentries = exp.nentries or 0
        local firstentry = exp.firstentry or 0
        local exp = exp.exp
        return _Draw(self, exp, cond, opts, nentries, firstentry)
      else
        if cond == nil then cond = "" end
        if opts == nil then opts = "" end
        if nentries == nil then nentries = 0 end
        if firstentry == nil then firstentry = 0 end
        return _Draw(self, exp, cond, opts, nentries, firstentry)
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
      return self.branches_list
    end

    function self:Reset()
      for k, v in pairs(self.branches_list) do
        v:Set()
      end
    end
  end)
