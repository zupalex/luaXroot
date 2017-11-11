-- Do not touch this except if you exactly know what you are doing --

local defaultPackages = {}

luaXrootParams = {}
luaXrootParams.max_history_length = 500

libraries = {}
libraries.loaded = {}

function LoadLib(lib, libname, use_pristine)
  if libname == nil then
    print("Library name must be specified")
    return
  end

  local open_lib

  if use_pristine then
    open_lib = assert(package.loadlib(lib, libname))
  elseif libname ~= "*" then
    open_lib = assert(package.loadlib(lib, "openlib_"..libname))
  else
    open_lib = assert(package.loadlib(lib, "*"))
  end

  open_lib()

  libraries.loaded[libname] = true
end

-- Modules which wil be loaded upon starting a session of luaXroot --
require("lua_classes")
require("lua_helper")
shm = require("lua_shmem")
socket = require("lua_sockets")
require("lua_tree")

-- Loading the wrapper between ROOT objects and lua
LoadLib(LUAXROOTLIBPATH .. "/libLuaXRootlib.so", "luaopen_libLuaXRootlib", true)

LoadLib(LUAXROOTLIBPATH .. "/libRootBinderLib.so", "lua_root_classes")

require("lua_root_binder")

_require = _G.require
function require(pckg)
  local luaExt = pckg:find(".lua")
  if luaExt then
    local pckg_adjust = pckg:sub(1, luaExt-1)

    local required = _require(pckg_adjust)

    if required then
      return required
    end
  end

  return _require(pckg)
end

if IsMasterState then
-- Initialize the ROOT interaction application (event processing in TCanvas, etc...)
  theApp = TApplication()

  RootTasks = {}

  local function GetAdditionnalPackages()
    local packaddons = {}

    for k, v in pairs(package.loaded) do
      if not defaultPackages[k] and k ~= "luaXrootlogon" then
        local packpath = package.searchpath(k, package.path)

        if packpath ~= nil then
          packaddons[k] = packpath
        end
      end
    end

    return packaddons
  end

--******** Task Managing Functions ********--

  function ForkWithRedirect(fn, ...)
    if type(fn) ~= "function" then
      print("Invalid arguments. Format: function, [arg1], [arg2], ...")
      return
    end

    local fds = {}
    local prefn = function() 
      fds.fdin, fds.fdout = MakePipe() 
    end

    local new_fn = function(fds, ...)
      SysDup2(fds.fdin, 1)
      fn(...)
    end

    SysFork({fn=new_fn, args=table.pack(fds, ...), preinit=prefn})

    return fds.fdout
  end

  function StartNewTask(taskname, fn, ...)
    RootTasks[taskname] = {
      name = taskname,
      taskfn = fn,
      args = table.pack(...),
    }

    local serialized_task = serpent.dump(RootTasks[taskname])
    local serialized_packslist = serpent.dump(GetAdditionnalPackages())

    StartNewTask_C(serialized_task, serialized_packslist)
  end

  function GetStatus(taskname)
    local status = GetTaskStatus(taskname)
    return status
  end

  function SendSignal(taskname, sig, ...)
    local args = table.pack(...)
    local args_str = serpent.dump(args)

    if type(sig) == "string" then
      if #args > 0 then
        sig = sig.."$ARGS"..args_str
      end

      SendSignal_C(taskname, sig)
    elseif type(sig) == "function" then
      local sig_str = serpent.dump(sig)

      if #args > 0 then
        sig_str = sig_str.."$ARGS"..args_str
      end

      SendSignal_C(taskname, sig_str)
    end
  end

  function TasksList(doprint)
    local tasks = TasksList_C()

    if doprint then
      for k, v in pairs(tasks) do
        print("*", k, ": status =", v)
      end
    end

    return tasks
  end

  function PauseTask(taskname)
    SendSignal(taskname, "wait")
  end

  function ResumeTask(taskname)
    SendSignal(taskname, "resume")
  end

  function StopTask(taskname)
    SendSignal(taskname, "stop")
  end

--******** Late Compilation Utilities ********--

  function CompileC(args, ...)    
    if type(args) ~= "table" then
      local other_args = table.pack(...)
      args = {script=args, libname = other_args[1], target= other_args[2] and other_args[2] or nil}
    end

    if args.openfn ~= nil then
      args.libname = args.openfn
    end

    if libraries.loaded[args.libname] then
      print("Library "..args.libname.." has already been loaded...")
      return
    end

    CompilePostInit_C({script=args.script, target=args.target})

    if args.libname ~= nil and args.libname ~= "*" then
      if args.openfn == nil then
        args.libname = "openlib_"..args.libname
      end

      local scriptExtPos = args.script:find("%.C") or args.script:find("%.c")
      local scriptExt = args.script:sub(scriptExtPos+1)

      if scriptExt then
        local scriptBase = args.script:sub(1, scriptExtPos-1)

        if not scriptBase:find("/") then
          scriptBase = "./"..scriptBase
        end

        LoadLib(scriptBase.."_"..scriptExt..".so", args.libname, true)
      end
    end
  end

--******** Make functions to exit the program nicely without getting a bucket load of seg faults ********--

  function exit() 
    print("")
    saveprompthistory(luaXrootParams.max_history_length)
    theApp:Terminate() 
  end

  function q()
    exit()
  end
else
  theApp = GetTheApp()
end

-- Create a sleep function
function sleep(s)
  local t0 = os.clock()
  while os.clock() - t0 <= s do end
end

-- Support function to setup new threads
function LoadAdditionnalPackages(packslist)
  for k, v in pairs(packslist) do
    require(k)
  end
end

-- Initialize the table serializer written by Paul Kulchenko (paul@kulchenko.com)
-- github link: https://github.com/pkulchenko/serpent
serpent = require("serpent")

function CheckSignals()
  return CheckSignals_C()
end

_tasksignals = {}

function AddSignal(signame, sigfn)
  _tasksignals[signame] = sigfn
end

function ProcessSignal(sig_str)
  local args_pos = sig_str:find("$ARGS")
  local args = {}
  if args_pos ~= nil then
    local args_str = sig_str:sub(args_pos+5)
    sig_str = sig_str:sub(1, args_pos-1)
    args = load(args_str)()
  end

  if _tasksignals[sig_str] then
    _tasksignals[sig_str](table.unpack(args))
  else
    local sigfn = load(sig_str)()
    sigfn(table.unpack(args))
  end
end

pcall(require, 'userlogon') -- this line attempt to load additional user/userlogon.lua. If it doesnt't exist it does nothing. 
-- If the user wants to load additional modules, it should be done in this file. Create it if needed. The directory user might need to be created as well.

-- If you want to add modules which are not where you found built-in modules, you ----------------
-- will need to set the search path to include the location of such scripts ----------------------
-- To do this add "package.path = package.path .. ";<path/to/add>/?.lua"  in user/userlogon.lua --
-- e.g.: package.path = package.path .. ";/home/awesomescripts/?.lua;/home/awesomescripts/lua_scripts/?"

defaultPackages = shallowcopy(package.loaded)