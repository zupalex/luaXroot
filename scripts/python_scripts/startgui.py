from Tkinter import *
import sys
import fcntl
import os
import termios
import socket
import time
import select
import array
import signal
from multiprocessing import Process

LUAXROOTLIBPATH = os.environ['HOME']
main_pyscripts_path = LUAXROOTLIBPATH+"/../scripts/python_scripts"

sys.path.insert(0, main_pyscripts_path)

from mainbuttons import *



class AppProperties:
  def __init__(self):
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.master_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.lua_to_py_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self._stopExec = False
    
    self.windows = dict()
  
  def add_window(self, name, win):
    self.windows[name] = win


class WelcomeBox:
  def __init__(self, master, appProps):
    self.master = master
    self.appProps = appProps
    self.appProps.add_window("WelcomeBox", self)
    self.frame = Frame(self.master)
    
    load_main_buttons(self, master)
    
    self.frame.pack()
    
  def open_common_modules_list(self):
    if hasattr(self, "submenu"):
      self.submenu.destroy()
    
    self.submenu = Frame(self.master)
    CommonModulesBox(self.submenu, self.appProps)
    self.submenu.pack()

  def open_e16025_macros_list(self):
    if hasattr(self, "submenu"):
      self.submenu.destroy()
      
    self.submenu = Frame(self.master)
    E16025MacrosBox(self.submenu, self.appProps)
    self.submenu.pack()
  
def listen_for_messages(tkroot, appProps):
  data_length = array.array('i', [0])

  while not appProps._stopExec:
    rready = select.select((appProps.master_socket.fileno(),), (), ())
    if len(rready[0]) > 0:
      fcntl.ioctl(rready[0][0], termios.FIONREAD, data_length, True)
      host_msg = appProps.master_socket.recv(data_length[0])
      
      if host_msg == "terminate process":
        appProps._stopExec = True

def main_loop(tkroot):
  tkroot.mainloop()

def sigint_handler(signum, frame):
  pass

def main():
  signal.signal(signal.SIGINT, sigint_handler)

  globProperties = AppProperties()

  if len(sys.argv) > 1:
    skiparg = 0
    
    for idx, arg in enumerate(sys.argv):
      if skiparg > 0:
        skiparg -= 1
        continue
      if idx > 0:
        if arg == "--print":
          skiparg = 1
          print(sys.argv[idx+1])
        if arg == "--socket":
          skiparg = 2
          globProperties.socket.connect(("127.0.0.1", int(sys.argv[idx+1])))
          globProperties.master_socket.connect(("127.0.0.1", int(sys.argv[idx+2])))
          globProperties.lua_to_py_sock.connect(("127.0.0.1", int(sys.argv[idx+2])))
  
  root = Tk()
  app = WelcomeBox(root, globProperties)
  
  p1 = Process(target=listen_for_messages, args=(root,globProperties,))
  p1.start()
  
  p2 = Process(target=main_loop, args=(root,))
  p2.start()
  
  p1.join()
  
  globProperties.socket.close()
    
  p2.terminate()

if __name__ == '__main__':
  main()