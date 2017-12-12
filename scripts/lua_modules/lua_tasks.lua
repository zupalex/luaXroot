local defaultPackages = {}

RootTasks = {}

if IsMasterState then 
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
end

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

-- Support function to setup new threads
function LoadAdditionnalPackages(packslist)
  for k, v in pairs(packslist) do
    require(k)
  end
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

function SendSignalUnique(taskname, sig, ...)
  local args = table.pack(...)
  local args_str = serpent.dump(args)

  if type(sig) == "string" then
    if #args > 0 then
      sig = sig.."$ARGS"..args_str
    end

    SendSignal_C(taskname, sig, true)
  elseif type(sig) == "function" then
    local sig_str = serpent.dump(sig)

    if #args > 0 then
      sig_str = sig_str.."$ARGS"..args_str
    end

    SendSignal_C(taskname, sig_str, true)
  end
end

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

return defaultPackages