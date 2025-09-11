# MapExpansionPlugin
This plugin is aimed at adding features for map makers by utilizing the power of bakkes mod via Kismet

## Features
- Running bakkes mod console commands
- Logging information to the console
- Running custom commands implemented in the plugin
- Ability to save/load kismet data to and from a file for cross load data storage
- Automatically installs Netcode Manager Plugin
- Connecting to the Rocket League Analytics site for data tracking

## How It Works
This plugin funtions by reading the contents of specifically named string kismet variables. Those variables are: `bmcommand bmlog mepcommand`. These all need to be defined on the main sequence. Each one serves a different purpose as described below. To make use of them, set the value the string that you'd like to use. For example to use the ballontop command from bakkes mod, set the `bmcommand` string to: `ballontop`. Each physics tick, the plugin will read the values of each of the specified named kismet strings and will try to run their commands. After it has processed the command, the plugin will set the value of the string to an empty string. 

Some of the commands need to send data back to the map and do so by triggering a remote event named `MEPOutputEvent` and place the data in a string variable named `mepoutput` (also defined on the main sequence). It's recommended to use flash to process the data, and this value will be overwritten the next time the plugin needs to send data to the map.

## Building MEP
MEP is fairly simple to build, you'll just need a couple requirements.
- [Visual Studio](https://visualstudio.microsoft.com/)
- [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell) 

Open the project in visual studio, make sure you are in release mode, and click build. If you have vcpkg setup, it should install nlohmann and link properly.

## Bakkes Mod Console Commands
To run console commands, make a string var called `bmcommand` and set the value to the command that you would like to run
You can find the available console commands that can be run [here](https://bakkesmod.fandom.com/wiki/Category:Console_commands)

## Console Logging
To log information to the console, make a string var called `bmlog` and set the value to the information to be logged

## Kismet Check For Plugin
To be notified when the plugin is loaded, make a remote event named `MEPLoaded`. When RL loads the map, the plugin will trigger this remote event. Additionally if the plugin is loaded/installed while the map is already opened, it will trigger this remote event then.

## Custom Commands
Custom commands are self implemented and requested features for map makers to utilize when the bakkes mod commands just won't cut it.
To run custom commands, make a string var called `mepcommand` and set the value to one of the custom commands listed below. You can chain multiple commands together by separating them with a semicolon.

### Custom Command List
- `input {stop/begin}` will either block input from all cars or remove the block
- `keylisten {Key List} RemoteEventName` registers a specified remote event to be triggered when the keys provided in the key list are pressed. `{Key List}` is expected to be a comma separated list of UE3 recognized keys with no spaces. You can find the list [here](https://docs.unrealengine.com/udk/Three/KeyBinds.html)
- `savedata {Kismet Var List} FileName`(DEPRECATED use savedataprefix instead) saves the contents of the kismet vars in the kismet var list to the specified filename. The list is expected to be a comma separated list of kismet vars that are defined on the main sequence. Supported var types are: Bool, Float, Int, String, & Vector. The names cannot have a space in them. The FileName is the name of the file that you are saving too without a file extension. Make this unique to your map to not conflict with others. Once the data has been saved to a file, a remote event named `MEPDataSaved` will be triggered.
- `savedataprefix {Prefix List} FileName` saves all variables in the map that start with one of the prefixes in the `{Prefix List}` to the specified filename. The list is expected to be a comma seperated list of prefixes that the plugin will use to search for all variables that start with one of the prefixes. They must all be defined on the main sequence. Supported var types are: Bool, Float, Int, String, & Vector. The names cannot have a space in them. The FileName is the name of the file that you are saving too without a file extension. Make this unique to your map to not conflict with others. Once the data has been saved to a file, a remote event named `MEPDataSaved` will be triggered.
- `loaddata FileName` loads in data from a specified FileName. FileName is the name of file that your map has saved too without a file extension. It will take the saved kismet vars inside the file and set the value of them in the map. Once all variables are set, a remote event named `MEPDataLoaded` will be triggered.
- `remoteevent {Remote Event Name}` will trigger a remote event with the same name
- `changescore {blue/orange} {add/sub} {amount}` alters the scoreboard for the specified team by the amount given. For amount, only give non-negative integers. Using the sub operation will subtract the amount from the score while add will increment the score.
- `changestats {assists/goals/saves/score/shots} {add/sub} {amount} {playerid}` is used to alter the stats on the scoreboard for a specific player. Choose a stat to interact with and whether to add or remove from the total. For amount, only give non-negative integers. The `{playerid}` value is the 3 digit in game playerid that is assigned to a player. This can be found on a player through the object property chain of `Player > PRI > PlayerID`. All players will need MEP in order for it to show on the scoreboard for them.
- `gamestate {end/kickoff}` will either end the match or create an immediate kickoff event
- `gametime {add/seb/set} {amount}` changes the remaining time in match by the amount given. This can be used to either add time, remove time, or set the remaining time to a static number. For amount, only give non-negative integers.
#### Speedrun.com Leaderboards
The `speedrun` command is used to pull data from the SRC API. A few specific endpoints have been implemented for easy querying and parsing inside a map, but there is an option to request any aribitrary endpoint supported by the API and parsing the resulting JSON data yourself. A full description of the SRC API can be found [here](https://github.com/speedruncomorg/api)
- `speedrun get {}`
#### Rocket League Analytics
A feature that deserves it's own section is the ability to track custom events from players with Rocket League Analytics. You can define whatever events you would like from level completion times, to number of deaths, so that you gain more insight on how your map is being played. Each session and event is associated to a specific player by either their Epic or Steam Ids. Players have the option to opt out from the service through the plugin settings. If they have it disabled, any attempt to call the `analytics` command will result in the `MEPOutputEvent` being triggered and `Analytics Refused` being set as the output. For details on how to get started with Rocket League Analytics visit [here](https://analytics.rocketleaguemapmaking.com/). The analytics site has a flash package that can be used to make interating with this command much easier.
- `analytics trackinit {Project Id} {API Key}` is used to initialize a session based on your project id and api key. The other options in the `analytics` command require an active session to use. On a successful session creation, the `MEPOutputEvent` is triggered with the output `Analytics session started`
- `analytics trackevent {Json Data}` is used to send data to the API. The json data at a minimum needs a property called `name` to correspond with a defined event to track. The plugin will insert the players information into these requests. After a period of 5 seconds of no calls to `analytics trackevent` a request will be made with all the events queued up. The `MEPOutputEvent` is triggered with the output `Analytics event tracked` after this.
- `analytics trackend` is used to end a session. This will immediately send out any outstanding events queued up and then end the session. Leaving a map without calling this will also cause this cleanup to happen. The `MEPOutputEvent` is triggered with the output `Analytics session stopped` after this.
