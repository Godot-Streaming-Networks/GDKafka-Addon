:: Setup Clang
set CC=clang
set CXX=clang++
set buildType=Release

:: Run this script to build the project
cmake -DCMAKE_BUILD_TYPE=%buildType% -B .sln -S . && cmake --build .sln --config %buildType%