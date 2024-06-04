# Source SDK
* This repository houses the source code for the development package targeting the game **Apex Legends**.

## Building
R5sdk uses the CMake project generation and build tools. For more information, visit [CMake](https://cmake.org/).<br />
In order to compile the SDK, you will need to install Visual Studio 2017, 2019 or 2022 with:
* Desktop Development with C++ Package.
* Windows SDK 10.0.10240.0 or higher.
* C++ MFC build tools for x86 and x64.
* [Optional] C++ Clang/LLVM compiler.

Steps:
1. Download or clone the project to anywhere on your disk.
    1. Run `CreateSolution.bat` in the root folder, this will generate the files in `build_intermediate`.
    2. Move all the game files in the `game` folder so that the path `game/r5apex(_ds).exe` is valid.
2. Open `r5sdk.sln` in Visual Studio and compile the solution.
    1. All binaries and symbols are compiled to the `game` folder.
    2. Run `launcher.exe`, toggle and set the desired options and hit the `Launch Game` button.

## Debugging
The tools and libraries offered by the SDK could be debugged right after they are compiled.

Steps:
1. Set the target project as **Startup Project**.
    1. Select `Project -> Set as Startup Project`.
2. Configure the project's debugging settings.
    1. Debug settings are found in `Project -> Properties -> Configuration Properties -> Debugging`.
    2. Additional command line arguments could be set in the `Command Arguments` field.

## Launch Parameters
- The `-wconsole` parameter toggles the external console window to which output of the game is getting logged to.
- The `-ansicolor` parameter enables colored console output to enhance readability (NOTE: unsupported for some OS versions!).
- The `-nosmap` parameter instructs the SDK to always compute the RVA's of each function signature on launch (!! slow !!).
- The `-noworkerdll` parameter prevents the GameSDK DLL from initializing (workaround as the DLL is imported by the game executable).

Launch parameters can be added to the `startup_*.cfg` files,<br />
which are located in `<gamedir>\platform\cfg\startup_*.cfg`.

## Note [IMPORTANT]
This is not a cheat or hack; attempting to use the SDK on the live version of the game could result in a permanent account ban.<br />
The supported game versions are:

 * S3 `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`.

## Pylon [DISCLAIMER]
When you host game servers on the Server Browser (Pylon) you will stream your IP address to the database,
which will be stored there until you stop hosting the server; this is needed so other people can connect to your server.

There is a checkbox in the Server Browser called `Server Visibility` that defaults to `Offline`.
- `Offline`: No data is broadcasted to the Pylon master server; you are playing offline.
- `Hidden`: Your server will be broadcasted to the Pylon master server, but could only be joined using a private token.
- `Online`: Your server will be broadcasted to the Pylon master server, and could be joined from the public list.

Alternatively, you can host game servers without the use of our master server. You can grant people access to your game server
by sharing the IP address and port manually. The client can connect using the `connect` command. The usage of the `connect`
command is as follows: IPv4 `connect 127.0.0.1:37015`, IPv6 `connect [::1]:37015`. NOTE: the IP address and port were examples.
