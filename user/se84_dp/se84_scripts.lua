local OverrideIdentifyPacket = require("nscl_unpacker/packet_identifier.lua")

require("nscl_unpacker/nscl_unpacker")
require("se84_dp/se84_processdata")

NSCL_UNPACKER.NSCL_DAQ_VERSION = 11.0

OverrideIdentifyPacket("E16025PacketIdentifier")

function StartBufferingRingSelector(mastertask, buf_file_size, source, accept)
  local pipe=AttachOutput(os.getenv("DAQBIN").."/ringselector", "ringselector", 
    {accept, "--non-blocking", source}, {"DAQBIN"})

  local buf_file = assert(io.open("online_buffer/test_buf-0.evt", "wb"))
  local buf_file_num = 0

  while CheckSignals() do
    local data = pipe:WaitAndRead(nil, true)
    buf_file:write(data)

    if buf_file:seek("end") > buf_file_size then
      buf_file:close()
      buf_file_num = buf_file_num+1
      buf_file = assert(io.open("online_buffer/test_buf-"..tostring(buf_file_num)..".evt", "wb"))
      SendSignal(mastertask, "buffilenum", buf_file_num)
    end
  end
end

function StartBufferingAndRead(dump, source)
  local max_buf_file_size, buf_file_num, writer_file_num = 5e7, 0, 0
  os.remove("online_buffer/test_buf-"..tostring(buf_file_num)..".evt")

  local buf_file = io.open("online_buffer/test_buf-"..tostring(buf_file_num)..".evt", "rb")

  if source == nil or source == "masterevb" or source == "meb" or source == "MEB" then
    source = "--source=tcp://spdaq08/masterevb"
    NSCL_UNPACKER.PHYSICS_FRAGMENT = true
  elseif source == "s800filter" then
    source = "--source=tcp://spdaq50/s800filter"
  elseif source == "orruba" then
    source = "--source=tcp://spdaq08/orruba"
  end

  if accept == nil then
    accept = "--accept=PHYSICS_EVENT,PHYSICS_EVENT_COUNT"
  else
    accept = "--accept="..accept
  end

  StartNewTask("buffering", "StartBufferingRingSelector", GetTaskName(), writer_file_num, source, accept)

  if dump then debug_log = 3 end

  nscl_buffer = {}
  scaler_buffer = {}
  physics_count_buffer = {}

  while buf_file == nil do
    sleep(0.1)
    print("waiting for first buffer spill...")
    buf_file = io.open("online_buffer/test_buf-"..tostring(buf_file_num)..".evt", "rb")
  end

  local maxBufSize = 32000
  local binData, fpos = "", 0

  buf_file:seek("set")

  local nevt = 0

  local startingCounts, s800_ev_counts, last_s800_evtnbr, orruba_ev_count, last_orruba_evtnbr, last_print = {}, nil, 0, nil, 0, 0

  local frag_leftover, missing_bytes

  AddSignal("buffilenum", function(num)
      writer_file_num = num
    end)

  local stat_term = MakeSlaveTerm({bgcolor="Grey42", fgcolor="Grey93", fontstyle="Monospace", fontsize=10, geometry="100x15-0+0"})

  local hists

  if debug_log == 0 then
    hists = SetupNSCLHistograms()
  end

  InitOnlineDisplay(hists)

  while CheckSignals() do
    if writer_file_num > buf_file_num and fpos > max_buf_file_size then
      if fpos < buf_file:seek("end") then
        buf_file:seek("cur", fpos)

        local file_tail = buf_file:read("a")

        if frag_leftover then 
          frag_leftover = frag_leftover..file_tail
        else
          frag_leftover = file_tail
        end
      else
        os.remove("online_buffer/test_buf-"..tostring(buf_file_num)..".evt")
        buf_file_num = buf_file_num+1
        buf_file = io.open("online_buffer/test_buf-"..tostring(buf_file_num)..".evt", "rb")
      end
    else
      if frag_leftover then 
        binData = frag_leftover 
        frag_leftover = nil
      end

      local buff_data = buf_file:read(maxBufSize)

      local buff_tbl, buff_size, bytes_read = {}, 0, 0

      if buff_data ~= nil then
        buff_tbl[#buff_tbl+1] = buff_data
        buff_size = buff_size + buff_data:len()
      end

      while buff_size < 8 or (missing_bytes and buff_size < missing_bytes) do
        print("waiting for additional data... current buffer size:", buff_size, "waiting for", missing_bytes and missing_bytes or 8)
        sleep(0.1)
        buff_data = buf_file:read(maxBufSize)
        if buff_data ~= nil then
          buff_tbl[#buff_tbl+1] = buff_data
          buff_size = buff_size +  buff_data:len()
        end
      end

      if missing_bytes then missing_bytes = nil end

      binData = binData .. table.concat(buff_tbl)

      fpos = buf_file:seek("cur")

      local totread, dlength = 0, binData:len()

      while totread < dlength do
        if totread > dlength-8 then
          frag_leftover = binData:sub(totread+1)
          break
        end

        local ptype, bread = IdentifyAndUnpack(binData, totread)

        if debug_log == 0 then
--        print(#nscl_buffer)
          ProcessNSCLBuffer(nscl_buffer, nevt)
        end

        nevt = nevt+1

        if ptype == nil then
          frag_leftover = binData:sub(totread+1)
          missing_bytes = bread
--        print("end of buffer...")
          sleep(0.1)
          break
        end

        totread = totread+bread

        if ptype == "PHYSICS_EVENT_COUNT" then
          if physics_count_buffer.body_header_size == 0 then
            if startingCounts.s800 == nil then
              startingCounts.s800 = physics_count_buffer.event_count
              s800_ev_counts = 0
              last_s800_evtnbr = startingCounts.s800
            else
              last_s800_evtnbr = physics_count_buffer.event_count
            end
          elseif physics_count_buffer.body_header_sourceID then
            if startingCounts.orruba == nil then
              startingCounts.orruba = physics_count_buffer.event_count
              orruba_ev_counts = 0
              last_orruba_evtnbr = physics_count_buffer.event_count
            else
              last_orruba_evtnbr = physics_count_buffer.event_count
            end
          end

          if ptype == "PHYSICS_EVENT_COUNT" then
            if physics_count_buffer.body_header_size == 0 then
              if startingCounts.s800 == nil then
                startingCounts.s800 = physics_count_buffer.event_count
                s800_ev_counts = 0
                last_s800_evtnbr = startingCounts.s800
              else
                last_s800_evtnbr = physics_count_buffer.event_count
              end
            elseif physics_count_buffer.body_header_sourceID then
              if startingCounts.orruba == nil then
                startingCounts.orruba = physics_count_buffer.event_count
                orruba_ev_counts = 0
                last_orruba_evtnbr = physics_count_buffer.event_count
              else
                last_orruba_evtnbr = physics_count_buffer.event_count
              end
            end
          end
        end

        nscl_buffer = {}
        scaler_buffer = {}
        physics_count_buffer = {}
      end

      bread = 0
      totread = 0

      if nevt-last_print > 10000 then
        if startingCounts.s800 and startingCounts.orruba then
          stat_term:Write(string.format("Received: S800 => %10d / %10d ||| ORRUBA => %10d / %10d\r", 
              s800_ev_counts, last_s800_evtnbr-startingCounts.s800, 
              orruba_ev_counts, last_orruba_evtnbr-startingCounts.orruba))
        end

        last_print = nevt

        UpdateNSCLHistograms(hists)
      end
    end
  end
end













----------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------

local run_infos = {
  [1] = {cut_suffix="_1", coinc_win = {low=-2, high=5}},
  [70] = {cut_suffix="_2", coinc_win = {low=-2, high=5}},
  [73] = {cut_suffix="_3", coinc_win = {low=-2, high=5}},
  [76] = {cut_suffix="_4",coinc_win = {low=0, high=10}},
}

local GetRunCategory = function(runname)
  local runnbr = runname:match("-%d%d%d%d%-")
  runnbr = tonumber(runnbr:sub(2, 5))

  local low_bound, high_bound = 1, 9999

  for k, v in pairs(run_infos) do
    if k < runnbr and k > low_bound then
      low_bound = k
    end

    if k > runnbr and k < high_bound then
      high_bound = k
    end
  end

  return low_bound
end

function SetOutputType(out_type, input_type, output_name)
  if out_type:lower() == "histograms" then
    local init = require("se84_dp/se84_hist_processors")
    init(input_type)

    if input_type == "file" then
      local run_cat = GetRunCategory(output_name)
      local cutsuffix = run_infos[run_cat].cut_suffix

      local cfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/pid_cuts.root", "read")
      sidar_protons_nopt = cfile:GetObject("TCutG", "sidar_protons_nopt")
      active_cuts[sidar_protons_nopt:GetName()] = sidar_protons_nopt

      sx3_up_protons_pt = cfile:GetObject("TCutG", "sx3_up_protons_pt")
      active_cuts[sx3_up_protons_pt:GetName()] = sx3_up_protons_pt
      sx3_up_protons_nopt = cfile:GetObject("TCutG", "sx3_up_protons_nopt")
      active_cuts[sx3_up_protons_nopt:GetName()] = sx3_up_protons_nopt
      sx3_down_protons_pt = cfile:GetObject("TCutG", "sx3_down_protons_pt")
      active_cuts[sx3_down_protons_pt:GetName()] = sx3_down_protons_pt
      sx3_down_protons_nopt = cfile:GetObject("TCutG", "sx3_down_protons_nopt")
      active_cuts[sx3_down_protons_nopt:GetName()] = sx3_down_protons_nopt

      crdc_tac_cut = cfile:GetObject("TCutG", "tac_vs_crdc1x"..cutsuffix)
      active_cuts[crdc_tac_cut:GetName()] = crdc_tac_cut

      angle_tof_cut = cfile:GetObject("TCutG", "angle_tof_cut"..cutsuffix)
      active_cuts[angle_tof_cut:GetName()] = angle_tof_cut

      pid_unreacted = cfile:GetObject("TCutG", "pid_unreacted")
      active_cuts[pid_unreacted:GetName()] = pid_unreacted

      pid_se85 = cfile:GetObject("TCutG", "pid_se85")
      active_cuts[pid_se85:GetName()] = pid_se85

      crdc1x_unreacted = cfile:GetObject("TCutG", "crdc1x_unreacted")
      active_cuts[crdc1x_unreacted:GetName()] = crdc1x_unreacted

      crdc1x_leftunreacted = cfile:GetObject("TCutG", "crdc1x_leftunreacted")
      active_cuts[crdc1x_leftunreacted:GetName()] = crdc1x_leftunreacted

      gate_pid_1 = cfile:GetObject("TCutG", "gate_pid_1")
      active_cuts[gate_pid_1:GetName()] = gate_pid_1

      gate_pid_2 = cfile:GetObject("TCutG", "gate_pid_2")
      active_cuts[gate_pid_2:GetName()] = gate_pid_2

      gate_pid_3 = cfile:GetObject("TCutG", "gate_pid_3")
      active_cuts[gate_pid_3:GetName()] = gate_pid_3

      gate_pid_4 = cfile:GetObject("TCutG", "gate_pid_4")
      active_cuts[gate_pid_4:GetName()] = gate_pid_4

      gate_pid_5 = cfile:GetObject("TCutG", "gate_pid_5")
      active_cuts[gate_pid_5:GetName()] = gate_pid_5

      cfile:Close()

      SetCoincidenceWindow(run_infos[run_cat].coinc_win.low, run_infos[run_cat].coinc_win.high)
    end
  elseif out_type:lower() == "root" then
    local init = require("se84_dp/se84_tree_processors")
    tree, tbranches = init(input_type)
  end
end

function StartListeningRingSelector(dump, source, accept, calibrate)
  if source == nil or source == "masterevb" or source == "meb" or source == "MEB" then
    source = "--source=tcp://spdaq08/masterevb"
    NSCL_UNPACKER.PHYSICS_FRAGMENT = true
  elseif source == "s800filter" then
    source = "--source=tcp://spdaq50/s800filter"
    NSCL_UNPACKER.PHYSICS_FRAGMENT = false
  elseif source == "orruba" then
    source = "--source=tcp://spdaq08/orruba"
    NSCL_UNPACKER.PHYSICS_FRAGMENT = false
  end

  if accept == nil then
    accept = "--accept=PHYSICS_EVENT,PHYSICS_EVENT_COUNT"
  else
    accept = "--accept="..accept
  end

  if calibrate then
    orruba_applycal = true
  elseif calibrate == false then
    orruba_applycal = false
  end

  nscl_buffer = {}
  scaler_buffer = {}
  physics_count_buffer = {}

  if dump then debug_log = 3 end

--  local xfprate_monitorfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/monitor_xfp_rate.root", "read")
--  local crdcx_cut = xfprate_monitorfile:GetObject("TCutG", "crdc1_cut")
--  local xfprate_graph = xfprate_monitorfile:GetObject("TGraph", "graph")
--  if xfprate_graph == nil then 
--    xfprate_graph = TGraph()
--    xfprate_graph:SetName("graph")
--    xfprate_graph:SetTitle("XFP Efficiency (compared to CRDC1)")
--  end
--  xfprate_monitorfile:Close()

  local startTime = GetClockTime()

--  xfprate_graph:Draw("ALP")

  local getdate = AttachOutput("date", "date")
  local date = getdate:WaitAndRead()
  local dIter = date:gmatch("%g+")
  local day, month, daynum, time, zone, year = dIter(), dIter(), dIter(), dIter(), dIter(), dIter()
  local root_logfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/root_logs/"..day.."-"..daynum.."-"..month.."-"..year.."-"..time..".root", "recreate")

  SetOutputType("histograms", "online")

  root_logfile:Write()

  local cfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/pid_cuts.root", "read")

  sidar_protons_nopt = cfile:GetObject("TCutG", "sidar_protons_nopt")
  active_cuts[sidar_protons_nopt:GetName()] = sidar_protons_nopt
  sx3_up_protons_pt = cfile:GetObject("TCutG", "sx3_up_protons_pt")
  active_cuts[sx3_up_protons_pt:GetName()] = sx3_up_protons_pt
  sx3_up_protons_nopt = cfile:GetObject("TCutG", "sx3_up_protons_nopt")
  active_cuts[sx3_up_protons_nopt:GetName()] = sx3_up_protons_nopt
  sx3_down_protons_pt = cfile:GetObject("TCutG", "sx3_down_protons_pt")
  active_cuts[sx3_down_protons_pt:GetName()] = sx3_down_protons_pt
  sx3_down_protons_nopt = cfile:GetObject("TCutG", "sx3_down_protons_nopt")
  active_cuts[sx3_down_protons_nopt:GetName()] = sx3_down_protons_nopt

  crdc_tac_cut = cfile:GetObject("TCutG", "tac_vs_crdc1x_4")
  active_cuts[crdc_tac_cut:GetName()] = crdc_tac_cut

  angle_tof_cut = cfile:GetObject("TCutG", "angle_tof_cut_4")
  active_cuts[angle_tof_cut:GetName()] = angle_tof_cut

  crdc1x_unreacted = cfile:GetObject("TCutG", "crdc1x_unreacted")
  active_cuts[crdc1x_unreacted:GetName()] = crdc1x_unreacted
  crdc1x_leftunreacted = cfile:GetObject("TCutG", "crdc1x_leftunreacted")
  active_cuts[crdc1x_leftunreacted:GetName()] = crdc1x_leftunreacted

  gate_pid_1 = cfile:GetObject("TCutG", "gate_pid_1")
  active_cuts[gate_pid_1:GetName()] = gate_pid_1

  gate_pid_2 = cfile:GetObject("TCutG", "gate_pid_2")
  active_cuts[gate_pid_2:GetName()] = gate_pid_2

  gate_pid_3 = cfile:GetObject("TCutG", "gate_pid_3")
  active_cuts[gate_pid_3:GetName()] = gate_pid_3

  gate_pid_4 = cfile:GetObject("TCutG", "gate_pid_4")
  active_cuts[gate_pid_4:GetName()] = gate_pid_4

  gate_pid_5 = cfile:GetObject("TCutG", "gate_pid_5")
  active_cuts[gate_pid_5:GetName()] = gate_pid_5

  cfile:Close()

  local pipe=AttachOutput(os.getenv("DAQBIN").."/ringselector", "ringselector", 
    {accept, "--non-blocking", source}, {"DAQBIN"})

  local stat_term = MakeSlaveTerm({bgcolor="Purple4", fgcolor="Grey93", fontstyle="Monospace", fontsize=10, geometry="100x15-0+0",
      title="RingSelector Listener Log", label="ringselector log"})

  local data, ptype, dlength, frag_data, frag_length, frag_leftover = nil, nil, 0, {}, {}, nil

  local bread, totread = 0, 0

  local nevt = 0

  local startingCounts, s800_ev_counts, last_s800_evtnbr, orruba_ev_count, last_orruba_evtnbr, last_print = {}, nil, 0, nil, 0, 0

  while CheckSignals() do
--    data = pipe:Read()

    dlength = 0
    frag_data = {}
    frag_length = {}

    if frag_leftover ~= nil then
      frag_data[1] = frag_leftover
      frag_length[1] = frag_leftover:len()
      frag_leftover = nil
    end

    frag_data[#frag_data+1], frag_length[#frag_length+1] = pipe:WaitAndRead(nil, true)

    while frag_length[#frag_length] > 0 do
      dlength = dlength+frag_length[#frag_length]
      frag_data[#frag_data+1], frag_length[#frag_length+1] = pipe:Read()
    end

    data = table.concat(frag_data)

    if dlength > 8 then
      while totread < dlength do
        ptype, bread = IdentifyAndUnpack(data, totread)

        if debug_log == 0 then
          ProcessNSCLBuffer(nscl_buffer, nevt)
        end

        nevt = nevt+1

        if ptype == nil then
          frag_leftover = data:sub(totread+1)
          break
        end

        totread = totread+bread

        if s800_ev_counts and #nscl_buffer>0 and (nscl_buffer[#nscl_buffer].sourceID == 2 or nscl_buffer[#nscl_buffer].sourceID == 2+16) then 
          s800_ev_counts = s800_ev_counts+1
        end

        if orruba_ev_counts and #nscl_buffer>0 and (nscl_buffer[#nscl_buffer].sourceID == 16 or nscl_buffer[#nscl_buffer].sourceID == 2+16) then 
          orruba_ev_counts = orruba_ev_counts+1
        end

        if ptype == "PHYSICS_EVENT_COUNT" then
          if physics_count_buffer.body_header_size == 0 then
            if startingCounts.s800 == nil then
              startingCounts.s800 = physics_count_buffer.event_count
              s800_ev_counts = 0
              last_s800_evtnbr = startingCounts.s800
            else
              last_s800_evtnbr = physics_count_buffer.event_count
            end
          elseif physics_count_buffer.body_header_sourceID then
            if startingCounts.orruba == nil then
              startingCounts.orruba = physics_count_buffer.event_count
              orruba_ev_counts = 0
              last_orruba_evtnbr = physics_count_buffer.event_count
            else
              last_orruba_evtnbr = physics_count_buffer.event_count
            end
          end
        end

        nscl_buffer = {}
        scaler_buffer = {}
        physics_count_buffer = {}
      end

      bread = 0
      totread = 0
    end

    if nevt-last_print > 2000 then
      if startingCounts.s800 and startingCounts.orruba then
        stat_term:Write(string.format("Received: S800 => %10d / %10d ||| ORRUBA => %10d / %10d\r", 
            s800_ev_counts, last_s800_evtnbr-startingCounts.s800, 
            orruba_ev_counts, last_orruba_evtnbr-startingCounts.orruba))
      end

      last_print = nevt

      NSCL_UNPACKER.PostProcessing()

      if ClockTimeDiff(startTime, "second") > 600 then
        startTime = GetClockTime()

        root_logfile:Overwrite()
        root_logfile:Flush()

--        local xfp_integral = online_hists.h_mtdc_tof_xf:Integral(-13, 25)
--        local crdc1x_integral = crdcx_cut:IntegralHist(online_hists.h_crdc1_envsch)

--        xfprate_graph:SetNPoint(xfprate_graph:GetMaxSize()+1)
--        xfprate_graph:SetPoint(xfprate_graph:GetMaxSize(), startTime.sec, xfp_integral/crdc1x_integral)

--        xfprate_clone = xfprate_graph:Clone()

--        xfprate_monitorfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/monitor_xfp_rate.root", "recreate")
--        xfprate_clone:Write()
--        crdcx_cut:Clone():Write()
--        xfprate_monitorfile:Close()

--        xfprate_graph:Update()
      end
    end

--    theApp:ProcessEvents()
  end

  root_logfile:Overwrite()
  root_logfile:Close()
end


function GenerateFilesInput(origin_path, runnbrs)
  local evtfiles = {}

  for i, run in ipairs(runnbrs) do
    local runpath = origin_path.."/run"..run.."/run-"
    if run < 10 then runpath = runpath.."000"..run.."-00.evt"
    elseif run < 100 then runpath = runpath.."00"..run.."-00.evt"
    elseif run < 1000 then runpath = runpath.."0"..run.."-00.evt"
    else runpath = runpath..run.."-00.evt" end

    table.insert(evtfiles, runpath)
  end

  return evtfiles
end

function ReplayNSCLEvt(mode, evtfiles, max_packet, skip_to_physics, outroot)
  nscl_buffer = {}
  scaler_buffer = {}

  NSCL_UNPACKER.PHYSICS_FRAGMENT = true
--  NSCL_UNPACKER.PHYSICS_FRAGMENT = false

  if type(evtfiles) == "string" then
    evtfiles = {evtfiles}
  end

  local s800_file = assert(io.open(evtfiles[1])) 

  local fpos = 0
  local filelength = s800_file:seek("end")
  s800_file:seek("set")

  local packet_counter = 0

  if skip_to_physics then
    while ReadNextPacket(s800_file) ~= "PHYSICS_EVENT" do
      ReadNextPacket(s800_file)
    end
  end

--  debug_log=2
--  debug_log_details.crdc = true

  local buffile
  if outroot then
    buffile = TFile(outroot, "recreate")
  end

  local hists

  if mode == "root" then SetOutputType("root", "file")
  elseif mode == "histograms" then SetOutputType("histograms", "file", evtfiles[1])
  else print("Invalid mode", mode, "(expecting \"root\" or \"histograms\")") end

  if outroot then
    buffile:Write()
  end

  local dumpEvery = 10000
  local prevProgress = 0

  local nevt = 0

  local stat_term = MakeSlaveTerm({bgcolor="Grey42", fgcolor="Grey93", fontstyle="Monospace", fontsize=10, geometry="100x15-0+0"})

  print("Convert process started at")
  local success = os.execute("date")

  stat_term:Write(string.format("\nNumber of file fragments: %d\n", #evtfiles))
  stat_term:Write(string.format("\nStarting treatment of %s\n", evtfiles[1]))

--  online_hists.ic_vs_xfp_corr:Draw("colz")

  while CheckSignals() and (fpos < filelength or #evtfiles > 1) and (max_packet == nil or packet_counter < max_packet) do
    if fpos >= filelength then
      if #evtfiles > 1 then
        fpos = 0
        stat_term:Write(string.format("\nFinished treating %s, switching to %s\n", evtfiles[1], evtfiles[2]))
        table.remove(evtfiles, 1)
        s800_file = assert(io.open(evtfiles[1]))
        filelength = s800_file:seek("end")
        s800_file:seek("set", 0)
        buffile:Overwrite()
        buffile:Flush()

        if mode == "histograms" then
          run_cat = GetRunCategory(evtfiles[1])
          cutsuffix = run_infos[run_cat].cut_suffix

          cfile = TFile("/mnt/hgfs/Dropbox/ORNL/software/luaXroot/user/pid_cuts.root", "read")
          crdc_tac_cut = cfile:GetObject("TCutG", "tac_vs_crdc1x"..cutsuffix)
          active_cuts[crdc_tac_cut:GetName()] = crdc_tac_cut
          angle_tof_cut = cfile:GetObject("TCutG", "angle_tof_cut"..cutsuffix)
          active_cuts[angle_tof_cut:GetName()] = angle_tof_cut
          cfile:Close()

          SetCoincidenceWindow(run_infos[run_cat].coinc_win.low, run_infos[run_cat].coinc_win.high)
        end
      else
        stat_term:Write("\nReached end of last file...\n")
        break
      end
    end

    if debug_log == 0 and packet_counter-prevProgress > dumpEvery then
      prevProgress = packet_counter
      local progress = fpos/filelength
      stat_term:Write(string.format("Processed %5.1f %% (%-12i packets)\r", progress*100, packet_counter))
--    io.write("Processed ", (fpos/filelength)*100, " %\r")

--      if debug_log == 0 then
--        theApp:Update()
--      end
    end

    local ptype, bread = ReadNextPacket(s800_file)

    if ptype ~= nil then
      nevt = nevt+1
      packet_counter = packet_counter+1

      if debug_log == 0 and ptype == "PHYSICS_EVENT" then
        ProcessNSCLBuffer(nscl_buffer, nevt)
      end

      nscl_buffer = {}
      scaler_buffer = {}

      fpos = s800_file:seek("cur")

      NSCL_UNPACKER.PostProcessing()
    end
  end

  stat_term:Write(string.format("\nProcessed %5.1f %% (%-12i packets)\n", 100.0, packet_counter))

  print("Convert process finished at")
  success = os.execute("date")

  if tree then
    tree:Write()
  end

  if outroot then
    buffile:Overwrite()
    buffile:Flush()

    buffile:Close()
  end
end