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
    self.listBox.insert(END, "ldf_unpacker/ldf_onlinereader", "nscl_unpacker/nscl_unpacker")

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
      self.appProps.socket.send("require(\"nscl_unpacker/se84_scripts\"); sleep(1); StartNewTask(\"rslistener\", \"StartListeningRingSelector\");") 
      
      if hasattr(self, "histPanel"):
        self.histPanel.destroy()
        
      self.histPanel = Frame(self.master)
      HistogramPanel(self.histPanel, self.appProps, "rslistener")
      self.histPanel.pack()


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
    
    self.doColz = IntVar()
    self.colzButton = Checkbutton(self.frame, text = 'colz', var=self.doColz)
    self.colzButton.grid(row=3, column=3, rowspan=2, pady=5)
    
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
    
    if nhx > 1 or nhy > 1:
      cmd = "SendSignal(\""+self.taskname+"\",\"display_multi\","+str(nhx)+","+str(nhy)+",{"
      
      for sel in selected_hists:
        cmd = cmd + "{hname="+"\""+self.listBox.get(sel)+"\", opts=\"\"},"
      
      cmd = cmd + "})"
    else:
      cmd = "SendSignal(\""+self.taskname+"\",\"display\",\""+self.listBox.get(selected_hists[0])
      if self.doColz.get() == 1:
        cmd = cmd+"\", \"colz"
      cmd = cmd+"\")"

    self.appProps.socket.send(cmd)