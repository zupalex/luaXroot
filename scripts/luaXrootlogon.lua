-- Do not touch this except if you exactly know what you are doing --

-- Loading the wrapper between ROOT objects and lua
local lgtbpckg = assert(package.loadlib(LUAXROOTLIBPATH .. "/libLuaXRootlib.so", "luaopen_libLuaXRootlib"))
lgtbpckg()

function sleep(s)
  local t0 = os.clock()
  while os.clock() - t0 <= s do end
end

-- Initialize the ROOT interaction application (event processing in TCanvas, etc...)
theApp = TApplication()

RootTasks = {}
NextTaskToStart = nil

function SetupNewTask(taskname, fn, ...)
  local task = coroutine.create( function(...)
      local rets = table.pack(fn(...))

      RootTasks[taskname].status = "done"

      return table.unpack(rets)
    end)

  local taskfn = function(...)
    coroutine.resume(task, ...)
  end

  RootTasks[taskname] = {
    task = SetupNewTask_C(taskfn, ...),
    routine = task,
    status = "waiting",
  }

  NextTaskToStart = taskname
end

function StartNewTask(taskname, fn, ...)
  SetupNewTask(taskname, fn, ...)
  StartNewTask_C()
  RootTasks[taskname].status = "running"
end

-- Make a function to exit the program nicely without getting a bucket load of seg faults
function exit() 
  saveprompthistory()
  theApp:Terminate() 
end

function q()
  exit()
end

-- Here are the modules which wil lbe loaded upon starting a session of luaXroot --

require("lua_helper")
require("lua_tree")

pcall(require, 'userlogon') -- this line attempt to load additional user/userlogon.lua. If it doesnt't exist it does nothing. 
-- If the user wants to load additional modules, it should be done in this file. Create it if needed. The directory user might need to be created as well.

-- If you want to add modules which are not where you found built-in modules, you ----------------
-- will need to set the search path to include the location of such scripts ----------------------
-- To do this add "package.path = package.path .. ";<path/to/add>/?.lua"  in user/userlogon.lua --
-- e.g.: package.path = package.path .. ";/home/awesomescripts/?.lua;/home/awesomescripts/lua_scripts/?"