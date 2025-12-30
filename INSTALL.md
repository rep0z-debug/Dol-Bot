# Getting Started with Dol Bot

Dol Bot is a high-precision Minecraft stronghold triangulation tool designed for speedrunners and technical players. It helps you locate strongholds quickly and accurately using eye of ender throws.

## Installation

1.  **Download**: Get the latest release from the [Releases page](https://github.com/rep0z-debug/Dol-Bot/releases).
2.  **Extract**: Extract the zip file to a folder of your choice.
3.  **Run**: Double-click `DolBot.exe` to start the application.

## Basic Usage (Triangulation)

1.  **In-Game Setup**:
    *   Make sure you are in a Minecraft world.
    *   For best accuracy, set your **F3+I** (Copy Data) keybinding if you plan to use Divine/Fossil features.
    *   Ensure your `fov` and `sensitivity` are set comfortably, though Dol Bot relies on F3+C raw data.

2.  **Throwing Eyes**:
    *   Throw an Eye of Ender.
    *   Look at the eye and center your crosshair on it as best as you can.
    *   Press **F3+C** to copy your location and angle to the clipboard.
    *   **Dol Bot** will automatically detect the clipboard change and add the throw.

3.  **Triangulating**:
    *   Move perpendicular to the eye's direction (about 20-30 blocks away) and repeat the throw setup.
    *   Dol Bot will update the results instantly.
    *   Look at the **Certainty** percentage. Once it's high (above 90%), you can likely go to the coordinates shown.

4.  **Information Display**:
    *   **Coords**: The X and Z coordinates of the stronghold start chunk (4, 4 by default).
    *   **Nether Coords**: The equivalent coordinates in the Nether (Overworld / 8).
    *   **Distance**: Straight-line distance from your current player position.
    *   **Overlay**: Enable the Overlay in Settings to see a compact view on top of your game.

## Features

*   **Overlay Mode**: See coordinates and certainty without switching windows.
*   **Boat Mode**: High-precision mode for boat travel measurements (auto-detected or toggleable via hotkey).
*   **Divine**: Fossil-based triangulation support.
*   **Hotkeys**: Global hotkeys for resetting, undoing throws, and more.
*   **OBS Hiding**: Hides the bot from capture software if enabled in Settings.

## Hotkeys (Default)

All hotkeys are customizable in Settings -> Hotkeys.

*   `Ctrl+R`: Reset all throws
*   `Ctrl+Z`: Undo last throw
*   `Ctrl+Y`: Redo last undo
*   `Ctrl+L`: Lock/Unlock result
*   `Ctrl+B`: Toggle boat mode
*   `Ctrl+H`: Toggle Privacy Mode
*   `Ctrl+]`: Adjust last angle by +0.01°
*   `Ctrl+[`: Adjust last angle by -0.01°
*   `Ctrl+,`: Open settings

