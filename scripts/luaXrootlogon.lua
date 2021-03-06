-- Do not touch this except if you exactly know what you are doing --

LUAXROOTLIBPATH = LUAXROOTLIBPATH .. "/"

_setterfns = {}
_getterfns = {}
_pushbackfns = {}

-- Modules which will be loaded upon starting a session of luaXroot --
require("lua_libraries_utils")
require("lua_classes")

-- Loading the wrapper between ROOT objects and lua
LoadLib(LUAXROOTLIBPATH .. "/libLuaXRootlib.so", "luaopen_libLuaXRootlib", true)
LoadLib(LUAXROOTLIBPATH .. "/libRootBinderLib.so", "lua_root_classes")

function SetUsePYGui(usepygui)
  if IsMasterState then
    if usepygui == true or usepygui == nil then
      _setluaxrootparam("usepygui", 1)
    else
      _setluaxrootparam("usepygui", 0)
    end
  end
end

_setluaxrootparam("usepygui", 0)

_setluaxrootparam("max_history_length", 500)

require("lua_helper")
msgq, sem, shmem, mmap = table.unpack(require("lua_shmem"))
socket = require("lua_sockets")
require("lua_tree")

require("lua_root_binder")

Preprocess = require("lua_preprocessor")

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

function requirep(pckg)
  local pckgname = pckg:sub(1, pckg:find(".lua"))

  if package.loaded.pckgname then return end

  local pckg_path = package.searchpath(pckgname, package.path)

  if not pckg_path then
    print(pckgname, "not found in the search paths")
    return
  end

  package.loaded[pckgname] = true

  local pckg_fn = Preprocess(pckg_path)

  if not pckg_fn then 
    package.loaded[pckgname] = nil 
    return
  end

  return load(pckg_fn)()
end

if IsMasterState then 
-- Initialize the ROOT interaction application (event processing in TCanvas, etc...)
  theApp = TApplication()

  function exit() 
    print("")
    if theApp.isforked == nil then
      saveprompthistory(_getluaxrootparam("max_history_length"))

      for i, v in ipairs(msgq._activemsgqs) do if v.owner then MsgCtl({v.id, IPC_RMID}) end end
      for i, v in ipairs(sem._activesems) do if v.owner then SemCtl({v.id, IPC_RMID}) end end
      for i, v in ipairs(shmem._activeshmems) do if v.owner then ShmCtl({v.id, IPC_RMID}) end end
    end

    if _getluaxrootparam("pygui_id") ~= -1 then
      __master_gui_socket:Send("terminate process")
    end

    sleep(0.5)

    os.exit()
  end

  function q()
    exit()
  end
else
  theApp = GetTheApp()
end

-- Initialize the table serializer written by Paul Kulchenko (paul@kulchenko.com)
-- github link: https://github.com/pkulchenko/serpent
serpent = require("serpent")

local defaultPackages = require("lua_tasks")

if not file_exists(LUAXROOTLIBPATH .. "/../user/userlogon.lua") then
  local fuserlogon = io.open(LUAXROOTLIBPATH .. "/../user/userlogon.lua", "w")

  fuserlogon:write("-- If you want to add modules which are not where you found built-in modules, you ----------------\n")
  fuserlogon:write("-- will need to set the search path to include the location of such scripts ----------------------\n")
  fuserlogon:write("-- To do this add AddLibrariesPath(\"<path/to/add>\")  in user/userlogon.lua --\n")
  fuserlogon:write("-- The full path to luaXroot/exec folder can be accessed using LUAXROOTLIBPATH --\n")  
  fuserlogon:write("-- e.g.: AddLibrariesPath(\"/home/awesomescripts\") --\n")
  fuserlogon:write("-- e.g.: AddLibrariesPath(LUAXROOTLIBPATH .. \"../user/awesomescripts\") --\n\n")
  io.close(fuserlogon)
end

local userlog_loaded, errmsg = pcall(require, 'userlogon') -- this line attempt to load additional user/userlogon.lua.
-- If the user wants to load additional modules, it should be done in this file.

if not userlog_loaded then
  if errmsg:find("module 'userlogon' not found") == nil then
    print("WARNING: Error in userlogon.lua script")
    print(errmsg)
    print("----------------------------------------------")
    print(debug.traceback())
  end
end

defaultPackages = shallowcopy(package.loaded)

if IsMasterState then
  function StartPYGUI()
    if _getluaxrootparam("pygui_id") == -1 then
      require("lua_pygui_ipc")
      __master_gui_socket = socket.CreateHost("net", "127.0.0.1:0", nil, nil, true)
      if __master_gui_socket == nil then
        print("Failed to connect master GUI socket")
        exit()
      end

      StartNewTask("__guilistener", function(master_port)
          local gui_socket = socket.CreateHost("net", "127.0.0.1:0", nil, nil, true, "Q")

          StartNewTask("__pygui", function(master_port, interface_port)
              os.execute("python "..LUAXROOTLIBPATH.."/../scripts/python_scripts/startgui.py --socket "..interface_port.." "..master_port)
            end, master_port, gui_socket.port)

          if gui_socket then
            local guifd = gui_socket:AcceptConnection()

            while CheckSignals() do
              local cmd = gui_socket:WaitAndReadResponse(guifd)

              if cmd and cmd:len() > 0 then
                local cmd_formatted = cmd
--            local cmd_formatted = serpent.dump(cmd)
                SendMasterCmd(cmd_formatted)
              end
            end
          end
        end, __master_gui_socket.port)

      __master_gui_socket:AcceptConnection()
      __master_gui_socket:AcceptConnection()

      readfds = SysSelect({read=__master_gui_socket.clientsfd})

      if #readfds ~= 1 then
        print("ERROR setting up the python GUI")
      end

      __pygui_pid = __master_gui_socket:ReadResponse(readfds[1])
      _setluaxrootparam("pygui_id", __pygui_pid)
    end
  end

  _setluaxrootparam("pygui_id", -1)

  if _getluaxrootparam("usepygui") == 1 then
    StartPYGUI()
  end
end