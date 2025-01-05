# RandomItems

A Hitman 3 SDK mod that allows you to spawn a random item every 15 seconds (or whatever delay you want it to be!).

## Installation Instructions

1. Download the latest version of [ZHMModSDK](https://github.com/OrfeasZ/ZHMModSDK) and install it.
2. Download the latest version of `RandomItems` and copy it to the ZHMModSDK `mods` folder (e.g. `C:\Games\HITMAN 3\Retail\mods`).
3. Run the game and once in the main menu, press the `~` key (`^` on QWERTZ layouts) and enable `RandomItems` from the menu at the top of the screen (you may need to restart your game afterwards).
4. Enjoy!

## Experimental Configuration
There are some repository items that do not come with a `Title`-property. This means that, by default, they'll be excluded by this mod as these usually aren't spawnable items and result in longer periods of no items being spawned. If however you do want to include these, simply tick the checkbox and in case the mod is already running also hit the `Rebuild Item Pool`-button.

## Building

### 1. Clone this repository locally with all submodules.

You can either use `git clone --recurse-submodules` or run `git submodule update --init --recursive` after cloning.

### 2. Install Visual Studio (any edition).

Make sure you install the C++ and game development workloads.

### 3. Open the project in your IDE of choice.

See instructions for [Visual Studio](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-Visual-Studio-for-development) or [CLion](https://github.com/OrfeasZ/ZHMModSDK/wiki/Setting-up-CLion-for-development).
