function CheckClockDiff()
  nscl_buffer = {}
  scaler_buffer = {}
  ldf_buffer = {}

--  s800_file = assert(io.open("/user/e16025/s800tempfiles/run224.evt")) 
--  s800_file = assert(io.open("/user/e16025/s800tempfiles/run223.evt")) 
--  s800_file = assert(io.open("/user/e16025/s800tempfiles/run222.evt")) 
  s800_file = assert(io.open("/user/e16025/s800tempfiles/run221.evt")) 
--  s800_file = assert(io.open("/user/e16025/s800tempfiles/run220.evt")) 
--  s800_file = assert(io.open("/user/e16025/s800tempfiles/run219.evt")) 

--  debug_log = 2

  require("ldf_decoder")
--  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800_morning02.ldf")
--  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800_morning01.ldf")
--  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800_overnight03.ldf")
  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800_overnight02.ldf")
--  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800_overnight01.ldf")
--  SetLDFTrackedFile("/user/e16025/Downloads/orruba_data/S800test02.ldf")

  local fpos = 0
  local filelength = s800_file:seek("end")
  s800_file:seek("set")

  h_tdiff = TH2("h_tdiff", "Time difference between s800 clock measured by nscl daq and ornl daq", 10^3, 0, 10^8, 10^3, 0, 10^8)
  h_ts_vs_time_nscl = TH2("h_ts_vs_time_nscl", "s800 clock measured by nscl daq", 10^3, 0, 10^8, 5*10^2, 0, 5*10^8)
  h_ts_vs_time_ornl = TH2("h_ts_vs_time_ornl", "s800 clock measured by ornl daq",10^3, 0, 10^8, 5*10^2, 0, 5*10^8)

  h_s800clock_nscl = TH1("h_s800clock_nscl", "s800 clock measured by nscl daq", 5*10^6, 0, 5*10^8)
  h_s800clock_ornl = TH1("h_s800clock_ornl", "s800 clock measured by ornl daq", 5*10^6, 0, 5*10^8)

  h_drift_vs_time = TGraph(0)
  h_jump_vs_time = TGraph(0)

  local evt_nbr = 0
  local prev_evt_nbr = 0
  local dump_every = 10000

  local clock_frequency = 10^7
  local trigger_rate = 30.5

  local clock_warning_increment = (clock_frequency/trigger_rate)*0.25
  local next_clock_warning = clock_warning_increment
  local event_diff = 0

  local last_s800_ts, last_ornl_ts

  while fpos < filelength do
    if (evt_nbr-prev_evt_nbr) > dump_every then
      io.write("Read ", evt_nbr, " events\r")
      io.flush()
      prev_evt_nbr = evt_nbr
    end

    local read_ret, ldf_head_type = ReadNextRecord()

    while read_ret ~= 4 and read_ret ~= nil do
      read_ret = ReadNextRecord()
    end

    local n_ldf_evt = #ldf_buffer

    if n_ldf_evt == 0 then
      if read_ret ~= nil then
        print("WARNING: no clock in the ldf file???")
      else
        while ReadNextPacket(s800_file) ~= nil do
          ReadNextPacket(s800_file)
        end

        if #nscl_buffer > 0 then
          print("WARNING!!! Reached end of ORNL file but there is still data left in NSCL file =>", #nscl_buffer)
        end

        print("REACHED THE END OF THE ORNL FILE........", ldf_head_type)
        break
      end
    end

    for i=1,n_ldf_evt do
      read_ret = ReadNextPacket(s800_file)
      if read_ret == nil then
        while ReadNextRecord() ~= nil do
          ReadNextRecord()
        end
        print("WARNING!!! Reached end of NSCL file while we still had", #ldf_buffer-i, "orruba events")
        break 
      end

      while read_ret ~= "PHYSICS_EVENT" do
        nscl_buffer.prev_packet_pos = nil
        nscl_buffer.this_packet_pos = nil
        read_ret = ReadNextPacket(s800_file)
        if read_ret == nil then
          while ReadNextRecord() ~= nil do
            ReadNextRecord()
          end
          print("WARNING!!! Reached end of NSCL file while we still had", #ldf_buffer-i, "orruba events")
          break 
        end
      end

      if read_ret == nil then
        while ReadNextRecord() ~= nil do
          ReadNextRecord()
        end
        print("WARNING!!! Reached end of NSCL file while we still had", #ldf_buffer-i, "orruba events")
        break 
      end

      local nscl_s800_clock = nscl_buffer[i].timestamp.low + (nscl_buffer[i].timestamp.med<<16) + (nscl_buffer[i].timestamp.high<<32)
      local ornl_s800_clock = ldf_buffer[i].s800clock.low + (ldf_buffer[i].s800clock.med<<16) + (ldf_buffer[i].s800clock.high<<32)

      local clock_diff_tbl = {}
      clock_diff_tbl.low = nscl_buffer[i].timestamp.low - ldf_buffer[i].s800clock.low
      clock_diff_tbl.med = nscl_buffer[i].timestamp.med - ldf_buffer[i].s800clock.med
      clock_diff_tbl.high = nscl_buffer[i].timestamp.high - ldf_buffer[i].s800clock.high

      local ts_diff = clock_diff_tbl.low + (clock_diff_tbl.med<<16) + (clock_diff_tbl.high<<32)

--      if false then
      if math.abs(ts_diff) > next_clock_warning or nscl_s800_clock == 0 or (event_diff ~= nscl_buffer.evtnbr-evt_nbr-1) then
        if math.abs(ts_diff) > next_clock_warning then
          print("WARNING: clock difference changed significantly at", evt_nbr)
          next_clock_warning = ts_diff+clock_warning_increment

          local jvst_np = h_drift_vs_time:GetMaxSize()
          h_drift_vs_time:SetNPoint(jvst_np+1)
          h_jump_vs_time:SetNPoint(jvst_np+1)

          local prev_jump = jvst_np > 0 and h_drift_vs_time:GetPoint(jvst_np-1)[2] or 0

          h_drift_vs_time:SetPoint(jvst_np, evt_nbr, ts_diff)
          h_jump_vs_time:SetPoint(jvst_np, evt_nbr, ts_diff-prev_jump)
        end
        if nscl_s800_clock == 0 then
          print("WARNING: s800 clock is ZERO", evt_nbr)
        end
        if (event_diff ~= nscl_buffer.evtnbr-evt_nbr-1) then
          print("WARNING: event number differs not incremented by 1 at", evt_nbr)
          event_diff = nscl_buffer.evtnbr-evt_nbr-1
        end

        print(string.format("             CLOCK        LOW     MED     HIGH"))
        print(string.format("S800   %15i   %5i   %5i   %5i", nscl_s800_clock, nscl_buffer[i].timestamp.low, nscl_buffer[i].timestamp.med, nscl_buffer[i].timestamp.high))
        print(string.format("ORRUBA %15i   %5i   %5i   %5i", ornl_s800_clock, ldf_buffer[i].s800clock.low, ldf_buffer[i].s800clock.med, ldf_buffer[i].s800clock.high))
        print("-------------------------------------------------")
        print(string.format("DIFF   %15i   %5i   %5i   %5i", ts_diff, clock_diff_tbl.low, clock_diff_tbl.med, clock_diff_tbl.high))
        print("*************************************************************************")

        print("ORRUBA clock read by ORNL and NSCL DAQs for adjacent events (ORNL / diff with previous event / NSCL / diff with previous event)")
        local adjacent_clocks = {}
        adjacent_clocks.orruba = {}
        adjacent_clocks.nscl = {}

        for j=i-5,i+5 do
          if j > 0 and j <= #ldf_buffer then
            table.insert(adjacent_clocks.orruba, ldf_buffer[j].s800clock.low + (ldf_buffer[j].s800clock.med<<16) + (ldf_buffer[j].s800clock.high<<32))
            if j <= i then table.insert(adjacent_clocks.nscl, nscl_buffer[j].timestamp.low + (nscl_buffer[j].timestamp.med<<16) + (nscl_buffer[j].timestamp.high<<32)) end
            if i == j then adjacent_clocks.jumpat = #adjacent_clocks.orruba end
          end
        end

        local nearby_scalers = {}
        for j, v in ipairs(scaler_buffer) do
          nearby_scalers[#scaler_buffer-j+1] = v.timestamp
        end

        print("NUMBER OF SCALERS READ FOR UP UNTIL NOW FOR THIS RECORD =", #nearby_scalers)
        print("")

        for j=1, #adjacent_clocks.orruba do
          while #nearby_scalers > 0 and adjacent_clocks.nscl[j] and adjacent_clocks.nscl[j] > nearby_scalers[#nearby_scalers] do
            print("SCALER HERE ===>", nearby_scalers[#nearby_scalers], j>1 and adjacent_clocks.nscl[j-1]-nearby_scalers[#nearby_scalers] or "")
            nearby_scalers[#nearby_scalers] = nil
          end

          if j == adjacent_clocks.jumpat then
            print(adjacent_clocks.orruba[j], j > 1 and adjacent_clocks.orruba[j]-adjacent_clocks.orruba[j-1] or "", adjacent_clocks.nscl[j], j > 1 and adjacent_clocks.nscl[j]-adjacent_clocks.nscl[j-1] or "", " <= Jump Occured Here")
          else
            print(adjacent_clocks.orruba[j], j > 1 and adjacent_clocks.orruba[j]-adjacent_clocks.orruba[j-1] or "", j <= adjacent_clocks.jumpat and adjacent_clocks.nscl[j] or "", (j > 1 and j <= adjacent_clocks.jumpat)and adjacent_clocks.nscl[j]-adjacent_clocks.nscl[j-1] or "")
          end
        end

        print("*************************************************************************")

        print("Complete Dump of the S800 event for your own pleasure")
        if nscl_buffer.prev_packet_pos then 
          s800_file:seek("set", nscl_buffer.prev_packet_pos)
          debug_log = 2
          ReadNextPacket(s800_file)
          debug_log = 0
          ReadNextPacket(s800_file)
          nscl_buffer[#nscl_buffer] = nil
          nscl_buffer[#nscl_buffer] = nil
        end
        print("=======================================================================")
        print("=======================================================================")
        print("=======================================================================")
      end

--      print("Clock Diff:", ts_diff)

      h_s800clock_nscl:Fill(nscl_s800_clock/100)
      h_s800clock_ornl:Fill(ornl_s800_clock/100)      

      h_ts_vs_time_nscl:Fill(evt_nbr, nscl_s800_clock/100)
      h_ts_vs_time_ornl:Fill(evt_nbr, ornl_s800_clock/100)

      h_tdiff:Fill(evt_nbr, ts_diff)
      evt_nbr = evt_nbr+1
    end

    last_s800_ts = nscl_buffer[#nscl_buffer].timestamp.low + (nscl_buffer[#nscl_buffer].timestamp.med<<16) + (nscl_buffer[#nscl_buffer].timestamp.high<<32)
    last_ornl_ts = ldf_buffer[#ldf_buffer].s800clock.low + (ldf_buffer[#ldf_buffer].s800clock.med<<16) + (ldf_buffer[#ldf_buffer].s800clock.high<<32)

    ldf_buffer = {}
    nscl_buffer = {}
    scaler_buffer = {}

    fpos = s800_file:seek("cur")
  end

  print("")

  return {ts_diff=h_tdiff, s800clock_ornl=h_s800clock_ornl, s800clock_nscl=h_s800clock_nscl}, {drift_over_time=h_drift_vs_time, jumps_over_time=h_jump_vs_time}, {evt_nbr, last_ornl_ts, last_s800_ts}
end
