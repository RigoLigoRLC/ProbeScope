# ProbeScope

Real time variable acquisition, data plotting, RTT terminal, etc via debug probe (ST-Link, CMSIS DAP Link).

This project started as a side-project when I was participating in RoboMaster competition, and it gradually evolved into my undergraduate thesis. As a result, please expect poor code quality. 🙂

> [!WARNING]
> No issue and pull-requests are accepted for now. This project is open-source right now purely because I'm willing to share this work.

# Building

Make sure Rust toolchain is installed and `cargo` is in PATH.

CMake 3.19 is required.

Qt 6.8 is required (6.9 will fail the build)

Note that the project is in a tight messy development mode and nothing in the documentation should be considered accurate. ABSOLUTE NO WARRANTY GUARANTEED.

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
