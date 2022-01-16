# Discription

 * Fully documented SDK to be used for the 'Apex Legends' Source Engine.

## Instructions

To compile and use the SDK:

1. Download or clone the solution to `<gamedir>\platform\`.
	* The results should be `r5apex.exe\platform\r5apexsdk\detours.sln`.
2. Open `detours.sln` in Visual Studio and compile the project.
	* The launcher executable and worker dll will be copied over automatically to the root directory of the game.
	* If this does not happen, copy `r5apexsdk64.dll` and `r5reloaded.exe` to the game folder (where `r5apex.exe` resides).
4. Run `r5reloaded.exe` and follow instructions in the console to launch in desired mode.

## Launch options

There are 4 launch options, all of them use separate `startup_*.cfg` files to pass command line arguments to the game.
All preconfigured launch parameters are available from the release page of this repository.

 * Launch option 1 [DEBUG GAME] is for research and development purposes on the game.
 * Launch option 2 [RELEASE GAME] is for playing the game and creating servers.
 * Launch option 3 [DEBUG DEDICATED] is for research and development purposes on the dedicated server.
 * Launch option 4 [RELEASE DEDICATED] is for running and hosting dedicated servers.

The `startup_*.cfg` files are loaded from `<gamedir>\platform\cfg\startup_*.cfg`

 * Launch option 1 [RELEASE GAME] loads `startup_retail.cfg`
 * Launch option 2 [DEBUG GAME] loads `startup_debug.cfg`
 * Launch option 3 [RELEASE DEDICATED] loads `startup_dedi_retail.cfg`
 * Launch option 4 [DEBUG DEDICATED] loads `startup_dedi_debug.cfg`

You can add or remove launch parameters from these files. Be carefull, as some are necessary to run the SDK with the game.

## Launch parameters

When the `-wconsole` parameter is passed to the game, a external debug terminal will pop up
in which everything of the game gets logged to (this is passed by default on debug launch options).
This does not count for the dedicated server, the debug terminal is always present as this is a console application.

When the `-ansiclr` parameter is passed to the game or dedicated server, color logging will be enabled to enhance
output readability. It is recommended to use this unless your system does not support `VirtualTerminalLevel 1`.
This is passed by default.

## Note [Important]
This is not a cheat or hack. Do not attempt to use this on the latest version of the game!
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

## R5Net DISCLAIMER

When hosting to the Server Browser (R5Net) you will stream your IP address to the database 
which will be stored there until you stop hosting the server.
This is needed so you can connect to other player's servers.

There is a Checkbox in the server browser called `Server Visibility` that is by default set to `Offline`.
 * `Offline` allows you to play the game offline without any data being broadcasted to the R5Net master server.
 * `Hidden` allows you to play the game online, but your server will not be listed in the server borwser list
 people with a token can connect to your server, similar to the real private matches of Apex Legends.
 * `Public` allows you to play online, your server will be listed in the server browser list and everyone
 except for banned people can youn your server.

Alternative way is just to host the server without ticking the server browser checkbox and just give the people you want to connect the IP including the port.
