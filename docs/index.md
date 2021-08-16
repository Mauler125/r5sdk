## Note **Important**
This is not a cheat or hack. Do not attempt to use this on the latest version of the game.

# Apex

APEX Legends / R5 Server Engine research

 * Detours-based Visual Studio project for hooking the game engine

## R5Dev

Go to [installaion](installation/install.md)

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
