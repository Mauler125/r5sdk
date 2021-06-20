## Note [Important]
This is not a cheat or hack. Do not attempt to use this on the latest version of the game

# Apex

APEX Legends / R5 Server Engine research

 * Detours-based Visual Studio project for hooking the game engine

## R5Dev

To use the vs project / engine hooking, here are some basic instructions:

1. Build the solution
	* or get the binaries from the releases page of this repo
2. Copy `r5detours.dll` and `launcher.exe` to the apex game folder
3. Copy the unpacked version of `r5apex.exe` over the original in your game folder
4. Run `launcher.exe`

The game will pop a debug terminal, that will forward your commands directly to the in-game source console backend. SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.
