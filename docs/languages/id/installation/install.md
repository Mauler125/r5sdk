# **Pemasangan**

## Hal-hal yang kamu butuhkan
1. Build Apex: `R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM`
2. detours_r5 dari: [Mauler125/detours_r5](https://github.com/Mauler125/detours_r5)
3. scripts_r5 dari: [Mauler125/scripts_r5](https://github.com/Mauler125/scripts_r5)
4. Launcher Origin yang terpasang dan sudak masuk ke dalam akun yang memiliki Apex Legends dalam librarynya.<br/> Lihat: [Apa resiko ban-nya?](../faq/faq#apa-resiko-ban-nya)

## Sebelum melanjutkan tolong...
- Baca [FAQ](../faq/faq)
- Jalankan versi terbaru Apex Legends setidaknya sekali
- Bergabung [Discord!](https://discord.com/invite/jqMkUdXrBr) kami

## Pemasangan
### 1. Buat Direktori
Buat sebuah direktori untuk berkas-berkas kamu. Ini seharusnya berada di tempat dengan setidaknya 45GB bebas. Kamu sekarang bisa memindahkan build Apex ke folder ini. Pastikan kamu memiliki cadangan yang tidak diubah untuk berjaga-jaga.

### 3. Salin Binari
Beikutnya kamu sebaiknya mengambil binari r5_detours kamu dapat melakukan itu melalui seksi releases dari repo yang tercantum atau dengan [membangunnya sendiri.](../installation/build) Setelah kamu mendapatkan `r5detours.dll` `dedicated.dll` dan `launcher.exe` salin mereka ke folder root install kamu. Direktori pemasangan kamu seharusnya terlihat seperti ini. Beberapa file telah dihilangkan untuk keringkasan. Lihatlah [Pohon Direkotri Lengkap](../installation/tree) jika kamu bingung.
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
### 4. Salin Skrip
Sekarang kita berpindah ke menyalin skrip. Isi dari scripts_r5 harus dipindahkan ke folder scripts yang berada di folder platform. Jika kamu tidak memiliki folder scripts sebaiknya kamu buat sendiri.

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

### 5. Map Tambahan

Sekarang kamu memiliki installasi yang bekerja, jika kamu ingin menambahkan map kamu harusnya lakukan sekarang. Ikuti arahan dari readme yang terdapat di zip peta.

## Menjalankan dan Penggunaan

Untuk menjalankan R5Reloaded jalankan `launcher.exe` di folder root instalasi. Ada juga file bat yang tersedia di bagian `releases` dari repo `detours_r5` untuk pindah dari versi retail dan versi pengembang.<br/> Jika kamu melakukan semuanya dengan benar, kamu akan disambut splash screen EA, Lalu gamenya akan mencoba untuk menyambung ke server milik EA yang lalu akan gagal. Dari sini kamu dapat menekan `F10` dan menyegarkan peramban server untuk menemukan server untuk dimasukki atau [buat server sendiri.](../servers/hosting)
