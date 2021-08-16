# **Installation**

## Things you will need.
1. Apex Build: `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`
2. detours_r5 from: [Mauler125/detours_r5](https://github.com/Mauler125/detours_r5)
3. scripts_r5 from: [Mauler125/scripts_r5](https://github.com/Mauler125/scripts_r5)
4. The Origin launcher installed and signed into an account with Apex Legends in the library.<br/> See: [What are the ban risks?](../faq/faq#what-are-the-ban-risks)

## Before proceding please...
- Read the [FAQ](../faq/faq)
- Run the current version of Apex Legends. 

## Installing
### 1. Create Directory
Create a Directory for your files. It should be in a place with at least 45GB free. You can now move your apex build to this folder. Make sure to keep an unmodified backup just in case.

### 3. Build files
Next you should obtain the r5_detours binaries you can do that through the linked repo or by building it yourself. See: [Building the binaries yourself.](#building-the-binaries-yourself) Once you have obtained `r5detours.dll` `dedicated.dll` and `launcher.exe` copy them to the root of your install folder. Your install directory should look as follows. Some files have been ommited for brevity. See [Full Directory Tree](../installation/tree) if you are confused.
```
├───audio
├───paks
├───platform
├───stbsp
├───vpk
├───r5apex.exe
├───launcher.exe <-- 
├───dedicated.dll <-- 
├───r5detours.dll <-- 
└───... 
```
### 4. Copy files
Now you can move on to copying the scripts. The contents of scripts_r5 needs to go into the scripts folder which itself is in the platform folder. If you do not have a scripts folder you should create it. 

```
platform
|
|   imgui.ini
|   playlists_r5_patch.txt
|   
+---cfg
|   |
|   ...
|           
+---log
|   |
|   ...
|
+---maps
|   |
|   ...
|           
+---scripts                                 <--
|   |   .gitattributes                      <--
|   |   enginevguilayout.res                <--
|   |   entitlements.rson                   <--
|   |   hudanimations.txt                   <--
|   |   hud_textures.txt                    <--
|   |   kb_act.lst                          <--
|   |   propdata.txt                        <--
|   |   status_effect_types.txt             <--
|   |   surfaceproperties.rson              <--
|   |   surfaceproperties_manifest.txt      <--
|   |   vgui_screens.txt                    <--
|       10 Folders were ommited...          
|               
+---shaders
|   |
|   ...
|           
\---support
    |
    ...
```

### 5. Additional Maps

At this point you have a working install, if you would like to install additional maps you should do that now. Simply follow the directions of the readme inside the map zip.

## Usage



## Building the binaries yourself

TO-DO

```
Instructions are kinda outdated. Will be updated soon.

To use the vs project / engine hooking, here are some basic instructions:

1. Build the solution
	* or get the binaries from the releases page of this repo
2. Copy `r5detours.dll` and `launcher.exe` to the apex game folder
3. Copy the unpacked version of `r5apex.exe` over the original in your game folder
4. Run `launcher.exe`

The game will pop a debug terminal, that will forward your commands directly to the in-game source console backend. SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.
```