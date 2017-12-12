
if IsMasterState then
  _pyguifns = {}

  function _pyguifns.GetList(listname)
    print("received GetList request from", requester, "for", listname, "to redirect to", method)

    local gmIter = listname:gmatch("%a+%w*")

    local rootfield = _G[gmIter()]
    local nextfield = gmIter()

    while nextfield do
      rootfield = rootfield[nextfield]
      nextfield = gmIter()
    end

    local keys = {}

    for k, v in pairs(nextfield) do
      table.insert(k)
    end

    local list_of_keys = table.concat(keys, "\\li")

    __master_gui_socket:Send(list_of_keys)
  end

  function _pyguifns.GetHMonitors(filter)
    SetSharedBuffer("")
    SendSignal("ornlmonitor", "ls", true, {filter}, true)

    local list_of_keys = GetSharedBuffer()

    while list_of_keys:len() == 0 do
      sleep(0.1)
      list_of_keys = GetSharedBuffer()
    end

    __master_gui_socket:Send(list_of_keys)
  end
end