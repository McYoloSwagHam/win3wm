# WinWM
WinWM is a tiling window manager inspired by i3wm, it was initially private and commerical, but I decided to opensource it.

## Manual
for the config options please check the page [here](google.com "lmao")

## Architecture
 * twm - is the actual Tiling Window Manager logic, that handles the trees, layouts, Config, input, etc...
 * ForceResize - is the DLL that is to be injected in all applications to allow resizing a window past its limiits
 * WinHook - is the DLL that is to be injected in all applications to notify the man application 
 * x86ipc - Due to WinAPI constraints, there needs to be a child process for interacting with x86 processes
 * WinWMGUI - was the C# gui that was to be used for licensing purposes.
 
 ## Building
 * twm needs to be built for x64
 * ForceResize and WinHook need to be built for both x86 and x64
 * x86ipc needs to be built for x86
 * WinWMGUI can be built for either x86/x64
 
## TODO
- [ ] focus switching on cursor movement and not hotkey
- [ ] save tree layout and restore on restart.
- [ ] CrashHandling (restart on crash)
- [ ] Focus system similar to i3wm (current focus system is basic)
