# ProbeScope

Real time variable acquisition, data plotting, RTT terminal, etc via debug probe (ST-Link, CMSIS DAP Link).

This project started as a side-project when I was participating in RoboMaster competition, and it gradually evolved into my undergraduate dissertation. As a result, please expect poor code quality. 🙂

# Building

Make sure Rust toolchain is installed and `cargo` is in PATH.

CMake 3.19 is required.

Qt 5.15 is required.

# Windows

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=D:/Path/To/Vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_PREFIX_PATH=D:/sdks/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5/ \
      -GNinja \
      ..
ninja
```
