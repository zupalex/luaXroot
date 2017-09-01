-- Do not touch this except if you exactly know what you are doing --

-- Loading the wrapper between ROOT objects and lua
local lgtbpckg = assert(package.loadlib(LUAXROOTLIBPATH .. "/libLuaXRootlib.so", "luaopen_libLuaXRootlib"))
lgtbpckg()

-- Initialize the ROOT interaction application (event processing in TCanvas, etc...)
theApp = TApplication()

-- Make a function to exit the program nicely without getting a wheelbarrow load of seg faults
function exit() 
  theApp:Terminate() 
end

-- Add here the modules you want to be loaded upon launching a session of lua_rint --
-- The modules by default should probably not be removed from that list -------------

-- *****************************
-- ** Recommended modules ** 
-- *****************************

require("lua_helper") -- this one is named helper but it should have been named "core". Do not remove
require("lua_tree")

-- If you want to add modules which are not where you found built-in modules, you ---
-- will need to set the search path to include the location of such scripts ---------
-- To do this:  "package.path = package.path .. ";<path/to/add>/?.lua" --------------

package.path = package.path .. ";/mnt/hgfs/Dropbox/ORNL/goddess_daq/goddess_daq/user/lua_scripts/?.lua;/mnt/hgfs/Dropbox/ORNL/goddess_daq/goddess_daq/user/lua_scripts/?"

-- **************************
-- ** Optional modules **
-- **************************

-- require("MIDAS_reader")
require("LDF_reader")