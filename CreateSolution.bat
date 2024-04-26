@echo off
setlocal

set BUILDDIR=build_intermediate
set BINDIR=game

REM Check for Visual Studio versions in order
for %%V in (15 16 17) do (
    reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.%%V.0" >> nul 2>&1
    if NOT ERRORLEVEL 1 (
        if "%%V"=="15" (
            set "CMAKE_GENERATOR=Visual Studio 15 2017"
        ) else if "%%V"=="16" (
            set "CMAKE_GENERATOR=Visual Studio 16 2019"
        ) else if "%%V"=="17" (
            set "CMAKE_GENERATOR=Visual Studio 17 2022"
        )
        echo Using Visual Studio %%V as generator.
        goto :build
    )
)

echo Could not find a supported version of Visual Studio; exiting...
exit /b 1

:build
if not exist "%BUILDDIR%" (
  mkdir "%BUILDDIR%"
)
if not exist "%BINDIR%" (
  mkdir "%BINDIR%"
)

cd "%BUILDDIR%"
cmake .. -G"%CMAKE_GENERATOR%" -A"x64" -DOPTION_CERTAIN=ON
cd ..

echo Finished generating solution files.
