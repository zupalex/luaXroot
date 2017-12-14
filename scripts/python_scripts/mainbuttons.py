from Tkinter import *
import termios
import array
import select
import socket
import fcntl

def load_main_buttons(owner, master):
  owner.common_modules_button = Button(owner.frame, text = 'Common Modules', width = 25, command = owner.open_common_modules_list)
  owner.common_modules_button.pack(padx=15, pady=5, side=TOP)
  
  owner.e16025_macros_button = Button(owner.frame, text = 'Common Macros', width = 25, command = owner.open_e16025_macros_list)
  owner.e16025_macros_button.pack(padx=15, pady=5, side=TOP)

class CommonModulesBox:
  def __init__(self, master, appProps):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("CommonModulesBox", self)
    self.frame = Frame(self.master)
    self.title = Label(self.frame, text= 'Commonly used modules')
    self.listBox = Listbox(self.frame, width = 50)
    self.title.pack()
    self.listBox.pack(fill=BOTH, expand=1)
    
    self.sendCommand = Button(self.frame, text = 'Load', width = 50, command = self.send_command)
    self.sendCommand.pack()
    
    self.frame.pack(fill=BOTH, expand=1)
    self.listBox.insert(END, "ldf_unpacker/ldf_onlinereader", "nscl_unpacker/nscl_unpacker", "nscl_unpacker/se84_scripts")

  def send_command(self):
    self.appProps.socket.send("require(\""+self.listBox.get(self.listBox.curselection())+"\")")
  
  
class E16025MacrosBox:
  def __init__(self, master, appProps):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("E16025MacrosBox", self)
    self.frame = Frame(self.master)
    self.title = Label(self.frame, text= 'e16025 Specific Macros')
    self.listBox = Listbox(self.frame, width = 50)
    self.title.pack()
    self.listBox.pack(fill=BOTH, expand=1)
    
    self.sendCommand = Button(self.frame, text = 'Load', width = 50, command = self.send_command)
    self.sendCommand.pack()
    
    self.frame.pack(fill=BOTH, expand=1)
    self.listBox.insert(END, "Attach to ORNL DAQ Output", "Listen to Ringselector")

  def send_command(self):
    selected_macro = self.listBox.get(self.listBox.curselection())
    
    if selected_macro == "Attach to ORNL DAQ Output":
      self.appProps.socket.send("require(\"ldf_unpacker/ldf_onlinereader\"); StartNewTask(\"ornlmonitor\", \"AttachToORNLSender\", \"192.168.31.10:51031\");") 
      
      if hasattr(self, "histPanel"):
        self.histPanel.destroy()
        
      self.histPanel = Frame(self.master)
      HistogramPanel(self.histPanel, self.appProps, "ornlmonitor")
      self.histPanel.pack()
      
    if selected_macro == "Listen to Ringselector":
      self.rsprompt = Toplevel(self.master)
      Label(self.rsprompt, text="Select Source").pack()
      
      self.sourceList = Listbox(self.rsprompt, width=25, exportselection=0)
      self.sourceList.insert(END, "masterevb", "s800filter", "orruba")
      self.sourceList.pack()
      
      Label(self.rsprompt, text="Accept Items").pack()
      
      self.acceptList = Listbox(self.rsprompt, width=25, selectmode=MULTIPLE, exportselection=0)
      self.acceptList.insert(END, "PHYSICS_EVENT", "PHYSICS_EVENT_COUNT")
      self.acceptList.pack()
      
      Button(self.rsprompt, text = 'Validate', width = 10, command = self.start_listening_ringselector).pack(side=LEFT)
      
      cbframe = Frame(self.rsprompt)
      
      self.dumpCbVal = IntVar()
      self.dumpCb = Checkbutton(cbframe, text="Raw Dump", variable=self.dumpCbVal)
      self.dumpCb.grid(row=0, column=0, sticky=W)
      
      self.calibrateCbVal = IntVar()
      self.calibrateCb = Checkbutton(cbframe, text="Calibrate", variable=self.calibrateCbVal)
      self.calibrateCb.grid(row=1, column=0, sticky=W)
      
      cbframe.pack(side=RIGHT)
      
  def start_listening_ringselector(self):
    if len(self.sourceList.curselection()) == 0:
      return
      
    if len(self.acceptList.curselection()) == 0:
      return
    
    cmd = "require(\"nscl_unpacker/se84_scripts\"); sleep(1); StartNewTask(\"rslistener\", \"StartListeningRingSelector\""
    if self.dumpCbVal.get() == 1:
      cmd = cmd + ",true,"
    else:
      cmd = cmd + ",false,"
    
    source = self.sourceList.get(self.sourceList.curselection()[0])
    cmd = cmd + "\"" + source + "\","
    
    accept = self.acceptList.curselection()
    
    cmd = cmd + "\""
    
    for idx, sel in enumerate(accept):
      cmd = cmd + self.acceptList.get(sel)
      if idx < len(accept):
        cmd = cmd+","
    
    cmd = cmd + "\","
    
    if self.calibrateCbVal.get() == 1:
      cmd = cmd + "true)"
    else:
      cmd = cmd + "false)"
    
    self.appProps.socket.send(cmd) 
      
    if hasattr(self, "histPanel"):
      self.histPanel.destroy()

    self.histPanel = Frame(self.master)
    HistogramPanel(self.histPanel, self.appProps, "rslistener")
    self.histPanel.pack()
    self.rsprompt.destroy()
    
    self.additionalControls = Frame(self.master)
    Se84AdditionalControls(self.additionalControls, self.appProps, "rslistener")
    self.additionalControls.pack()


class HistogramPanel:
  def __init__(self, master, appProps, taskname):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("HistogramPanel", self)
    self.taskname = taskname
    
    self.frame = Frame(self.master)
    self.title = Label(self.frame, text= 'Histogram Pannel')
    
    self.title.grid(row=0, column=0, columnspan=6, padx=5, pady=5)
    
    self.filterButton = Button(self.frame, text = 'Filter:', width = 10, command = self.process_filter)
    self.filterIF = Entry(self.frame, width = 35)
    
    self.filterButton.grid(row=1, column=0, padx=5, pady=5)
    self.filterIF.grid(row=1, column=1, columnspan=5, padx=5, pady=5)
    
    self.max_select = 1
    self.listBox = Listbox(self.frame, width=55, selectmode=MULTIPLE)
    self.listBox.grid(row=2, column=0, columnspan=6, padx=5, pady=5)
    self.listBox.bind("<<ListboxSelect>>", self.checkselection_callback)
    
    self.drawButton = Button(self.frame, text = 'Draw', width = 10, command = self.draw_selection)
    self.drawButton.grid(row=3, column=2, rowspan=2, pady=5)
    
    self.doSame = IntVar()
    self.sameButton = Checkbutton(self.frame, text = 'same', var=self.doSame)
    self.sameButton.grid(row=3, column=3, sticky=W, pady=5)
    
    self.doColz = IntVar()
    self.colzButton = Checkbutton(self.frame, text = 'colz', var=self.doColz)
    self.colzButton.grid(row=4, column=3, sticky=W, pady=5)
    
    self.gridXVal = StringVar()
    self.gridYVal = StringVar()
    self.gridXVal.trace("w", lambda name, index, mode, xval=self.gridXVal, yval=self.gridYVal: self.adjust_max_selection(xval, yval))
    self.gridYVal.trace("w", lambda name, index, mode, panel=self, xval=self.gridXVal, yval=self.gridYVal: self.adjust_max_selection(xval, yval))
    
    self.divideXLabel = Label(self.frame, text = 'Grid X')
    self.diveideXIF = Entry(self.frame, width = 10, textvariable=self.gridXVal)
    self.gridXVal.set("1")
        
    self.divideXLabel.grid(row=3, column=0, padx=5, pady=5)
    self.diveideXIF.grid(row=3, column=1, padx=5, pady=5)
    
    self.divideYLabel = Label(self.frame, text = 'Grid Y')
    self.diveideYIF = Entry(self.frame, width = 10, textvariable=self.gridYVal)
    self.gridYVal.set("1")
    
    self.divideYLabel.grid(row=4, column=0, padx=5, pady=5)
    self.diveideYIF.grid(row=4, column=1, padx=5, pady=5)
    
    self.frame.pack()
    
  def adjust_max_selection(self, xif, yif):
    if hasattr(self, "previous_selection"):
      del self.previous_selection
      
    gridxval = 1
    gridyval = 1
    
    if len(xif.get()) > 0:
      try:
        gridxval = int(xif.get())
      except ValueError:
        self.gridXVal.set("1")
    else:
      gridxval = 1
    
    if len(yif.get()) > 0:
      try:
        gridyval = int(yif.get())
      except ValueError:
        self.gridYVal.set("1")
    else:
      gridyval = 1
      
    self.max_select = gridxval * gridyval
  
  def checkselection_callback(self, dummy):
    if not hasattr(self, "previous_selection"):
      self.previous_selection = list()
        
    selects = self.listBox.curselection()
  
    if self.max_select == 1:
      if len(self.previous_selection) > 0 and len(selects) > 0:
        if selects[0] == self.previous_selection[0]:
          self.listBox.selection_clear(selects[0])
        else:
          self.listBox.selection_clear(selects[1])
    
    else:      
      if len(selects) > self.max_select:
        for sel in selects:
          if not sel in self.previous_selection:
            self.listBox.selection_clear(sel)
    
    self.previous_selection = self.listBox.curselection()
  
  def process_filter(self):
    if hasattr(self, "previous_selection"):
      del self.previous_selection
      
    hist_filter = self.filterIF.get()
    self.appProps.socket.send("_pyguifns.GetHMonitors(\""+self.taskname+"\",\""+hist_filter+"\")")
    rready = select.select((self.appProps.lua_to_py_sock.fileno(),), (), ())
    if len(rready[0]) > 0:
      data_length = array.array('i', [0])
      fcntl.ioctl(rready[0][0], termios.FIONREAD, data_length, True)
      cmd = self.appProps.lua_to_py_sock.recv(data_length[0])
      if cmd == "no results":
        self.listBox.delete(0, END)
      else:
        hist_list = cmd.split("\\li")
        self.listBox.delete(0, END)
        for hist in hist_list:
          self.listBox.insert(END, hist)
        
  def draw_selection(self):
    selected_hists = self.listBox.curselection()
    nhx = int(self.diveideXIF.get())
    nhy = int(self.diveideYIF.get())
    
    cmd = ""
    
    doColz = self.doColz.get() == 1
    doSame = self.doSame.get() == 1
    
    if doSame:
      cmd = ""
      for sel in selected_hists:
        cmd = cmd+"SendSignal(\""+self.taskname+"\",\"display\",\""+self.listBox.get(sel)+"\",\"same\");"
    elif nhx > 1 or nhy > 1:
      cmd = "SendSignal(\""+self.taskname+"\",\"display_multi\","+str(nhx)+","+str(nhy)+",{"
      
      for sel in selected_hists:
        cmd = cmd + "{hname="+"\""+self.listBox.get(sel)+"\", opts="
        if doColz:
          cmd = cmd + "\"colz\""
        else:
          cmd = cmd + "\"\""
          
        cmd = cmd + "},"
      
      cmd = cmd + "})"
    else:
      cmd = "SendSignal(\""+self.taskname+"\",\"display\",\""+self.listBox.get(selected_hists[0])
      if doColz:
        cmd = cmd+"\", \"colz"
      cmd = cmd+"\")"

    self.appProps.socket.send(cmd)
    

class Se84AdditionalControls:
  def __init__(self, master, appProps, taskname):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("Se84AdditionalControls", self)
    self.taskname = taskname
    
    self.frame = Frame(self.master, borderwidth=4, relief=GROOVE)
    self.title = Label(self.frame, text= 'Se84 Monitor Contols')
    self.title.grid(row=0, column=0, columnspan=5, padx=5, pady=5)
    
    self.coinWinLabel = Label(self.frame, text = 'Coincidence Window')
    self.coinWinLabel.grid(row=1, column=0, columnspan=2, padx=5, pady=5)
    
    self.coinWinLowLabel = Label(self.frame, text = 'Low')
    self.coinWinLow = Entry(self.frame, width = 5)
    self.coinWinLow.insert(0, "-5")
    
    self.coinWinLowLabel.grid(row=2, column=0, sticky=W, padx=5, pady=5)
    self.coinWinLow.grid(row=2, column=1, padx=5, pady=5)
    
    self.coinWinHighLabel = Label(self.frame, text = 'High')
    self.coinWinHigh = Entry(self.frame, width = 5)
    self.coinWinHigh.insert(0, "5")
    
    self.coinWinHighLabel.grid(row=3, column=0, sticky=W, padx=5, pady=5)
    self.coinWinHigh.grid(row=3, column=1, padx=5, pady=5)
    
    self.setCoincWinButton = Button(self.frame, text = 'Set', width = 5, command = self.set_coincwin)
    
    self.setCoincWinButton.grid(row=2, column=2, rowspan=2, padx=5, pady=5)
    
    self.zeroHistsButton = Button(self.frame, text = 'Zero Hists', width = 5, command = self.zero_hists)
    
    self.zeroHistsButton.grid(row=2, column=4, columnspan=2, sticky=E, padx=5, pady=5)
    
    self.frame.pack()
    
  def set_coincwin(self):     
    low_bound = -5
    high_bound = 5
    
    if len(self.coinWinLow.get()) > 0:
      try:
        low_bound = int(self.coinWinLow.get())
      except ValueError:
        self.coinWinLow.delete(0, END)
        self.coinWinLow.insert(0, "-5")
    else:
      self.coinWinLow.delete(0, END)
      self.coinWinLow.insert(0, "-5")
  
    if len(self.coinWinHigh.get()) > 0:
      try:
        high_bound = int(self.coinWinHigh.get())
      except ValueError:
        self.coinWinHigh.delete(0, END)
        self.coinWinHigh.insert(0, "5")
    else:
      self.coinWinHigh.delete(0, END)
      self.coinWinHigh.insert(0, "5")
    
    cmd = "SendSignal(\"rslistener\", \"setcoincwindow\","
    cmd = cmd + str(low_bound) + "," + str(high_bound) + ")"

    self.appProps.socket.send(cmd)
    
  
  def zero_hists(self):
    self.confirmZero = Toplevel(self.master)
    self.confirmZero.label = Label(self.confirmZero, text="Confirm Zeroing ALL\nthe histograms?", padx=5, pady=5)
    self.doZeroButton = Button(self.confirmZero, text="Confirm", width = 5, command = self.do_zero_all_hists, padx=5, pady=5)
    self.cancelZeroButton = Button(self.confirmZero, text="Cancel", width = 5, command = self.cancel_zero_all_hists, padx=5, pady=5)
    self.confirmZero.label.pack()
    self.doZeroButton.pack(side=LEFT)
    self.cancelZeroButton.pack(side=RIGHT)
    
  def do_zero_all_hists(self):
    cmd = "SendSignal(\"rslistener\", \"zeroallhists\")"
    self.appProps.socket.send(cmd)
    self.confirmZero.destroy()
    
  def cancel_zero_all_hists(self):
    self.confirmZero.destroy()
    