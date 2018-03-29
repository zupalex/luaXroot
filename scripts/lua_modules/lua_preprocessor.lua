local function check_preprocessor(line, preproc)
  local prep_id = line:find(preproc)

  if prep_id then
    if prep_id == 1 or (prep_id>1 and line:sub(1,prep_id-1):find("[^%s]") == nil) then
      local prep_cond = "return "..line:sub(prep_id+preproc:len())
      return prep_cond
    end
  end

  return nil
end

local function skip_to_next_preproc(file, gotoend)
  local line = file:read("*line")

  local nend = 1

  while line do
    if check_preprocessor(line, "#if") then 
      nend = nend+1
    elseif check_preprocessor(line, "#elseif") and not gotoend then
      if nend == 1 then
        local prep_instruction = check_preprocessor(line, "#elseif")
        if load(prep_instruction)() then
--          print("validating an #elseif", check_preprocessor(line, "#elseif"))
          return
        end
      end
    elseif check_preprocessor(line, "#else") and not gotoend then
      if nend == 1 then
--        print("validating an #else")
        return
      end
    elseif check_preprocessor(line, "#endif") then
      nend = nend -1

      if nend == 0 then
--        print("reached #endif while skipping")
        return 
      end
    end

    line = file:read("*line")
  end
end

local function read_preproc_content(file, preprocessed)
  local line = file:read("*line")

  while line do
    local prep_instruction = check_preprocessor(line, "#if")
    if prep_instruction then
      if not load(prep_instruction)() then
--        print("preprocessor evaluated false", prep_instruction)
        skip_to_next_preproc(file)
      end

      read_preproc_content(file, preprocessed)
    elseif check_preprocessor(line, "#elseif") then
--      print("preprocessor #elseif will be skipped", check_preprocessor(line, "#elseif"))
      skip_to_next_preproc(file, true)
    elseif check_preprocessor(line, "#else") then
--      print("preprocessor #else will be skipped")
      skip_to_next_preproc(file, true)
    elseif check_preprocessor(line, "#endif") then
--      print("preprocessor #endif reached")
      return
    else
--      print("keeping line", line)
      table.insert(preprocessed, line)
    end

    line = file:read("*line")
  end
end

local function preprocess(file)
  local inputf = io.open(file, "r")

  if not inputf then
    print("Error: could not load", file)
    return nil
  end

  local preprocessed = {}

  local line = inputf:read("*line")

  while line do
    local prep_instruction = check_preprocessor(line, "#if")

    if prep_instruction == nil then
--      print("no preprocessor instruction")
--      print(line)
      table.insert(preprocessed, line)
    else
      if not load(prep_instruction)() then
--        print("preprocessor evaluated false", prep_instruction)
        skip_to_next_preproc(inputf)
      end

      read_preproc_content(inputf, preprocessed)
    end

    line = inputf:read("*line")
  end

--  print(#preprocessed)
--  for i, v in ipairs(preprocessed) do
--    print(i, v)
--  end

  local finalstr = table.concat(preprocessed, "\n")

  return finalstr
end

return preprocess