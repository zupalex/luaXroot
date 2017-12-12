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
end