@echo off
if "%1" == "clear" (
    rmdir /s/q .\build
)
if not exist build mkdir build
cd ./build && ^
cmake .. ^
    -G "MinGW Makefiles" ^
    -DBUILD_EXAMPLES=OFF ^
    -DBUILD_TESTS=OFF ^
    -DBUILD_RELEASE=ON ^
    -DENABLE_OPENSSL=ON ^
    && ^
make && ^
make install