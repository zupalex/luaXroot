# luaXroot
A binder to use ROOT classes in Lua and merging ROOT TApplication with the Lua interpreter (unix only).

What it does:  
- Start a "standard" lua interpreter and run a derived version of the ROOT TApplication in the background.  
      -> This allows to use the ROOT display classes to plot graphs, histograms, functions, ...
    
What it can do:
- A powerful TTree binder allowing the creation of branches for a TTree from a Lua script and manipulate them without having to interact with the C side at all.
- Provide a base C++ class that can be used to create custom user classes that can then be compiled easily from the interpreter.  
      -> Compiles libraries (using ROOT cling) that can be reloaded from a Lua script or from the interpreter.  
      -> These custom classes can be used as TTree branches.  
      -> API functions available to easily create Lua Getter and Setter for class members.  
      -> API functions available to easily create Lua bindings for non default constructors and methods.  
      -> See user/Example/LateCompile.cxx as an example (commented code).
    
 What it has been designed for:
 - Provide a user-friendly environment for data analysis using the flexibility of the Lua scripting language.
 - Provide a framework for "online" data analysis (analyse data in real-time as they are recorded to disk).
 - Provide tools to unpack binary data to human readable data.  
      -> A module with helper functions to read and decode binary data is provided (/scripts/lua_modules/binaryreader.lua).
    
REQUIREMENTS:
CMake and Readline development libraries are required.

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
"source /mnt/hgfs/Dropbox/ORNL/luaXroot/scripts/thisluaXroot.sh"  
If you are using a different shell use the appropriate command to source the aforementioned file.
- You should be able to start the program from anywhere using "./luaXroot"
