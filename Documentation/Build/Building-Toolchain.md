The first step of the build process is getting a system toolchain to work with.  Clang / LLVM is a toolchain that is designed with extensibility and new operating systems in mind and so with that and some added changes, a build needs to happen.  It's a very large and complex system so it takes a long time to compile but here is a detailed description of what happens in the process.

### Chicken / Egg
---------------

In order to properly cross compile for any system you need two things:

1. A compiler capable of emitting compatible machine code for the target architecture
2. A system root, which is just a fancy way of saying a folder in which you pretend that folder is the entire world (i.e. sysroot/usr would be considered /usr on the real device)

Usually this requires multiple rounds of compilation in order to generate a bare boned compilation environment in which to generate the sysroot so that things can then be compiled again against the just made sysroot, but that's time consuming so to save time borrrdex has precompiled the starter sysroot and hosted it.  The script for building the toolchain first downloads and extracts this.

### Binutils
-----------

First of all, a custom binutils is made.  Binutils is a collection of programs for creating binary artifacts that a given system knows how to use (i.e. executables, shared libraries, etc).  Because of the way binutils works this requires patching a handful of scripts inside of the configuration file in order to get it to recognize that "borrrdex" is a valid target to make binaries for.  Essentially, it is just a copy of any other target that produces ELF binaries with x86 instructions, but it allows things like setting standard library search paths and provides for future expansion if necessary.  The toolchain script first patches in these changes, and then builds and installs these files alongside the sysroot from above.

> /home/borrrden/.local/share/borrrdex/bin/<br />
├── x86_64-borrrdex-addr2line<br />
├── x86_64-borrrdex-ar<br />
├── x86_64-borrrdex-as<br />
├── x86_64-borrrdex-c++filt<br />
├── x86_64-borrrdex-elfedit<br />
├── x86_64-borrrdex-gprof<br />
├── x86_64-borrrdex-ld<br />
├── x86_64-borrrdex-nm<br />
├── x86_64-borrrdex-objcopy<br />
├── x86_64-borrrdex-objdump<br />
├── x86_64-borrrdex-ranlib<br />
├── x86_64-borrrdex-readelf<br />
├── x86_64-borrrdex-size<br />
├── x86_64-borrrdex-strings<br />
└── x86_64-borrrdex-strip<br />

Now if you have object files from a compilation for some reason, you can use these programs to generate binary files that borrrdex understands.  However most likely you don't because there is no compile yet.  That's the next step!

### clang / LLVM
---------------

clang is a C / C++ (among other languages) compiler that belongs to the LLVM ecosystem.  It generates bytecode for LLVM, and then LLVM optimizes it and in turn generates machine code with it, and calls out to some backend tools to create the final binaries.  In this case, it will be using the tools from binutils that we just created above.  In order to accomplish this, like with binutils, modification is necessary to get LLVM to recognize borrrdex as a valid portion of a target triple.  For example, at the moment clang will recognize x86_64-linux as valid, but x86_64-borrrdex will confuse it and it will refuse to do anything.  The changes are in [this fork](https://github.com/borrrden/llvm-project) of LLVM on the release/11.x branch.  Basically, like binutils, it is 90% similar to other targets that emit x86 / x64 machine code, except there are some variations in the system defines and include paths.  

The compilation of this takes quite a while, as there are over 3000 heavy files to compile in this project.  However when finished, the toolchain script will install this alongside the binutils above:

> /home/borrrden/.local/share/borrrdex/bin/
├── borrrdex-clang -> clang<br />
├── borrrdex-clang++ -> clang++<br />
├── clang -> clang-11<br />
├── clang++ -> clang<br />
├── clang-11<br />
├── clang-check<br />
├── clang-cl -> clang<br />
├── clang-cpp -> clang<br />
├── clang-extdef-mapping<br />
├── clang-format<br />
├── clang-offload-bundler<br />
├── clang-offload-wrapper<br />
├── clang-refactor<br />
├── clang-rename<br />
├── clang-scan-deps<br />
├── diagtool<br />
├── git-clang-format<br />
├── hmaptool<br />
├── ld.lld -> lld<br />
├── ld64.lld -> lld<br />
├── lld<br />
├── lld-link -> lld<br />
├── llvm-ar<br />
├── llvm-cov<br />
├── llvm-cxxfilt<br />
├── llvm-lib -> llvm-ar<br />
├── llvm-nm<br />
├── llvm-objcopy<br />
├── llvm-objdump<br />
├── llvm-profdata<br />
├── llvm-ranlib -> llvm-ar<br />
├── llvm-rc<br />
├── llvm-size<br />
├── llvm-strings<br />
├── llvm-strip -> llvm-objcopy<br />
├── llvm-symbolizer<br />
├── scan-build<br />
├── scan-view<br />
├── wasm-ld -> lld<br />
├── x86_64-borrrdex-addr2line<br />
├── x86_64-borrrdex-ar<br />
├── x86_64-borrrdex-as<br />
├── x86_64-borrrdex-c++filt<br />
├── x86_64-borrrdex-elfedit<br />
├── x86_64-borrrdex-gprof<br />
├── x86_64-borrrdex-ld<br />
├── x86_64-borrrdex-ld.bfd<br />
├── x86_64-borrrdex-nm<br />
├── x86_64-borrrdex-objcopy<br />
├── x86_64-borrrdex-objdump<br />
├── x86_64-borrrdex-ranlib<br />
├── x86_64-borrrdex-readelf<br />
├── x86_64-borrrdex-size<br />
├── x86_64-borrrdex-strings<br />
└── x86_64-borrrdex-strip<br />

And if you run `borrrdex-clang -v` you will notice the custom output:

> borrrdex-clang -v<br />
Borrrdex clang version 11.1.0 (https://github.com/borrrden/llvm-project b21c8d7cc8003bb059a421032ace35aebdec271c)<br />
Target: x86_64-unknown-borrrdex<br />
Thread model: posix<br />
InstalledDir: /home/borrrden/.local/share/borrrdex/bin

### Limine
-------------

Limine is the bootloader that borrrdex uses so the last step of the toolchain script is downloading it and installing it alongside the two tools above.  It is much faster than both of them and results in some binary files that need to be copied to the disk image later.