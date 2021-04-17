# apex

APEX Legends / R5 Server Engine research

 * detours-based Visual Studio project for hooking the game engine

## r5dev

to use the vs project / engine hooking, here are some basic instructions

1. build the solution
	* or get the binaries from the releases page of this repo
2. copy `r5dev.dll` and `r5launcher.exe` to the apex game folder
3. copy the unpacked version of `r5apex.exe` over the original in your game folder
4. run `r5launcher.exe`

the game will pop a debug terminal, that will forward your commands directly to the in-game source console backend. SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.
