1. Download MSYS2 from https://www.msys2.org/

    https://github.com/msys2/msys2-installer/releases/download/2023-03-18/msys2-x86_64-20230318.exe

2. After installation open "MSYS2 MINGW64" from Menu Start

3. Install required packages with

    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-autotools git

4. Browse to MultiCraft directory, for example

    cd /c/Users/username/Documents/MultiCraft

5. Build inside build/windows directory with

    ./Start
    cmake --build . -j

6. Check libraries linked with multicraft.exe using command below. It should
   show only system libraries.

    objdump -x ../../bin/multicraft.exe | grep dll


--------------

The "make -j" eats a lot of memory (I was out of memory on laptop with 12GB RAM
and 4 cores CPU) and even "make -j`nproc`" said out of memory for me when
building libcurl. If it's a problem, then change NPROC to 1 or so.

Also don't install any other packages that are not for mingw because then some
libraries search headers in /usr/include rather than /mingw64/include and fail
to compile.
