# luaXroot
A binder to use ROOT classes in Lua and merging ROOT TApplication with the Lua interpreter.

To install it:

- Go to the root folder of the repository after you cloned it
- mkdir build; cd build
- cmake build ..
- make install
- For a bash shell, add the following line to your .bashrc (without the quotes)
  "source /mnt/hgfs/Dropbox/ORNL/luaXroot/scripts/thisluaXroot.sh"
  If you are using a different shell use the appropriate command to source the aforementioned file.
- You should be able to start the program from anywhere using "./luaXroot"
