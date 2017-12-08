
_LuaRootObj = {}

function _LuaRootObj.Set(self, member, value)
  return self.members[member]:Set(value)
end

function _LuaRootObj.Get(self, member, value)
  return self.members[member]
end

function _LuaRootObj.Value(self, member, value)
  return self.members[member]:Get()
end

---------------------------------------------------------------------
------------------------------- TCanvas -------------------------------
---------------------------------------------------------------------

AddPostInit("TCanvas", function(self)
    local _Divide = self.Divide
    function self:Divide(nrow, ncol)
      self.nrow = nrow
      self.ncol = ncol

      _Divide(self, nrow, ncol)
    end

    local _Draw = self.Draw
    function self:Draw(rootObj, opts, rown, coln)
      if rootObj == nil then return end
      _Draw(self, rootObj, rown or 1, coln or 0)
      rootObj:Draw(opts)
    end

    local _SetLogScale = self.SetLogScale
    function self:SetLogScale(rown, coln, axis, val)
      if type(rown) == "string" then
        _SetLogScale(self, 0, 0, rown, coln or true)
      else
        _SetLogScale(self, rown, coln, axis, val or true)
      end
    end
  end)

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

    local tfunc

    if formula ~= nil then
      if xmin ~= nil and xmax ~= nil then
        tfunc = _TF1(name, formula)
      else
        tfunc = _TF1(name, formula, xmin, xmax)
        tfunc.xmin = xmin
        tfunc.xmax = xmax
      end
    elseif fn ~= nil and npars ~= nil and xmin ~= nil and xmax ~= nil and type(fn) == "function" then
      local fn_id = _stdfunctions.unnamed+1
      local reg_name = "tf1fn_"..tostring(fn_id)
      RegisterTF1fn(reg_name, fn, npars)
      _stdfunctions.unnamed = fn_id

      if ndim ~= nil then
        local tfunc = _TF1(name, reg_name, xmin, xmax, npars, ndim)
        tfunc.xmin = xmin
        tfunc.xmax = xmax
        tfunc.npars = npars
        tfunc.ndim = dnim
      else
        local tfunc = _TF1(name, reg_name, xmin, xmax, npars)
        tfunc.xmin = xmin
        tfunc.xmax = xmax
        tfunc.npars = npars
      end
    else
      return PrintTF1Error()
    end

    return tfunc
--    return tfunc:IsValid() and tfunc or nil
  elseif args == nil then
    return _TF1()
  else
    local tfunc
    local other_args = table.pack(...)
    if type(other_args[1]) == "function" and other_args[4] ~= nil then
      local fn_id = _stdfunctions.unnamed+1
      local reg_name = "tf1fn_"..tostring(fn_id)
      RegisterTF1fn(reg_name, other_args[1], other_args[4])
      _stdfunctions.unnamed = fn_id

      if other_args[5] == nil then
        tfunc = _TF1(args, reg_name, other_args[2], other_args[3], other_args[4])
        tfunc.xmin = other_args[2]
        tfunc.xmax = other_args[3]
        tfunc.npars = other_args[4]
      else
        local tfunc = _TF1(args, reg_name, other_args[2], other_args[3], other_args[4], other_args[5])
        tfunc.xmin = other_args[2]
        tfunc.xmax = other_args[3]
        tfunc.npars = other_args[4]
        tfunc.ndim = other_args[5] 
      end
    else
      tfunc = _TF1(args, ...)
      tfunc.xmin = other_args[2]
      tfunc.xmax = other_args[3]
      tfunc.npars = other_args[4]
      tfunc.ndim = other_args[5] 
    end

    return tfunc
--    return tfunc:IsValid() and tfunc or nil
  end
end

AddPostInit("TF1", function(self)
    local _GetRandom = self.GetRandom
    function self:GetRandom(xmin, xmax)
      if xmin == nil then xmin = self.xmin end
      if xmax == nil then xmax = self.xmax end

      return _GetRandom(self, xmin, xmax)
    end

    local _Integral = self.Integral
    function self:Integral(xmin, xmax)
      if xmin == nil then xmin = self.xmin end
      if xmax == nil then xmax = self.xmax end

      return _Integral(self, xmin, xmax)
    end
  end)

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
    self.bins = {}

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

    local _SetXProperties = self.SetXProperties
    function self:SetXProperties(nbinsx, xmin, xmax)
      _SetXProperties(self, nbinsx, xmin, xmax)
      self.binwidth = (xmax-xmin)/nbinsx
      self.nbinsx = nbinsx
      self.xmin = xmin
      self.xmax = xmax
      self:Draw()
    end

    local _Integral = self.Integral
    function self:Integral(xmin, xmax)
      if xmin == nil or xmax == nil then xmin, xmax = 0, 0 end
      return _Integral(self, xmin, xmax)
    end

    local xprops = self:GetXProperties()
    self.binwidth = (xprops[3]-xprops[2])/xprops[1]
    self.nbinsx = xprops[1]
    self.xmin = xprops[2]
    self.xmax = xprops[3]

    function self:GetBinWidth()
      return self.binwidth
    end

    function self:GetBinNumber(x)
      return math.floor((x-self.xmin)/self.binwidth)+1
    end

    for i=1, xprops[1]+2 do
      self.bins[i] = 0
    end

    function self:Buffer(x, weight)
      local bin = self:GetBinNumber(x)+1
      if bin <= self.nbinsx+2 and bin >= 1 then
        self.bins[bin] = self.bins[bin] + (weight or 1)
      end
    end

    function self:RefreshROOTObj()
      self:SetContent(self.bins)
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
    self.bins = {}

    local _Fill = self.Fill
    function self:Fill(valx, valy, weight)
      if weight == nil then
        return _Fill(self, valx, valy, 1)
      else
        return _Fill(self, valx, valy, weight)
      end
    end

    local _SetXProperties = self.SetXProperties
    function self:SetXProperties(nbinsx, xmin, xmax, drawopts)
      _SetXProperties(self, nbinsx, xmin, xmax)
      self.binwidthx = (xmax-xmin)/nbinsx
      self.nbinsx = nbinsx
      self.xmin = xmin
      self.xmax = xmax
      self.nbins = (self.nbinsy+2)*(self.nbinsx+2)
      self:Draw(drawopts)
    end

    local _SetYProperties = self.SetYProperties
    function self:SetYProperties(nbinsy, ymin, ymax, drawopts)
      _SetYProperties(self, nbinsy, ymin, ymax)
      self.binwidthy = (ymax-ymin)/nbinsy
      self.nbinsy = nbinsy
      self.ymin = ymin
      self.ymax = ymax
      self.nbins = (self.nbinsy+2)*(self.nbinsx+2)
      self:Draw(drawopts)
    end

    local _ProjectX = self.ProjectX
    function self:ProjectX(ymin, ymax)
      _ProjectX(self, ymin, ymax)
      return GetObject("TH1D", self:GetName().."_projX")
    end

    local _ProjectY = self.ProjectY
    function self:ProjectY(xmin, xmax)
      _ProjectY(self, xmin, xmax)
      return GetObject("TH1D", self:GetName().."_projY")
    end

    function self:GetProjectionX()
      return GetObject("TH1D", self:GetName().."_projX")
    end

    function self:GetProjectionY()
      return GetObject("TH1D", self:GetName().."_projY")
    end

    local _Integral = self.Integral
    function self:Integral(xmin, xmax, ymin, ymax)
      if xmin == nil or xmax == nil or ymin == nil or ymax == nil then xmin, xmax, ymin, ymax = 0, 0, 0, 0 end
      return _Integral(self, xmin, xmax, ymin, ymax)
    end

    local xprops = self:GetXProperties()
    local yprops = self:GetYProperties()

    self.nbinsx = xprops[1]
    self.xmin = xprops[2]
    self.xmax = xprops[3]

    self.nbinsy = yprops[1]
    self.ymin = yprops[2]
    self.ymax = yprops[3]

    self.nbins = (yprops[1]+2)*(xprops[1]+2)

    self.binwidthx = (xprops[3]-xprops[2])/xprops[1]
    self.binwidthy = (yprops[3]-yprops[2])/yprops[1]

    function self:GetBinWidthX()
      return self.binwidthx
    end

    function self:GetBinWidthY()
      return self.binwidthy
    end

    function self:GetBinNumber(x, y)
      local binx = math.floor((x-self.xmin)/self.binwidthx)+1
      local biny = math.floor((y-self.ymin)/self.binwidthy)+1

      return binx+(biny)*(self.nbinsx+2)
    end

    for i=1, self.nbins do
      self.bins[i] = 0
    end

    function self:Buffer(x, y, weight)
      local bin = self:GetBinNumber(x, y)+1
      if bin <= self.nbins and bin >= 1 then
        self.bins[bin] = self.bins[bin] + (weight or 1)
      end
    end

    function self:RefreshROOTObj()
      self:SetContent(self.bins)
    end
  end)

---------------------------------------------------------------------
----------------------------- TSpectrum -----------------------------
---------------------------------------------------------------------

local function PrintTSpectrumError()
  print("ERROR in TSpectrum constructor")
  print("  [maxposition] -> maximum number of peaks")
end

local _TSpectrum = TSpectrum
function TSpectrum(args, ...)
  if type(args) == "table" then
    local maxpositions = args.maxpositions
    return _TSpectrum(maxpositions)
  elseif args == nil then
    return _TSpectrum()
  else
    return _TSpectrum(args, ...)
  end
end

AddPostInit("TSpectrum", function(self)
    local _Background = self.Background
    function self:Background(histname, niter, opts)
      _Background(self, histname, niter, opts)
      return GetObject("TH1D", histname.."_background")
    end

    function self:GetHistogram()
      local backname = self:GetBackgroundName()
      return GetObject("TH1D", backname)
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
        v:Reset()
      end
    end
  end)
