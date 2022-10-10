# Description

 * Development package to be used for the 'Apex Legends' Source Engine.

## Instructions

To compile and use the SDK:

1. Download or clone the solution to `<gamedir>\platform\`.
	* The results should be `r5apex.exe\platform\sdk\r5sdk.sln`.
2. Open `r5sdk.sln` in Visual Studio and compile the solution.
	* The launcher executable and worker dll will be copied over automatically to the root directory of the game.
	* If this does not happen, copy `gamesdk.dll` and `launcher.exe` to the game folder (where `r5apex.exe` resides).
4. Run `launcher.exe`, toggle the desired options and hit the `Launch Game` button.

## Launch parameters

When the `-wconsole` parameter is passed to the game, an external debug terminal will pop up
in which everything of the game gets logged to (this is passed by default on debug launch options).
This does not count for the dedicated server, the debug terminal is always present as this is a console application.

When the `-ansiclr` parameter is passed to the game or dedicated server, color logging will be enabled to enhance
output readability. It is recommended to use this unless your system does not support `VirtualTerminalLevel 1`,
in which a SDK warning message will follow. This parameter is passed by default.

Additional launch parameters can be added to the `startup_*.cfg` files,
which are located in `<gamedir>\platform\cfg\startup_*.cfg`.

## Note [Important]
This is not a cheat or hack. Do not attempt to use the SDK on the live version of the game!
The only versions currently supported are `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM` and anything below.

The following builds are tested and guaranteed to work:

 * S0 `R5pc_r5launch_J1557_CL387233_2019_01_28_07_43_PM`
 * S0 `R5pc_r5launch_J1595_CL392411_2019_02_15_11_25_AM`
 * S0 `R5pc_r5launch_J1624A_CL394493_2019_02_24_09_29_PM`

 * S1 `R5pc_r5launch_N52A_CL399039_2019_03_12_03_21_PM`
 * S1 `R5pc_r5launch_N29_CL406647_2019_04_09_06_28_PM`
 * S1 `R5pc_r5live_N10_CL423814_2019_06_18_07_11_PM`

 * S2 `R5pc_r5launch_N191_CL429743_2019_07_11_01_04_PM`
 * S2 `R5pc_r5launch_N428_CL436418_2019_08_07_09_35_PM`
 * S2 `R5pc_r5launch_N676_CL443034_2019_09_06_02_23_PM`

 * S3 `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`

## Pylon DISCLAIMER

When you host game servers on the Server Browser (Pylon) you will stream your IP address to the database 
which will be stored there until you stop hosting the server.
This is needed so other people can connect to your server.

There is a Checkbox in the server browser called `Server Visibility` that is by default set to `Offline`.
 * `Offline` allows you to play the game offline without any data being broadcasted to the Pylon master server.
 * `Hidden` allows you to play the game online, but your server will not be listed in the server borwser list
 people with a token can connect to your server, similar to the real private matches of Apex Legends.
 * `Public` allows you to play online, your server will be listed in the server browser list and everyone
 except for banned people can join your server.

Alternatively, you can host game servers without the use of our master server, and grant people access to your game server
by sharing the IP address and port manually. The client can connect using the `connect` command. The usage of the `connect`
command is as follows: IPv4 `connect 127.0.0.1:37015`, IPv6 `connect [::1]:37015`, NOTE: the IP and port where examples.
