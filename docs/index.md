# **Documentation**

## R5Reloaded

Detours-based Visual Studio project for hooking the game engine.

---
## Other Languages 🇺🇸🇺🇲 🇫🇷

[Languages](#languages)

---
## Console and Server Browser
To use the in-game server broswer, press `F10`!<br/>
You can also open a developer console window by pressing `~` or the `insert` key.

---
## Installation

Go to [Installation.](installation/install)

---
## Help! I Keep Crashing!

See: [FAQ #Are there bugs?](faq/faq#are-there-bugs)

---
## Hosting a server

See: [Hosting a server.](servers/hosting)

---
## Frequently Asked Questions

Go to [FAQ.](faq/faq)

---

## Important Notices
*  This is not a cheat or hack. Do not attempt to use this on the latest version of the game.
*   When using R5Net you will stream your IP to our database which will be stored untill you stop hosting your server. This is required to connect to others' servers.

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

## Additional Info
The in-game console will forward your commands directly to the in-game source console backend.<br/>
SQVM prints are also enabled, and the hooks will attempt to load game vscripts out of `platform\` relative from the game executable before falling back to the SearchPath's defined in GameInfo.txt or the in-memory VPK structures.

## Languages
 - [français](languages/fr)
