This step takes place after successfully building the toolchain.  Now that the toolchain is ready for building things the base of the operating system can begin to be built.  

### C++ stuff
---------------

Recall that a sysroot directory was made in the previous section.  This will continue to be used now.  First, symlinks will be made so that the sysroot include directory will be able to utilize the newly build C++ headers from the toolchain, and the runtime libraries like libc++.so and family will be copied to the sysroot lib directory.  Remember that anything that goes in here can be used on the OS, so now the OS will be capable of running C++ programs built with clang++.  

### Project Configuration
-------------------------

Next the various parts of the OS and userspace are configured for compilation so that the root level Makefile can build them.  The projects that get configured are:

- LibBor: A userspace library containing OS specific functions, in particular rendering text and graphics to the screen.  
- kernel: The kernel of the OS, which should be pretty self explanatory
- System: Some userspace level system utilities (right now all that is present is a console program)

Each of these will be set up via CMake, and the `Makefile` will build any or all of them (or running `ninja` in their build directories also works)

### Userspace Libraries
-------------------

There are also some third party libraries present in the Ports/ folder.  These are built during this step and installed into the sysroot so that they will be available on the OS.  There is a script called `buildport.sh` in that folder which takes the name of the port to build and is responsible for downloading, building, and installing it.