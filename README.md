# MapExpansionPlugin
This plugin is aimed at adding features for map makers to make use of by utilizing the power of bakkes mod via Kismet

## Features
- Running bakkes mod console commands
- Logging information to the console
- Running custom commands implemented in the plugin
- Automatically installs Netcode Manager Plugin

## Bakkes Mod Console Commands
To run console commands make a string var called `bmcommand` and set the value to the command you would like to run

You can find the available console commands that can be run [here](https://bakkesmod.fandom.com/wiki/Category:Console_commands)

## Console Logging
To log information to the console make a string var called `bmlog` and set the value to the information to be logged

## Custom Commands
To run custom commands make a string var called `customcommand` and set the value to one of the custom commands listed below

### Custom Command List
- `input stop` prevents the users inputs from effecting the car
- `input begin` allows users inputs to effect the car
