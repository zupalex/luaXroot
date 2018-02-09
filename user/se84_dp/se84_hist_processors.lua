require("nscl_unpacker/nscl_unpacker_cfg")
fillfns = require("ldf_unpacker/ornldaq_monitors")

local can = {}
online_hists = {}
active_cuts = {}
local active_hists = {}

local _AddMonitor = AddMonitor
function AddMonitor(alias, hparams, fillfn)
  _AddMonitor(alias, hparams, fillfn)

  online_hists[hparams.name] = orruba_monitors[hparams.name].hist
end

local function RegisterHistogram(hname, htitle, nbinsx, xmin, xmax, nbinsy, ymin, ymax)
  local htype
  if nbinsy then
    htype = "2D"
    online_hists[hname] = TH2(hname, htitle, nbinsx, xmin, xmax, nbinsy, ymin, ymax)
  else
    htype = "1D"
    online_hists[hname] = TH1(hname, htitle, nbinsx, xmin, xmax)
  end

  haliases[htitle] = {hist=online_hists[hname], type=htype}
end

local orruba_en_min = 0
local orruba_en_max = orruba_applycal and 15 or 4096
local orruba_nbins = orruba_applycal and 1500 or 2048

function SetupNSCLHistograms()
  RegisterHistogram("h_clockdiff", "Difference between S800 and ORNL measure of ORRUBA Clock (1tick = 100ns) over time", 50000, 0, 5000000, 80, -20, 20)
  RegisterHistogram("h_clockdiff_coinc", "(Trig Coincidence) Difference between S800 and ORNL measure of ORRUBA Clock (1tick = 100ns) over time", 50000, 0, 5000000, 80, -20, 20)

  RegisterHistogram("h_crdc1_chs", "CRDC1 Channels", 260, 0, 260)
  RegisterHistogram("h_crdc1_data", "CRDC1 data", 10000, 0, 5000)
  RegisterHistogram("h_crdc1_cal", "CRDC1 cal", 3000, 0, 300)
  RegisterHistogram("h_crdc1_mult", "CRDC1 mult", 300, 0, 300)
  RegisterHistogram("h_crdc1_time", "CRDC1 Anode Time", 4096, 0, 4096)

  RegisterHistogram("h_crdc1_envsch", "CRDC1 energy vs. channel", 300, 0, 300, 2048, 0, 2048)






  RegisterHistogram("h_crdc1_tacvsxgrav", "CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_trigcoinc", "(Trig coinc) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_gateproton", "(Gate Protons) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_gatesidar", "(Gate Sidar) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_gatesx3up", "(Gate SX3 Up) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_gatesx3down", "(Gate Sx3 Down) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)

  RegisterHistogram("h_crdc1_tacvsxgrav_pidunreacted", "(Gate Unreacted?) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pid85se", "(Gate 85Se?) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pidtest1", "(Gate PID1) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pidtest2", "(Gate PID2) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pidtest3", "(Gate PID3) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pidtest4", "(Gate PID4) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)
  RegisterHistogram("h_crdc1_tacvsxgrav_pidtest5", "(Gate PID5) CRDC1 TAC vs. X", 2000, -200, 200, 2048, 0, 4096)








  RegisterHistogram("h_beamangle_vs_tac", "CRDC1 TAC vs. Beam Angle", 2000, -0.1, 0.1, 2048, 0, 4096)

  RegisterHistogram("h_crdc2_chs", "CRDC2 Channels", 260, 0, 260)
  RegisterHistogram("h_crdc2_data", "CRDC2 data", 10000, 0, 5000)
  RegisterHistogram("h_crdc2_cal", "CRDC2 cal", 3000, 0, 300)
  RegisterHistogram("h_crdc2_mult", "CRDC2 mult", 300, 0, 300)
  RegisterHistogram("h_crdc2_time", "CRDC2 Anode Time", 4096, 0, 4096)

  RegisterHistogram("h_crdc2_envsch", "CRDC2 energy vs. channel", 300, 0, 300, 8192, 0, 8192)

  RegisterHistogram("h_scint_de_up", "Scintillator de_up", 8192, 0, 8192)
  RegisterHistogram("h_scint_de_down", "Scintillator de_down", 8192, 0, 8192)

  RegisterHistogram("h_mtdc", "Mesytec TDC Time vs. Channel", 32, 0, 32, 65535, 0, 65535)
  RegisterHistogram("h_mtdc_tof_e1up", "TOF E1 up", 2000, -300, 700)
  RegisterHistogram("h_mtdc_tof_e1down", "TOF E1 down", 2000, -300, 700)
  RegisterHistogram("h_mtdc_tof_xf", "TOF E1 XF", 5000, -1000, 4000)
  RegisterHistogram("h_mtdc_tof_obj", "TOF E1 OBJ", 2000, -300, 700)
  RegisterHistogram("h_mtdc_tof_gal", "TOF E1 Gallotte", 2000, -300, 700)
  RegisterHistogram("h_mtdc_tof_rf", "TOF E1 RF", 2000, -300, 700)
  RegisterHistogram("h_mtdc_tof_hodo", "TOF E1 Hodoscope", 2000, -300, 700)
  RegisterHistogram("h_tofrfxf_vs_tofxfe1", "TOF RF - XF vs. TOF XF - E1", 2000, -300, 700, 2000, -300, 700)

  RegisterHistogram("h_ic", "Ion Chamber Energy vs. Channel", 16, 0, 16, 4096, 0, 4096)
  RegisterHistogram("h_ic_avg", "Ion Chamber Average Energy", 4096, 0, 4096)

  RegisterHistogram("ic_vs_xfp", "Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("ic_vs_xfp_clean", "(Clean) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)

  RegisterHistogram("beamangle_vs_xfp", " Corrected TOF XF - E1 vs. Beam Angle (rad)", 2000, -200, 200, 1600, -0.1, 0.1)





  RegisterHistogram("crdc1x_vs_xfp", "CRDC 1 X vs. Corrected TOF XF - E1", 1600, -100, 300, 2000, -100, 100)
  RegisterHistogram("crdc1x_vs_xfp_gate_sidar", "(Gate Proton SIDAR) CRDC 1 X vs. Corrected TOF XF - E1", 1600, -100, 300, 2000, -100, 100)
  RegisterHistogram("crdc1x_vs_xfp_gate_sx3up", "(Gate Proton SX3 Up) CRDC 1 X vs. Corrected TOF XF - E1", 1600, -100, 300, 2000, -100, 100)
  RegisterHistogram("crdc1x_vs_xfp_gate_sx3down", "(Gate Proton SX3 Down) CRDC 1 X vs. Corrected TOF XF - E1", 1600, -100, 300, 2000, -100, 100)





  RegisterHistogram("h_ornl_envsch_s800coinc", "ORRUBA Energy vs. Channel - S800 coincidence", 899, 0, 899, orruba_nbins, orruba_en_min, orruba_en_max)

  RegisterHistogram("h_s800pid_gate_orruba", "(Gate ORRUBA) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("h_s800pid_gate_orruba_elastics", "(Gate Elastics) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("h_s800pid_gate_orruba_barrel", "(Gate Barrel) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)

  RegisterHistogram("ic_vs_xfp_gate_sidar_protons_nopt", "(SIDAR protons no punch thru) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)

  RegisterHistogram("ic_vs_xfp_gate_sx3up_protons_nopt", "(SX3 Up protons no punch thru) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("ic_vs_xfp_gate_sx3up_protons_pt", "(SX3 Up protons punch thru) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("ic_vs_xfp_gate_sx3up_protons_any", "(SX3 Up protons any) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)

  RegisterHistogram("ic_vs_xfp_gate_sx3down_protons_nopt", "(SX3 Down protons no punch thru) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("ic_vs_xfp_gate_sx3down_protons_pt", "(SX3 Down protons punch thru) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)
  RegisterHistogram("ic_vs_xfp_gate_sx3down_protons_any", "(SX3 Down protons any) Ion Chamber Avergage Energy vs. Corrected TOF XF - E1", 2000, -200, 200, 2000, 1000, 3000)

  RegisterHistogram("trig_pattern", "Trigger Pattern", 20, 0, 20)
end

function SetupORRUBAHistograms()
--  if not orruba_applycal then
  AddMonitor("ORRUBA En vs. Ch", {name="h_ornl_envsch", title="ORNL DAQ Energy vs. Channel", xmin=0, xmax=899, nbinsx=899, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillChVsValue)
--  else
--    AddMonitor("ORRUBA En vs. Ch", {name="h_ornl_envsch", title="ORNL DAQ Energy vs. Channel", xmin=0, xmax=899, nbinsx=899, ymin=0, ymax=15, nbinsy=1500})




--  AddMonitor("SIDAR En vs. Strip", {name = "sidar_en_vs_strip", title = "SIDAR Energy vs. Strip#", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500}, fillfns.FillORRUBAdEvsE())

--  AddMonitor("SIDAR En vs. Strip Gate Protons", {name = "sidar_en_vs_strip_protons_nopt", title = "SIDAR Energy vs. Strip# gate protons", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + Coinc S800", {name = "sidar_en_vs_strip_protons_trig2", title = "SIDAR Energy vs. Strip# gate protons + coinc S800", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + Coinc S800", {name = "sidar_en_maxafter_vs_strip_protons", title = "SIDAR Energy vs. Strip# gate protons + coinc S800", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + Coinc S800", {name = "sidar_en_maxafter_vs_strip_protons_trig2", title = "SIDAR Energy vs. Strip# gate protons + coinc S800", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + E1 in ORNL", {name = "sidar_en_vs_strip_protons_E1ornl", title = "SIDAR Energy vs. Strip# gate protons + E1 is ORNL", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + E1 in ORNL", {name = "sidar_en_all_vs_strip_protons_E1ornl", title = "SIDAR Energy vs. Strip# gate protons + E1 is ORNL", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("SIDAR En vs. Strip Gate Protons + E1 in ORNL", {name = "sidar_en_maxafter_vs_strip_protons_E1ornl", title = "SIDAR Energy vs. Strip# gate protons + E1 is ORNL", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("SIDAR En vs. Strip Gate PID1", {name = "sidar_en_vs_strip_gatepid_test1", title = "SIDAR Energy vs. Strip# Gate PID1", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("SIDAR En vs. Strip Gate PID2", {name = "sidar_en_vs_strip_gatepid_test2", title = "SIDAR Energy vs. Strip# Gate PID2", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("SIDAR En vs. Strip Gate PID3", {name = "sidar_en_vs_strip_gatepid_test3", title = "SIDAR Energy vs. Strip# Gate PID3", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("SIDAR En vs. Strip Gate PID4", {name = "sidar_en_vs_strip_gatepid_test4", title = "SIDAR Energy vs. Strip# Gate PID4", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("SIDAR En vs. Strip Gate PID5", {name = "sidar_en_vs_strip_gatepid_test5", title = "SIDAR Energy vs. Strip# Gate PID5", xmin = 0, xmax = 16, nbinsx = 16, ymin = 0, ymax = 15, nbinsy = 1500})





--  AddMonitor("SIDAR dE vs. E", {name = "sidar_dE_vs_E", title = "SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID Unreacted) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_unreacted", title = "(Gate PID Unreacted) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID 85Se) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_85se", title = "(Gate PID 85Se) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate CRDC Unreacted) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatecrdc_unreacted", title = "(Gate CRDC Unreacted) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate CRDC Left of Unreacted) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatecrdc_leftunreacted", title = "(Gate CRDC Left of unreacted) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID1) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_test1", title = "(Gate PID1) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID2) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_test2", title = "(Gate PID2) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID3) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_test3", title = "(Gate PID3) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID4) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_test4", title = "(Gate PID4) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID5) SIDAR dE vs. E", {name = "sidar_dE_vs_E_gatepid_test5", title = "(Gate PID5) SIDAR dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})


--    AddMonitor("SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E", title = "SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID1) SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E_gatepid_test1", title = "(Gate PID1) SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID2) SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E_gatepid_test2", title = "(Gate PID2) SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID3) SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E_gatepid_test3", title = "(Gate PID3) SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID4) SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E_gatepid_test4", title = "(Gate PID4) SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--    AddMonitor("(Gate PID5) SX3 Upstream dE vs. E", {name = "sx3u_dE_vs_E_gatepid_test5", title = "(Gate PID5) SX3 Upstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})



--    AddMonitor("SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E", title = "SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})
--  end

--  AddMonitor("(Gate PID1) SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E_gatepid_test1", title = "(Gate PID1) SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("(Gate PID2) SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E_gatepid_test2", title = "(Gate PID2) SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("(Gate PID3) SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E_gatepid_test3", title = "(Gate PID3) SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("(Gate PID4) SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E_gatepid_test4", title = "(Gate PID4) SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})

--  AddMonitor("(Gate PID5) SX3 Downstream dE vs. E", {name = "sx3d_dE_vs_E_gatepid_test5", title = "(Gate PID5) SX3 Downstream dE vs. E", xmin = 0, xmax = 15, nbinsx = 1500, ymin = 0, ymax = 15, nbinsy = 1500})



--  AddMonitor("SX3 vs X3 mapping", {name = "sx3_x3_mapping", title = "SX3 vs X3 mapping", xmin = 401, xmax = 499, nbinsx = 98, ymin = 601, ymax = 616, nbinsy = 15}, function(hist, ev) 
--      local max_dE, max_E = {en=0, strip=-1}, {en=0, strip=-1}

--      for s1=401,499 do
--        if ev[s1] and ev[s1] > 2 and ev[s1] > max_E.en then
--          max_E.en = ev[s1]
--          max_E.strip = s1
--        end
--      end

--      if max_E.strip > 0 then
--        for s2=601,615 do
--          if ev[s2] and ev[s2] > 1 and ev[s2] > max_dE.en then
--            max_dE.en = ev[s2]
--            max_dE.strip = s2
--          end
--        end
--        if max_dE.strip > 0 and max_E.strip > 0 then 
--          orruba_monitors.sx3_x3_mapping.hist:Fill(max_E.strip, max_dE.strip) 
--        end
--      end
--    end)

--    AddMonitor("ORRUBA En vs. Ch - S800 coincidence", {name="h_ornl_envsch_s800coinc", title="ORNL DAQ Energy vs. Channel - S800 coincidence", xmin=0, xmax=899, nbinsx=899, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillChVsValue)

--  for detid=1, 12 do
--    for strip=1, 4 do
--      local hname, htitle, detkey

--      hname = string.format("SX3_U%d_resistive_%d", detid, strip)
--      htitle = string.format("SuperX3 U%d front strip %d", detid, strip)
--      detkey = string.format("SuperX3 U%d", detid)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=0, xmax=4096, nbinsx=512, ymin=0, ymax=4096, nbinsy=512}, fillfns.FillSX3LeftVsRight(detkey, strip))

--      hname = string.format("SX3_U%d_position_%d", detid, strip)
--      htitle = string.format("SuperX3 U%d Energy vs Position strip %d", detid, strip)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=orruba_en_min, ymax=orruba_en_max, nbinsy=orruba_nbins}, fillfns.FillSX3EnergyVsPosition(detkey, strip))

--      hname = string.format("SX3_U%d_position_%d_enback", detid, strip)
--      htitle = string.format("SuperX3 U%d Energy vs Position %d using backside energy", detid, strip)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=orruba_en_min, ymax=orruba_en_max, nbinsy=orruba_nbins}, fillfns.FillSX3EnergyVsPosition(detkey, strip, true))

--      hname = string.format("SX3_D%d_resistive_%d", detid, strip)
--      htitle = string.format("SuperX3 D%d front strip %d", detid, strip)
--      detkey = string.format("SuperX3 D%d", detid)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=0, xmax=4096, nbinsx=512, ymin=0, ymax=4096, nbinsy=512}, fillfns.FillSX3LeftVsRight(detkey, strip))

--      hname = string.format("SX3_D%d_position_%d", detid, strip)
--      htitle = string.format("SuperX3 D%d Energy vs Position strip %d", detid, strip)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=orruba_en_min, ymax=orruba_en_max, nbinsy=orruba_nbins}, fillfns.FillSX3EnergyVsPosition(detkey, strip))

--      hname = string.format("SX3_D%d_position_%d_enback", detid, strip)
--      htitle = string.format("SuperX3 D%d Energy vs Position strip %d using backside energy", detid, strip)
--      AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=orruba_en_min, ymax=orruba_en_max, nbinsy=orruba_nbins}, fillfns.FillSX3EnergyVsPosition(detkey, strip, true))
--    end
--  end

--  for strip=1,4 do
--    hname = string.format("Elastics_TR_%d", strip)
--    htitle = string.format("Elastics Top Right Energy vs Position strip %d", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics TOP_RIGHT", strip))

--    hname = string.format("Elastics_TR_%d_back", strip)
--    htitle = string.format("Elastics Top Right Energy vs Position strip %d using backside", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics TOP_RIGHT", strip, true))

--    hname = string.format("Elastics_BR_%d", strip)
--    htitle = string.format("Elastics Bottom Right Energy vs Position strip %d", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics BOTTOM_RIGHT", strip))

--    hname = string.format("Elastics_BR_%d_back", strip)
--    htitle = string.format("Elastics Bottom Right Energy vs Position strip %d using backside", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics BOTTOM_RIGHT", strip, true))

--    hname = string.format("Elastics_BL_%d", strip)
--    htitle = string.format("Elastics Bottom Left Energy vs Position strip %d", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics BOTTOM_LEFT", strip))

--    hname = string.format("Elastics_BL_%d_back", strip)
--    htitle = string.format("Elastics Bottom Left Energy vs Position strip %d using backside", strip)
--    AddMonitor(htitle, {name = hname, title = htitle, xmin=-1, xmax=1, nbinsx=200, ymin=0, ymax=4096, nbinsy=2048}, fillfns.FillSX3EnergyVsPosition("Elastics BOTTOM_LEFT", strip, true))
--  end

--  AddMonitor("MCP1 X MPD4", {name = "MCP1_X_MPD4", title = "MCP1 X Position MPD4", xmin = 0, xmax = 1024, nbinsx = 512}, fillfns.FillMCP("MCP 1 MPD4", "x"))
--  AddMonitor("MCP1 Y MPD4", {name = "MCP1_Y_MPD4", title = "MCP1 Y Position MPD4", xmin = 0, xmax = 1024, nbinsx = 512}, fillfns.FillMCP("MCP 1 MPD4", "y"))
--  AddMonitor("MCP2 X MPD4", {name = "MCP2_X_MPD4", title = "MCP2 X Position MPD4", xmin = 0, xmax = 4096, nbinsx = 2048}, fillfns.FillMCP("MCP 2 MPD4", "x"))
--  AddMonitor("MCP2 Y MPD4", {name = "MCP2_Y_MPD4", title = "MCP2 Y Position MPD4", xmin = 0, xmax = 4096, nbinsx = 2048}, fillfns.FillMCP("MCP 2 MPD4", "y"))

--  AddMonitor("MCP1 X vs. Y MPD4", {name = "MCP1_XvsY_MDB4", title = "MCP1 X vs. Y MPD4", xmin = -1, xmax = 1, nbinsx = 1500, ymin = -1, ymax = 1, nbinsy = 1500}, fillfns.FillMCP("MCP 1 MPD4", "x vs. y"))
--  AddMonitor("MCP2 X vs. Y MPD4", {name = "MCP2_XvsY_MDB4", title = "MCP2 X vs. Y MPD4", xmin = -1, xmax = 1, nbinsx = 1000, ymin = -1, ymax = 1, nbinsy = 1000}, fillfns.FillMCP("MCP 2 MPD4", "x vs. y"))

--  AddMonitor("MCP1 X QDC", {name = "MCP1_X_QDC", title = "MCP1 X Position with QDC", xmin = 0, xmax = 1024, nbinsx = 512}, fillfns.FillMCP("MCP 1 QDC", "x"))
--  AddMonitor("MCP1 Y QDC", {name = "MCP1_Y_QDC", title = "MCP1 Y Position with QDC", xmin = 0, xmax = 1024, nbinsx = 512}, fillfns.FillMCP("MCP 1 QDC", "y"))
--  AddMonitor("MCP2 X QDC", {name = "MCP2_X_QDC", title = "MCP2 X Position with QDC", xmin = 0, xmax = 4096, nbinsx = 2048}, fillfns.FillMCP("MCP 2 QDC", "x"))
--  AddMonitor("MCP2 Y QDC", {name = "MCP2_Y_QDC", title = "MCP2 Y Position with QDC", xmin = 0, xmax = 4096, nbinsx = 2048}, fillfns.FillMCP("MCP 2 QDC", "y"))

--  AddMonitor("MCP1 X vs. Y QDC", {name = "MCP1_XvsY_QDC", title = "MCP1 X vs. Y with QDC", xmin = -1, xmax = 1, nbinsx = 1500, ymin = -1, ymax = 1, nbinsy = 1500}, fillfns.FillMCP("MCP 1 QDC", "x vs. y"))
--  AddMonitor("MCP2 X vs. Y QDC", {name = "MCP2_XvsY_QDC", title = "MCP2 X vs. Y with QDC", xmin = -1, xmax = 1, nbinsx = 1000, ymin = -1, ymax = 1, nbinsy = 1000}, fillfns.FillMCP("MCP 2 QDC", "x vs. y"))
end

AddSignal("display", function(hname, opts)
    if haliases[hname] then
      haliases[hname].hist:Draw(opts)
      active_hists[hname] = haliases[hname].hist
    elseif online_hists[hname] then
      online_hists[hname]:Draw(opts)
      active_hists[hname] = online_hists[hname]
    elseif active_cuts[hname] then
      active_cuts[hname]:Draw(opts)
    end
  end)

AddSignal("display_multi", function(divx, divy, hists)
    local can = TCanvas()
    can:Divide(divx, divy)

    for i, hinfo in ipairs(hists) do
      local row_ = math.floor((i-1)/divy)+1
      local col_ = i - divy*(row_-1)
      if online_hists[hname] then
        can:Draw(online_hists[hinfo.hname],hinfo.opts, row_, col_)
        active_hists[hinfo.hname] = online_hists[hinfo.hname]
      end

      if haliases[hinfo.hname] then
        can:Draw(haliases[hinfo.hname].hist, hinfo.opts, row_, col_)
        active_hists[hinfo.hname] = haliases[hinfo.hname].hist
      elseif online_hists[hinfo.hname] then
        can:Draw(online_hists[hinfo.hname], hinfo.opts, row_, col_)
        active_hists[hinfo.hname] = online_hists[hinfo.hname]
      end
    end
  end)

local function checknamematch(name, matches)
  if #matches == 0 then return true end

  for _, m in ipairs(matches) do
    if name:find(m) == nil then
      return false
    end
  end

  return true
end

AddSignal("ls", function(alias, matches, retrieveonly)
    local matching_hists = {}
    if alias == nil or alias then
      for k, v in pairs(haliases) do
        if checknamematch(k, matches) then 
          if not retrieveonly then print(v.type, "\""..tostring(k).."\"") end
          table.insert(matching_hists, k)
        end
      end
    else
      for k, v in pairs(online_hists) do
        if checknamematch(k, matches) then
          if not retrieveonly then print(v.type, "\""..tostring(k).."\"") end
          table.insert(matching_hists, k)
        end
      end
    end

    local result = #matching_hists > 0 and table.concat(matching_hists, "\\li") or "no results"

    SetSharedBuffer(result)
  end)

AddSignal("unmap", function(hname)
    active_hists[hname] = nil
    active_hists[hname] = nil
  end)

AddSignal("zeroallhists", function()
    for k, v in pairs(online_hists) do
      v:Reset()
    end

    theApp:Update()
  end)

function SetupOfflineCanvas()
  can[1] = TCanvas()
  can[1]:Draw(online_hists.h_ornl_envsch, "colz")

--  can[1] = TCanvas()
--  can[2] = TCanvas()

--  for i=1,2 do
--    can[i]:Divide(2, 3)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_chs"], "", 1, 1)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_data"], "", 1, 2)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_cal"], "", 1, 3)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_mult"], "", 2, 1)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_time"], "", 2, 2)
--    can[i]:Draw(online_hists["h_crdc"..tostring(i).."_envsch"], "colz", 2, 3)
--    can[i]:SetLogScale(2, 3, "Z", true)

--    can[i]:SetTitle("CRDC"..tostring(i))
--    can[i]:SetWindowSize(900, 700)
--  end

--  can[3] = TCanvas()
--  can[3]:Divide(1, 2)

--  can[3]:Draw(online_hists.h_scint_de_up, "", 1, 1)
--  can[3]:Draw(online_hists.h_scint_de_down, "", 1, 2)
--  can[3]:SetTitle("Scintillators")
--  can[3]:SetWindowSize(600, 400)

--  can[4] = TCanvas()
--  can[4]:Divide(1, 2)

--  can[4]:Draw(online_hists.h_ic_avg, "", 1, 1)
--  can[4]:Draw(online_hists.h_ic, "colz", 1, 2)
--  can[4]:SetLogScale(1, 2, "Z", true)
--  can[4]:SetTitle("Ion Chamber")
--  can[4]:SetWindowSize(600, 400)

--  can[5] = TCanvas()
--  can[5]:Divide(3, 3)

--  can[5]:Draw(online_hists.h_mtdc, "colz", 1, 1); can[5]:SetLogScale(1, 1, "Z", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_e1up, "", 1, 2); can[5]:SetLogScale(1, 2, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_e1down, "", 1, 3); can[5]:SetLogScale(1, 3, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_xf, "", 2, 1); can[5]:SetLogScale(2, 1, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_obj, "", 2, 2); can[5]:SetLogScale(2, 2, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_gal, "", 2, 3); can[5]:SetLogScale(2, 3, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_rf, "", 3, 1); can[5]:SetLogScale(3, 1, "Y", true)
--  can[5]:Draw(online_hists.h_mtdc_tof_hodo, "", 3, 2); can[5]:SetLogScale(3, 2, "Y", true)
--  can[5]:Draw(online_hists.h_tofrfxf_vs_tofxfe1, "colz", 3, 3); can[5]:SetLogScale(3, 3, "Z", true)
--  can[5]:SetTitle("Mesytec TDCs")
--  can[5]:SetWindowSize(1100, 900)

--  can[6] = TCanvas()
--  can[6]:Draw(online_hists.h_clockdiff, "colz")
end

function UpdateNSCLCanvas()
  for i, v in ipairs(can) do
    v:Update()
  end

--  for k, v in pairs(active_hists) do
--    v:Update()
--  end
end

function InitOfflineDisplay(hists)
--  hists.h_clockdiff:Draw("colz")
end

function InitOnlineDisplay()
  online_hists.h_clockdiff:Draw("colz")
end

function UpdateNSCLHistograms()
  -- online_hists.h_clockdiff:Update()

  for k, v in pairs(active_hists) do
    v:Update()
  end
end

function Initialization(input_type)
  SetupNSCLHistograms()
  SetupORRUBAHistograms()

  if input_type:lower() == "online" then
    InitOnlineDisplay()
  elseif input_type:lower() == "file" then
    SetupOfflineCanvas(hists)
  end
end

function Showh(hname, opts)
  SendSignal("rslistener", "display", hname, opts)
end

function Unmaph(hname)
  SendSignal("rslistener", "unmap", hname)
end

function ListHistograms(alias, ...)
  local matches

  if type(alias) == "string" then
    matches = table.pack(alias, ...)
    alias = true
  else
    matches = table.pack(...)
  end

  SendSignal("rslistener", "ls", alias, matches, false)
end

---------------------------------------------------------------
----------------------- PROCESSORS ----------------------------
---------------------------------------------------------------


local CRDCProcessor = {
  anode = function(crdcnum, time)
    online_hists["h_crdc"..tostring(crdcnum).."_time"]:Fill(time)
  end,

  raw = function(crdcnum, pad, en_avg)
    online_hists["h_crdc"..tostring(crdcnum).."_chs"]:Fill(pad)
    online_hists["h_crdc"..tostring(crdcnum).."_data"]:Fill(en_avg)
    online_hists["h_crdc"..tostring(crdcnum).."_envsch"]:Fill(pad, en_avg)
  end,

  cal = function(crdcnum, mult, xgravity)
    online_hists["h_crdc"..tostring(crdcnum).."_mult"]:Fill(mult)
    online_hists["h_crdc"..tostring(crdcnum).."_cal"]:Fill(xgravity)
  end,
}

local ScintProcessor = {
  up = function(val)
    online_hists.h_scint_de_up:Fill(val)
  end,

  down = function(val)
    online_hists.h_scint_de_down:Fill(val)
  end,
}

local IonChamberProcessor = {
  matrix = function(channel, value)
    online_hists.h_ic:Fill(channel, value)
  end,

  average = function(val)
    online_hists.h_ic_avg:Fill(val)
  end,
}

local MTDCProcessor = {
  matrix = function(channel, value)
    online_hists.h_mtdc:Fill(channel, value)
  end,

  tofs = function(chname, tof)
    online_hists["h_mtdc_tof_"..chname]:Fill(tof)
  end,

  pids = function(hname, tof1, tof2)
    online_hists[hname]:Fill(tof1, tof2)
  end
}

local ORRUBAProcessor = function(orruba_data, orruba_cal)
  for k, v in pairs(orruba_monitors) do
    if v.fillfn then v.fillfn(v.hist, orruba_data, orruba_cal) end
  end
end

local CorrelationProcessor = {
  ic_vs_xfp = function(tof, ic_en)
    online_hists.ic_vs_xfp:Fill(tof, ic_en)
  end,

  ic_vs_xfp_clean = function(tof, ic_en)
    online_hists.ic_vs_xfp_clean:Fill(tof, ic_en)
  end,

  beamangle_vs_xfp = function(tof, angle, cf)
    online_hists.beamangle_vs_xfp:Fill(tof+angle*cf, angle)
  end,

  crdc1x_vs_xfp = function(tof, crdc1x, cf)
    local tof_corr = tof+crdc1x*cf
    online_hists.crdc1x_vs_xfp:Fill(tof_corr, crdc1x)

    if validate_SIDAR_protons_nopt then
      online_hists.crdc1x_vs_xfp_gate_sidar:Fill(tof_corr, buf_mem.crdc1x)
    end

    if validate_SX3UP_protons_any then
      online_hists.crdc1x_vs_xfp_gate_sx3up:Fill(tof_corr, buf_mem.crdc1x)
    end

    if validate_SX3DOWN_protons_any then
      online_hists.crdc1x_vs_xfp_gate_sx3up:Fill(tof_corr, buf_mem.crdc1x)
    end
  end,

  h_crdc1_tacvsxgrav = function(x, tac)
    online_hists.h_crdc1_tacvsxgrav:Fill(x, tac)
  end,

  h_beamangle_vs_tac = function(angle, tac)
    online_hists.h_beamangle_vs_tac:Fill(angle, tac)
  end,
}

NSCL_UNPACKER.SetCRDCProcessor(CRDCProcessor)
--NSCL_UNPACKER.SetHodoProcessor()
NSCL_UNPACKER.SetScintProcessor(ScintProcessor)
NSCL_UNPACKER.SetIonChamberProcessor(IonChamberProcessor)
NSCL_UNPACKER.SetMTDCProcessor(MTDCProcessor)
NSCL_UNPACKER.SetTriggerProcessor(function() end)
--NSCL_UNPACKER.SetTOFProcessor()
NSCL_UNPACKER.SetORRUBAProcessor(ORRUBAProcessor)

NSCL_UNPACKER.SetCorrelationProcessor(CorrelationProcessor)

NSCL_UNPACKER.SetPostProcessing(function()
    UpdateNSCLHistograms()
  end)


return Initialization