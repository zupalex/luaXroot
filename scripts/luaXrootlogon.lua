-- Do not touch this except if you exactly know what you are doing --

_setterfns = {}
_getterfns = {}
_pushbackfns = {}

require("lua_libraries_utils")

-- Modules which wil be loaded upon starting a session of luaXroot --
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

  function exit() 
    print("")
    if theApp.isforked == nil then
      saveprompthistory(_getluaxrootparam("max_history_length"))

      for i, v in ipairs(msgq._activemsgqs) do if v.owner then MsgCtl({v.id, IPC_RMID}) end end
      for i, v in ipairs(sem._activesems) do if v.owner then SemCtl({v.id, IPC_RMID}) end end
      for i, v in ipairs(shmem._activeshmems) do if v.owner then ShmCtl({v.id, IPC_RMID}) end end
    end

    if _getluaxrootparam("pygui_id") ~= -1 then __master_gui_socket:Send("terminate process") end

    sleep(0.5)

    theApp:Terminate() 
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

local userlog_loaded, errmsg = pcall(require, 'userlogon') -- this line attempt to load additional user/userlogon.lua. If it doesnt't exist it does nothing. 
-- If the user wants to load additional modules, it should be done in this file. Create it if needed. The directory user might need to be created as well.

-- If you want to add modules which are not where you found built-in modules, you ----------------
-- will need to set the search path to include the location of such scripts ----------------------
-- To do this add "package.path = package.path .. ";<path/to/add>/?.lua"  in user/userlogon.lua --
-- e.g.: package.path = package.path .. ";/home/awesomescripts/?.lua;/home/awesomescripts/lua_scripts/?"

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

  if _getluaxrootparam("usepygui") == 1 then
    StartPYGUI()
  else
    _setluaxrootparam("pygui_id", -1)
  end
end