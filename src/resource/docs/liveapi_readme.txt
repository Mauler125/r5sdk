# Quick Start

This API is built on top of WebSockets and Google's Protocol Buffer (protobuf) technology. All gameplay events and
requests supported by the LiveAPI are documented in the `events.proto` protobuf file that lives in this directory.
If you're not familiar with protobuf, visit https://developers.google.com/protocol-buffers to learn more.

Protobuf bindings can be easily generated for your language of choice after you obtain the Protobuf Compiler `protoc`.
For example, to generate bindings for Python, use:
`protoc.exe --proto_path=<current directory of this readme> --python_out=<your project output directory> events.proto`

The bindings can then be included into your app code. In order to consume the events, your app will have to open a
WebSocket server that the running game server can connect and send events to.

# Configuration

The game server must also be running with additional configurations to activate the LiveAPI and connect to the app.
The configuration file is located in `platform/cfg/liveapi.cfg` (relative from the game executable `r5apex_ds.exe`).

For example, if the app has a websocket server in port 7777 of the same machine the game server is running on, the
following settings can be applied in the aforementioned configuration file to connect the game server to the app:

liveapi_enabled           "1"
liveapi_websocket_enabled "1"
liveapi_servers           "ws://127.0.0.1:7777"

Upon launching with these parameters, the game server will connect to the WebSocket server at 127.0.0.1 in port 7777.
Gameplay events are sent to the WebSocket server app once the game server has started.

The game can also be configured to write events to a local JSON file on the disk. The event logs will be located in
`platform/liveapi/logs/<session_uuid>/match_<matchNum>.json` (relative from the game executable `r5apex_ds.exe`).

The following settings can be applied in the aforementioned configuration file to log events to a local JSON file:

liveapi_enabled           "1"
liveapi_print_enabled     "1"
liveapi_print_pretty      "1"

- You can run the WebSocket and local JSON logging simultaneously.

# Scripting

The Squirrel scripting API of the LiveAPI is quite simple, there are 5 main functions:

LiveAPI_IsValidToRun()
- Whether the LiveAPI system was initialized and is currently able to run, check on this before calling the functions
  `LiveAPI_WriteLogUsingDefinedFields()` and `LiveAPI_WriteLogUsingCustomFields()`to save unnecessary load on the game
  server.

LiveAPI_StartLogging()
- This is used to start the logging of LiveAPI events to a local JSON file, if a log session is already active by the
  time this function is called, the previous log session will be closed and a new one will be started. The new session
  will be written to a new json file.
- Needs ConVar `liveapi_enabled` and `liveapi_print_enabled` set to 1, else this function will not do anything.

LiveAPI_StopLogging()
- This is used to stop the logging of LiveAPI events to a local JSON file. Calling this will finish off the file and
  stop the logging of new events.

LiveAPI_WriteLogUsingDefinedFields()
- This function takes 3 parameters, an event type (must be one registered in the 'eLiveAPI_EventTypes' enum, see
  the file `src/game/server/liveapi/liveapi.cpp` for available event types, or alternatively, see the events.proto
  file, event names must start with a lower case !!!), an array of data, and an array of field indices. The order of
  the 2 arrays are important, as the entries in the data array map directly to the data in the field indices array; if
  event type is eLiveAPI_EventTypes.playerKilled, and if the first entry of the data array is "someString", and the
  first entry in the field indices array is '3', then "someString" will be set to the third field index of the
  PlayerKilled event message. Make sure to study the event messages, and their respective field numbers in the
  `events.proto` file before assigning new events!

NOTE: Since there was a high demand for custom events, we added an event named `CustomEvent` which can be used to send
game data in any structure from the game's scripting API. This is useful if you wish to log your own data from the
game (e.g. a custom gamemode that has something the current protocol doesn't support). The following function utilizes
this special event message:

LiveAPI_WriteLogUsingCustomFields()
- This function takes 3 parameters, an event name (can be any string, the purpose of this is to identify your custom
  event!), an array of data, and an array of field names. You can entirely define the structure of the data yourself;
  if entry 6 in data array is "world!", and entry 6 in the field names array is "hello", then message variable "hello"
  becomes "world!". The data array can take any of the following types: bool|int|float|string|vector|array|table.
  NOTE: with tables, the key must be a string as this will be used to determine the custom field names, the values can
  be any of the aforementioned types.
- Tables and arrays cannot be nested into them selfs as this would cause infinite recursion, and the maximum nesting
  depth of a table or array is 64.

For more technical information about the scripting API of the system, see the file
`platform/scripts/vscripts/_live_api.gnut` (relative from the game executable `r5apex_ds.exe`).

# Technical Information

By popular demand, we tried our best to keep the protocol and documentation the same as that of the retail version of
the game to maintain compatibility with existing apps. As of the release of this build, the file `events.proto` is
synced with the retail build `R5pc_r5-201_J29_CL6350311_EX6402312_6403685_2024_03_22`.

# Contact & Feedback

If you have a question, suggestion, or encountered an issue in the system, you can send me (Kawe Mazidjatari) an email
at `amos@r5reloaded.com`.
