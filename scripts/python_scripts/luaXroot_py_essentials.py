import os
import termios
import array
import select
import socket
import fcntl
import cmd


def get_luaXroot_path():
    return os.environ['LUAXROOTLIBPATH'] + "/../"


def send_to_master(appProps, cmd):
    ready = select.select((), (appProps.socket.fileno(),), ())
    if len(ready[1]) > 0:
        appProps.socket.send(cmd)
        return True
    else:
        return False

    
def receive_master_response(appProps):
    ready = select.select((appProps.lua_to_py_sock.fileno(),), (), ())
    if len(ready[0]) > 0:
        data_length = array.array('i', [0])
        fcntl.ioctl(ready[0][0], termios.FIONREAD, data_length, True)
        cmd = appProps.lua_to_py_sock.recv(data_length[0])
        return cmd
    else:
        return None
