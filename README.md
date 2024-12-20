# MapExpansionPlugin
This plugin is aimed at adding features for map makers by utilizing the power of bakkes mod via Kismet

## Features
- Running bakkes mod console commands
- Logging information to the console
- Running custom commands implemented in the plugin
- Ability to save/load kismet data to and from a file for cross load data storage
- Automatically installs Netcode Manager Plugin

## How It Works
This plugin funtions by reading the contents of specifically named string kismet variables. Those variables are: `bmcommand bmlog mepcommand`. Each one serves a different purpose as described below. To make use of them, set the value the string that you'd like to use. For example to use the ballontop command from bakkes mod, set the `bmcommand` string to: `ballontop`. Each physics tick, the plugin will read the values of each of the specified named kismet strings and will try to run their commands. After it has processed the command, the plugin will set the value of the string to an empty string. 

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
- `input stop` prevents the users inputs from effecting the car
- `input begin` allows users inputs to effect the car
- `keylisten {Key List} RemoteEventName` registers a specified remote event to be triggered when the keys provided in the key list are pressed. `{Key List}` is expected to be a comma separated list of UE3 recognized keys with no spaces. You can find the list [here](https://docs.unrealengine.com/udk/Three/KeyBinds.html)
- `savedata {Kismet Var List} FileName` saves the contents of the kismet vars in the kismet var list to the specified filename. The list is expected to be a comma separated list of kismet vars that are defined on the main sequence. Supported var types are: Bool, Float, Int, String, & Vector. The names cannot have a space in them. The FileName is the name of the file that you are saving too without a file extension. Make this unique to your map to not conflict with others. Once the data has been saved to a file, a remote event named `MEPDataSaved` will be triggered.
- `loaddata FileName` loads in data from a specified FileName. FileName is the name of file that your map has saved too without a file extension. It will take the saved kismet vars inside the file and set the value of them in the map. Once all variables are set, a remote event named `MEPDataLoaded` will be triggered.
- `remoteevent {Remote Event Name}` will trigger a remote event with the same name
- `changescore {blue/orange} {add/sub} {amount}` alters the scoreboard for the specified team by the amount given. For amount, only give non-negative integers. Using the sub operation will subtract the amount from the score while add will increment the score.
- `gamestate end` will end the match
