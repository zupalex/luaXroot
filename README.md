# luaXroot
A binder to use ROOT classes in Lua and merging ROOT TApplication with the Lua interpreter (unix only).

For a more detailed documentation (WIP): [wiki page](https://zupalex.github.io/)

**A basic GUI using Python has been introduced as an experimental feature (December 2017, using Tkinter with Python 2.7). User can customize the GUI via python scripts (adding buttons, fields, ...) using the file user/userscripts.py. An example of user modified GUI will be added soon. It is disabled by default. To enable it, add the following to your userlogon.lua: SetUsePYGui(true). You can alternatively start it by hand after launching luaXroot by typing StartPYGUI()**

What it does:  
- Start a "standard" lua interpreter and run a derived version of the ROOT TApplication in the background.  
      -> This allows to use the ROOT display classes to plot graphs, histograms, functions, ...
    
What it can do:
- A powerful TTree binder allowing the creation and manipulation of TTree from a Lua script without having to interact with the C side at all.
- Provides a base C++ class which can be used to create custom user classes that can be easily compiled from the interpreter.  
      -> Compiles libraries (using ROOT cling) that can be reloaded from a Lua script or from the interpreter.  
      -> These custom classes can be used as TTree branches.  
      -> API functions available to easily create Lua Getter and Setter for class members.  
      -> API functions available to easily create Lua bindings for non default constructors and methods.  
      -> See user/Example/LateCompile.cxx as an example (commented code).
    
 What it has been designed for:
 - Provides a user-friendly environment for data analysis using the flexibility of the Lua scripting language.
 - Provides a framework for "online" data analysis (process data in real-time as they are recorded to disk).
 - Provides tools to unpack binary data to human readable data.  
      -> A module with helper functions to read and decode binary data is provided (/scripts/lua_modules/binaryreader.lua).
    
REQUIREMENTS:
CMake and Readline development libraries are required.
Python and Tkinter module if the GUID feature is enabled.

example for Ubuntu:  
sudo apt-get install cmake libreadline6 libreadline6-dev

example for Fedora 24:  
sudo dnf install cmake readline-devel

To install it:

- Go to the root folder of the repository after you cloned it
- mkdir build; cd build
- cmake build ..
- make install
- For a bash shell, add the following line to your .bashrc (without the quotes)  
"source [path/to/luaXroot]/scripts/thisluaXroot.sh"  
If you are using a different shell use the appropriate command to source the aforementioned file.
- You should be able to start the program from anywhere using "./luaXroot"

# Known Issues/Awaiting fix:

- Drawing more than 4 histograms/graphs on the same canvas might cause a crash when closing it.
