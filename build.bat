@echo off
setlocal
set CXX=C:\MinGW\bin\g++.exe
if not exist build mkdir build
%CXX% -std=c++14 -Wall -Wextra -Iinclude -Ithird_party\minitest src\util.cpp src\namespace.cpp src\cgroup.cpp src\rootfs.cpp src\runtime.cpp src\ipc.cpp src\daemon.cpp src\oci.cpp src\main.cpp -o build\minict.exe
if errorlevel 1 exit /b 1
%CXX% -std=c++14 -Wall -Wextra -Iinclude -Ithird_party\minitest src\util.cpp src\namespace.cpp src\cgroup.cpp src\rootfs.cpp src\runtime.cpp src\ipc.cpp src\daemon.cpp src\oci.cpp third_party\minitest\minitest.cpp tests\test_util.cpp tests\test_namespace.cpp tests\test_cgroup.cpp tests\test_rootfs.cpp tests\test_runtime.cpp tests\test_cli_parse.cpp tests\test_ipc.cpp tests\test_oci.cpp -o build\minict_tests.exe
if errorlevel 1 exit /b 1
set MINICT_SIM=1
build\minict_tests.exe
