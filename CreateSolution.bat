@echo off
set CMAKE_GENERATOR=Visual Studio 16 2019
set BUILDDIR=build_intermediate

if not exist "%BUILDDIR%" (
  mkdir "%BUILDDIR%"
)

cd "%BUILDDIR%"
cmake .. -G"%CMAKE_GENERATOR%"
cd ..

echo Finished generating solution files.
