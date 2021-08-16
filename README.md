## Note [Important]
This is not a cheat or hack. Do not attempt to use this on the latest version of the game

# Apex

APEX Legends / R5 Server Engine research

 * Detours-based Visual Studio project for hooking the game engine

## R5Dev

Instructions are kinda outdated. Will be updated soon.

To use the vs project / engine hooking, here are some basic instructions:

1. Build the solution
	* or get the binaries from the releases page of this repo
2. Copy `r5detours.dll` and `launcher.exe` to the apex game folder
3. Copy the unpacked version of `r5apex.exe` over the original in your game folder
4. Run `launcher.exe`

The game will pop a debug terminal, that will forward your commands directly to the in-game source console backend. SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.

## R5Net DISCLAIMER

When hosting to the Server Browser (R5Net) you will stream your IP to that database which will be stored there till you stop hosting the server.

This is NEEDED so you can even connect to other players servers.

There is a Checkbox that is by default disabled that allows you to stream to the server browser.

If you tick that and don't assign a password your server will be visible in the server browser.

But if you assign a password you will get a token as a response and a password and it won't be visible in the server browser.

People can join with that over the "Private Servers" button.

If you do that your IP will still be stored on the database till you stop hosting the server to ensure people can connect.

Alternative way is just to host the server without ticking the server browser checkbox and just give the people you want to connect the IP including the port.

TL;DR If you tick the server browser box in "Host Server" your IP will be stored till you close the server.



## [RU] Заметка [Важно]
Это не чит и не взлом игры. Не пытайтесь использовать это на последней версии игры.

# Apex

APEX Legends / Исследование серверного движка R5.

 * Проект для Visual Studio на основе обходных путей для хука игрового движка.

## R5Dev

Инструкция для версий до v1.5.1-beta (ВКЛЮЧИТЕЛЬНО):

1. Скомпилируйте проект или скачайте готовую версию с страницы релизов.
2. Скопируйте `r5detours.dll` и `launcher.exe` в папку с Apex Legends (Билд Сезона 3).
3. Скопируйте распакованую версию `r5apex.exe` поверх оригинальной в папке билда 3 - его сезона.
4. Запустите `launcher.exe`

Инструкция для версий от v1.5.2-beta (ВКЛЮЧИТЕЛЬНО):

1. Скомпилируйте проект или скачайте готовую версию с страницы релизов.
2. Распакуйте файлы архива в корневую папку билда 3 - его сезона. (Где находиться r5apex.exe.)
3. Скачайте https://github.com/Mauler125/scripts_r5.
4. В папку билда 3 - его сезона по пути platform/scripts (если нет папки scripts, то её нужно создать.).
5. Затем запускаем Run R5 Reloaded.bat, далее выбераем нужный билд игры:
 a) Dev Build - для соло ознакомления с игрой. НЕ ЗАПУСКАТЬ ДЛЯ СЕТЕВОЙ ИГРЫ Т.К. СТОРОНИЕ ИГРОКИ МОГУ ЧТО-ЛИБО СДЕЛАТЬ С ВАШИМИ ФАЙЛАМИ.
 б) Release Build - для совместной игры. Вроде безопасен.

В ИГРЕ НУЖНО НАЖАТЬ:
 
F10 - для браузера серверов.

INSERT - для консоли.

## R5Net ДИСКЛЕЙМЕР

Когда вы хостите игру через Браузер Серверов (R5Net), ваш IP будет отображаться и храниться в Базе Данных, пока вы не прекратите держать сервер.

IP нужен для Бота Discord и для отображения вашего матча в поиске.

Существует флажок, который по умолчанию отключен, что позволяет видеть игрокам ваш сервер в браузере серверов.

Если вы поставите галочку и не назначите пароль, ваш сервер будет виден в браузере серверов.

Но если вы назначите пароль, вы получите токен и пароль, и он не будет виден в браузере сервера.

Люди могут присоединиться к этому через кнопку "Private Servers".

Если вы сделаете это, ваш IP-адрес все равно будет храниться в базе данных до тех пор, пока вы не прекратите размещать сервер, чтобы люди могли подключиться.

Альтернативный способ - разместить сервер, не ставя галочку в браузере сервера, и просто дать людям, которых вы хотите подключить, IP-адрес, включая порт.

TL;DR Если вы поставите галочку в поле браузера сервера в разделе "Host Server", ваш IP-адрес будет сохранен до тех пор, пока вы не закроете сервер.

P.S:
### Минимальные требования для запуска:

ОС: 64-разрядная версия Windows 7
Процессор (AMD): AMD FX 4350 или аналогичный
Процессор (Intel): Intel Core i3 6300 или аналогичный
ОЗУ: 6 ГБ DDR3, 1333 МГц
Видеокарта (AMD): AMD Radeon™ HD 7730
Видеокарта (NVIDIA): NVIDIA GeForce® GT 640
DirectX: совместимая с DirectX 11 видеокарта или ее эквивалент
Сетевое соединение: подключение к Интернету со скоростью 512 кбит/с или быстрее
Жесткий диск: 40 ГБ

В основном нагрузка идёт на процессор.

### Server Ports for server hosting.

| PORT    | default     |
|---------|-------------|
| UDP     | 37015-37020 |
