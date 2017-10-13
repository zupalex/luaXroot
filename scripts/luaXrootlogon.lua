-- Do not touch this except if you exactly know what you are doing --

-- Loading the wrapper between ROOT objects and lua
local rootbindpckg = assert(package.loadlib(LUAXROOTLIBPATH .. "/libLuaXRootlib.so", "luaopen_libLuaXRootlib"))
rootbindpckg()

if IsMasterState then
-- Initialize the ROOT interaction application (event processing in TCanvas, etc...)
  theApp = TApplication()

  RootTasks = {}

  local function GetAdditionnalPackages()
    local packaddons = {}

    for k, v in pairs(package.loaded) do
      local packpath = package.searchpath(k, package.path)

      if packpath ~= nil then
        packaddons[k] = packpath
      end
    end

    return packaddons
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

  function SendSignal(taskname, sig)
    if type(sig) == "string" then
      SendSignal_C(taskname, sig)
    elseif type(sig) == "function" then
      local sig_str = serpent.dump(sig)
      SendSignal_C(taskname, sig_str)
    end
  end

-- Make a function to exit the program nicely without getting a bucket load of seg faults
  function exit() 
    saveprompthistory()
    theApp:Terminate() 
  end

  function q()
    exit()
  end
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

function ProcessSignal(sig_str)
  local sigfn = load(sig_str)()
  sigfn()
end

function CheckSuspend()
  ReleaseTaskMutex()
  LockTaskMutex()
end

TaskComplete = ReleaseTaskMutex

-- Here are the modules which wil lbe loaded upon starting a session of luaXroot --

require("lua_helper")
require("lua_tree")

pcall(require, 'userlogon') -- this line attempt to load additional user/userlogon.lua. If it doesnt't exist it does nothing. 
-- If the user wants to load additional modules, it should be done in this file. Create it if needed. The directory user might need to be created as well.

-- If you want to add modules which are not where you found built-in modules, you ----------------
-- will need to set the search path to include the location of such scripts ----------------------
-- To do this add "package.path = package.path .. ";<path/to/add>/?.lua"  in user/userlogon.lua --
-- e.g.: package.path = package.path .. ";/home/awesomescripts/?.lua;/home/awesomescripts/lua_scripts/?"