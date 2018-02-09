from time import *
from Tkinter import *
import Tkinter, Tkconstants, tkFileDialog
from luaXroot_py_essentials import *


class Se84UnpackerBox:

  def __init__(self, master, appProps):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("Se84UnpackerBox", self)
    self.frame = Frame(self.master, borderwidth=1, relief=RAISED, background="cadetblue3")
    self.title = Label(self.frame, text='84Se(d,p) unpacker', background="cadetblue3")
    self.title.pack()
    
    self.compileDetClasses = Button(self.frame, text='Compile Detector Classes', width=19, background="cadetblue2", command=self.compile_detclasses)
    self.compileDetClasses.pack(padx=20, pady=15, side=TOP)
    
    self.convertRun = Button(self.frame, text='Convert Run', width=19, background="cadetblue2", command=self.convert_run)
    self.convertRun.pack(pady=5, side=TOP)
    
    self.convertMultiRuns = Button(self.frame, text='Convert Multiple Runs', width=19, background="cadetblue2", command=self.convert_multi_runs)
    self.convertMultiRuns.pack(pady=5, side=TOP)
    
    self.convertByFolder = Button(self.frame, text='Convert By Folder', width=19, background="cadetblue2", command=self.convert_by_folder)
    self.convertByFolder.pack(pady=5, side=TOP)
    
    self.frame.pack(fill=BOTH, expand=1)

  def compile_detclasses(self):
    send_to_master(self.appProps, "CompileC(\"" + get_luaXroot_path() + "user/se84_dp/se84_detclasses.cxx\", \"se84_detclasses\")")
    
  def convert_run(self):
    run_to_convert = tkFileDialog.askopenfilename(initialdir="/", title="Select Run", filetypes=(("evt files", "*.evt"),))
    
    if len(run_to_convert) == 0:
        return
    
    input_dir = run_to_convert[0:run_to_convert.rfind("/") + 1]
    output_file = tkFileDialog.asksaveasfilename(initialdir=input_dir, title="Output File", filetypes=(("ROOT files", "*.root"),))

    send_to_master(self.appProps, "dofile(\"" + get_luaXroot_path() + "user/se84_dp/se84_scripts.lua\");"
                                  "ReplayNSCLEvt(\"root\", \"" + run_to_convert + "\", nil, true, \"" + output_file + "\")")
    
  def convert_multi_runs(self):
    runs_to_convert = tkFileDialog.askopenfilenames(initialdir="/", title="Select Run", filetypes=(("evt files", "*.evt"),))
    
    if len(runs_to_convert) == 0:
        return
    
    runs_list = list(runs_to_convert)
    
    input_dir = runs_list[0][0:runs_list[0].rfind("/") + 1]
    output_dir = tkFileDialog.askdirectory(initialdir=input_dir, title="Output Directory")
    output_dir = output_dir + "/"
    
    for run in runs_list:
        output_file = runs_list[0][runs_list[0].rfind("/") + 1:]
        output_file = output_file[0:output_file.rfind(".evt")]
        output_file = output_dir + output_file + ".root"
    
        sent = send_to_master(self.appProps, "dofile(\"" + get_luaXroot_path() + "user/se84_dp/se84_scripts.lua\");"
                                            "ReplayNSCLEvt(\"root\", \"" + runs_to_convert[0] + "\", nil, true, \"" + output_file + "\")")
        
        print("sent command for", run)
        sleep(2)
    
  def convert_by_folder(self):            
    src_dir = tkFileDialog.askdirectory(initialdir="/", title="Select Source Directory")
    
    if len(src_dir) == 0:
        return
    
    src_list = sorted(os.listdir(src_dir))
    
    runs_list = list()
    
    for run_dir in src_list:
        if os.path.isdir(src_dir + "/" + run_dir):
            files_list = os.listdir(src_dir + "/" + run_dir)
            
            for file in files_list:
                if file.find(".evt") >= 0:
                    runs_list.append(run_dir)
                    break
            
    def toggle_select():
        for runcb in self.runsListFrame.runs:
            runcb[0].toggle()
            
    def select_all():
        for runcb in self.runsListFrame.runs:
            runcb[0].select()
            
    def deselect_all():
        for runcb in self.runsListFrame.runs:
            runcb[0].deselect()
    
    def start_conversion():
        send_to_master(self.appProps, "dofile(\"" + get_luaXroot_path() + "user/se84_dp/se84_scripts.lua\")")
        
        output_dir = tkFileDialog.askdirectory(initialdir=src_dir, title="Output Directory")
        output_dir = output_dir + "/"
         
        for runcb in self.runsListFrame.runs:
            if runcb[1].get() == 1:
                output_file = runcb[2]
                output_file = output_file[0:output_file.rfind(".evt")]
                output_file = output_dir + output_file + ".root"
                
                files_list = os.listdir(src_dir + "/" + runcb[2])
                
                run_frags = list()
                
                for file in files_list:
                    if file.find(".evt") >= 0:
                        frag_num = file[file.find(".evt")-2:file.find(".evt")]
                        run_frags.append((src_dir + "/" + runcb[2] + "/" + file, frag_num))
                
                run_frags.sort(key=lambda entry: entry[1])
                
                formated_runs_list = "{"
                
                for frag in run_frags:
                    formated_runs_list = formated_runs_list + "\"" + frag[0] + "\","
                    
                formated_runs_list = formated_runs_list + "}"
    
                send_to_master(self.appProps, "ReplayNSCLEvt(\"root\", " + formated_runs_list + ", nil, true, \"" + output_file + "\")")
        
                sleep(2)
    
    self.selectButtonsFrame = Frame(self.frame, background="cadetblue3")
    self.selectButtonsFrame.pack(pady=10)
    
    self.selectAllRunsButton = Button(self.selectButtonsFrame, text='Select All', width=8, background="cadetblue2", command=select_all)
    self.selectAllRunsButton.pack(side=LEFT, padx=4)
    
    self.deselectAllRunsButton = Button(self.selectButtonsFrame, text='Deselect All', width=8, background="cadetblue2", command=deselect_all)
    self.deselectAllRunsButton.pack(side=LEFT, padx=4)
    
    self.toggleSelectRunsButton = Button(self.selectButtonsFrame, text='Toggle Select', width=8, background="cadetblue2", command=toggle_select)
    self.toggleSelectRunsButton.pack(side=LEFT, padx=4)
    
    self.mainSelectionFrame = Frame(self.frame)
    self.mainSelectionFrame.pack(padx=5, pady=5)
    
    self.runsSelectionCanvas = Canvas(self.mainSelectionFrame, borderwidth=0, width=250)
    self.runsListFrame = Frame(self.runsSelectionCanvas)
    
    self.vsb = Scrollbar(self.mainSelectionFrame, orient="vertical", command=self.runsSelectionCanvas.yview)
    self.runsSelectionCanvas.configure(yscrollcommand=self.vsb.set)
    
    self.vsb.pack(side="right", fill="y")
    self.runsSelectionCanvas.pack(side="left", fill="both", expand=True)
    self.runsSelectionCanvas.create_window((0, 0), window=self.runsListFrame, anchor="nw", tags="self.runsListFrame")
    
    self.runsListFrame.runs = list()
    self.runsListFrame.cbstates = list()
    
    for run in runs_list:
        cbstate = IntVar()
        run_entry = Checkbutton(self.runsListFrame, text=run, variable=cbstate)
        
        self.runsListFrame.runs.append((run_entry, cbstate, run))
        
        run_entry.pack(anchor="w")
    
    self.runsSelectionCanvas.update()
    self.runsSelectionCanvas.configure(scrollregion=self.runsSelectionCanvas.bbox("all"))
    
    self.startConversion = Button(self.frame, text='Start Conversion', width=15, background="cadetblue2", command=start_conversion)
    self.startConversion.pack(pady=10)
    
       
def open_84se_unpacker_gui(win):
    if hasattr(win, "submenu"):
        win.submenu.destroy()
    
    win.submenu = Frame(win.master.mainWin)
    Se84UnpackerBox(win.submenu, win.appProps)
    win.submenu.pack()


def add_evt_converter_button(master):
    wb = master.welcomeBox
    
    wb.se84_unpacker_button = Button(wb.frame, text='84Se(d,p) unpacker', width=25, command=lambda: open_84se_unpacker_gui(wb))
    wb.se84_unpacker_button.pack(padx=15, pady=5, side=TOP)
