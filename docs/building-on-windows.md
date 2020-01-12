
# Building Executor 2000 Microsoft Windows

Executor 2000 can be built for Microsoft Windows, albeit only as a 32-bit
Windows Desktop (i.e. Win32) application. 
The build procedure is nearly the
same, however, some additional dependencies need to be installed, and then,
some extra options need to be passed into CMake.

The following procedure has been used to prepare a Windows 10 machine to build
Executor 2000 as a Windows app.  Some of Microsoft's
[free, time-use-limited Windows VM images](https://developer.microsoft.com/en-us/windows/downloads/virtual-machines)
have, in the past, been sufficient to build Executor 2000.  If you try this,
please note that these VMs did have some, but not all of the above dependencies
already installed.

Command line steps be performed once Visual Studio 2017 is installed, by
running the Start Menu searchable program, ```Developer Command Prompt for VS 2017```

1. install [Git](https://git-scm.com/download)
2. install [CMake](https://cmake.org/download/)
3. install [LLVM](http://releases.llvm.org/download.html). (8.0.0, pre-built
   binaries have been used, for example.)
4. install [Visual Studio 2017](https://visualstudio.microsoft.com/), plus it's
   [Desktop Development with C++](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=vs-2017)
   option 
5. install the VS-2017 add-on,
   [LLVM Compiler Toolchain](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.llvm-toolchain)
6. install perl and bison, via MinGW-get's packages for
   ```msys-bison-bin``` and ```msys-perl-bin``` - [MinGW Getting Started](http://www.mingw.org/wiki/getting_started)
7. add perl and bison to path.  If MinGW is installed to C:\MinGW, the
   directory to add to PATH should be, ```C:\MinGW\msys\1.0\bin```.
8. install [vcpkg](https://github.com/Microsoft/vcpkg).  Installation to
   ```C:\vcpkg``` is recommended (but not strictly required, perhaps).
9. use vcpkg to build and install Qt5, SDL2, and Boost.  This may take
   multiple hours to perform (due primarily to Qt).
   1. ```cd C:\vcpkg```
   2. ```vcpkg install sdl2 boost-filesystem boost-system qt5-base```

With the above steps performed, building was done using the normal procedure,
as listed above), but with the cmake-configuration step (aka. ```cmake ..```)
being issued as such:

```cmake -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -T"llvm" ..```

The important and new parts here are:

  - ```-DCMAKE_TOOLCHAIN_FILE=...```: lets the build system know about the
    vcpkg-installed dependencies.
  - ```-T"llvm"```: lets the build system know to use Clang to compile
    C/C++ code (rather than Microsoft's Visual C++ engine, for example).

By default, the build step (of ```cmake --build .```) will generate a Debug
build.  To make a Release build, run the build step as such:

```cmake --build . --config Release```

The resulting ```executor.exe``` will be in the ```Release``` sub-folder
(or ```Debug```, for Debug builds).