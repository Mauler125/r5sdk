# **Installation**

## Ce dont vous avez besoin :
1. Un Build de la saison 3 d'Apex: `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`
2. Le fichier zip detours_r5 depuis : [Mauler125/detours_r5](https://github.com/Mauler125/detours_r5)
3. Le fichier zip scripts_r5 depuis : [Mauler125/scripts_r5](https://github.com/Mauler125/scripts_r5)
4. Le launcher Origin installé et connecté à un compte avec Apex Legends dans la bibliothèque de jeux.<br/> Voir: [Quels sont les risques de bannissement ?](../faq/faq#what-are-the-ban-risks)

## Avant de commencer...
- Lisez la [FAQ](../faq/faq)
- Lancez la version actuelle d'Apex Legends au moins une fois. 
- Rejoindre notre [Discord!](https://discord.com/invite/jqMkUdXrBr)

## Installation
### 1. Créer un dossier
Créez un dossier pour vos fichiers. Il doit être dans un endroit avec au moins 45Go disponibles. Vous pouvez maintenant déplacer votre build d'Apex dans ce dossier. Faites attention à bien garder le build non modifié juste au cas où.

### 3. Copier les fichiers
Ensuite vous devez obtenir le dossier de fichiers r5_detours. Vous pouvez faire ça dans la section "Releases" de la repo github ou le construire par vous même. Voir: [Faire les fichiers vous même](#building-the-binaries-yourself) Une fois que vous avez obtenu `r5detours.dll` `dedicated.dll` et `launcher.exe` copiez les fichiers à la racine de votre dossier d'installation. Votre dossier d'installation devrait ressembler à ça. Certains fichiers ont étés omis par briéveté. Voir [Structure complète du dossier](../installation/tree) si vous êtes perdu.
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
### 4. Copier les scripts
Vous pouvez maintenant commencer à copier les scripts. Le contenu du dossier scripts_r5 doit aller dans le dossier scripts qui est lui même dans le dossier plateform. Si vous n'avez pas de dossier scripts vous devez le créer vous-même. 

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

### 5. Cartes Additionnelles

Maintenant, vous avez une installation fonctionnelle, si vous souhaitez installer des cartes additionnelles vous devriez faire ça maintenant. Suivez simplement les instructions depuis le fichier readme qui est dans le fichier zip de la map.

## Mise en marche et utilisation

Pour lancer R5Reloaded cliquez simplement sur `launcher.exe` dans la racine de votre dossier d'installation. Si vous avez tout configuré correctement, vous serez accueilli avec le logo d'EA et l'écran de chargement de la saison 3 et rapidement après sur l'écran de connexion qui vous affichera une erreur (c'est normal). Depuis cet écran vous pouvez appuyer sur la touche F10 et rafraîchir le Navigateur de Serveurs pour trouver un serveur à rejoindre ou [Créer le vôtre.](../servers/hosting)


TO-DO