# **Installation**

## Things you will need.
1. Apex Build: `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`
2. detours_r5 from: [Mauler125/detours_r5](https://github.com/Mauler125/detours_r5)
3. scripts_r5 from: [Mauler125/scripts_r5](https://github.com/Mauler125/scripts_r5)
4. The Origin launcher installed and signed into an account with Apex Legends in the library.<br/> See: [What are the ban risks?](../faq/faq#what-are-the-ban-risks)

## Before proceding please...
- Read the [FAQ](../faq/faq)
- Run the current version of Apex Legends at least once. 

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

At this point you have a working install, if you would like to install additional maps you should do that now. Simply follow the directions from the readme contained inside the map zip.

## Running and Usage

To run R5Reloaded simply execute the `launcher.exe` in the root of your install. If you have done everything correctly, you will be greeted with the EA splash screen and shortly after the games hould try and connect to EA's servers which will fail. From here you can press F10 and refresh the server browser to find a server to join or [create your own.](../servers/hosting)


## Building the binaries yourself

TO-DO