# MapExpansionPlugin
This plugin is aimed at adding features for map makers to make use of by utilizing the power of bakkes mod via Kismet

## Features
- Running bakkes mod console commands
- Logging information to the console
- Running custom commands implemented in the plugin
- Automatically installs Netcode Manager Plugin
- Kismet bool support for checking if plugin is enabled

## Bakkes Mod Console Commands
To run console commands, make a string var called `bmcommand` and set the value to the command that you would like to run
You can find the available console commands that can be run [here](https://bakkesmod.fandom.com/wiki/Category:Console_commands)

## Console Logging
To log information to the console, make a string var called `bmlog` and set the value to the information to be logged

## Kismet Check For Plugin
To check if the plugin is enabled, make a bool var called `mappluginenabled` and set it to false. When RL loads the map and the plugin is enabled, the plugin will set the var to true. Additionally if the plugin is loaded while the map is already opened, it will set the var to true.

## Custom Commands
Custom commands are self implemented and requested features for map makers to utilize when the bakkes mod commands just won't cut it.
To run custom commands, make a string var called `customcommand` and set the value to one of the custom commands listed below. You can chain multiple commands together by separating them with a semicolon.

### Custom Command List
- `input stop` prevents the users inputs from effecting the car
- `input begin` allows users inputs to effect the car
- `keylisten {Key List} RemoteEvent` registers a specified remote event to be triggered when the keys provided in the key list are pressed. `{Key List}` is expected to be a comma separated list of UE3 recognized keys with no spaces. You can find the list [here](https://docs.unrealengine.com/udk/Three/KeyBinds.html)
