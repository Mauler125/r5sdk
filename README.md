# Source SDK
* This repository houses the source code for the development package targeting the game **Apex Legends**.

## Building
In order to compile the SDK, you will need to install Visual Studio 2017, 2019 or 2022 with:
* Desktop Development with C++ Package.
* Windows SDK 10.0.10240.0 or higher.
* C++ MFC build tools for x86 and x64.

Steps:
1. Download or clone the solution to anywhere on your disk.
    1. In the folder `r5sdk.sln` resides, create a new folder called `game`.
    2. Move all the game files in the `game` folder so that the path `game/r5apex(_ds).exe` is valid.
2. Open `r5sdk.sln` in Visual Studio and compile the solution.
    1. Depending on your version of Visual Studio, you might need to re-target the solution.
    2. All binaries and symbols are compiled in the `game` folder.
    3. Run `launcher.exe`, toggle and set the desired options and hit the `Launch Game` button.

## Debugging
The tools and libraries offered by the SDK could be debugged right after they are compiled.

Steps:
1. Set the target project as **Startup Project**.
    1. `Right Click project --> Set as Startup Project`.
2. Configure the project's debugging settings.
    1. Debug settings are found in `Right Click Project --> Properties --> Configuration Properties --> Debugging`.
    2. The `Working Directory` field should be set to `$(SolutionDir)\game\`.
    3. The `Command` field should be set to the target executable (`r5apex(_ds).exe` for example).
    4. Additional command line arguments could be set in the `Command Arguments` field.

## Launch Parameters
- The `-wconsole` parameter toggles the external console window to which output of the game is getting logged to.
- The `-ansiclr` parameter enables colored console output to enhance readability (NOTE: unsupported for some OS versions!).
- The `-nosmap` parameter instructs the SDK to always compute the RVA's of each function signature on launch (!! slow !!).
- The `-noworkerdll` parameter prevents the GameSDK dll from initializing (workaround as the DLL is imported by the game executable).

Launch parameters can be added to the `startup_*.cfg` files,
which are located in `<gamedir>\platform\cfg\startup_*.cfg`.

## Note [Important]
This is not a cheat or hack, do not attempt to use the SDK on the live version of the game!
The following builds are tested and guaranteed to work:

 * S0 `R5pc_r5launch_J1557_CL387233_2019_01_28_07_43_PM`.
 * S0 `R5pc_r5launch_J1624A_CL394493_2019_02_24_09_29_PM`.
 * S1 `R5pc_r5launch_N52A_CL399039_2019_03_12_03_21_PM`.
 * S2 `R5pc_r5launch_N428_CL436418_2019_08_07_09_35_PM`.
 * S3 `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`.

## Pylon DISCLAIMER

When you host game servers on the Server Browser (Pylon) you will stream your IP address to the database,
which will be stored there until you stop hosting the server; this is needed so other people can connect to your server.

There is a checkbox in the Server Browser called `Server Visibility` that defaults to `Offline`.
- `Offline`: No data is broadcasted to the Pylon master server; you are playing offline.
- `Hidden`: Your server will be broadcasted to the Pylon master server, but could only be joined using the private token.
- `Online`: Your server will be broadcasted to the Pylon master server, and could be joined from the public list.

Alternatively, you can host game servers without the use of our master server, and grant people access to your game server
by sharing the IP address and port manually. The client can connect using the `connect` command. The usage of the `connect`
command is as follows: IPv4 `connect 127.0.0.1:37015`, IPv6 `connect [::1]:37015`. NOTE: the IP address and port were examples.
