@echo off
setlocal enabledelayedexpansion

pushd %~dp0

call vcpkg\bootstrap-vcpkg.bat
call vcpkg\vcpkg.exe install

if NOT ["%errorlevel%"]==["0"] pause

popd
