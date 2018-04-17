libraries = {}
libraries.loaded = {}

function file_exists(name)
  local f=io.open(name,"r")
  if f~=nil then io.close(f) return true else return false end
end

function AddLibrariesPath(path)
  package.path = package.path .. ";" .. path .. "/?.lua;" .. path .. "/?"
end

function LoadLib(lib, libname, use_pristine)
  if libname == nil then
    print("Library name must be specified")
    return
  end

  if lib:find("./") ~= 0 or lib:find("/") ~= 0 then
    if file_exists("./" .. lib) then
      lib = "./" .. lib
    end
  end

  if not file_exists(lib) then
    local tried_paths = { lib, "./"..lib }

    local found_lib = false

    test_path_fn = string.gmatch(package.path, "[^;]+")

    local test_path = test_path_fn()

    test_path = test_path:sub(1, test_path:find("?")-1) .. lib .. test_path:sub(test_path:find("?")+1)

    while test_path do
      if not file_exists(test_path) then
        table.insert(tried_paths, test_path)

        test_path = test_path_fn()

        if test_path then test_path = test_path:sub(1, test_path:find("?")-1) .. lib .. test_path:sub(test_path:find("?")+1) end
      else
        found_lib = true
        lib = test_path
        break
      end
    end

    if not found_lib then
      print("ERROR: Could not find libraries is the following search paths")

      for i, v in ipairs(tried_paths) do
        print("       " .. v)
      end

      return
    end
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

if IsMasterState then 
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

    print("Compilation complete...")

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
        print("Library loaded...")
      end
    end
  end
end