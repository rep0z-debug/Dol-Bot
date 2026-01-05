# Frequently Asked Questions (FAQ)

## General

### What is Dol Bot?
Dol Bot is a standalone C++ application for Minecraft stronghold triangulation. It processes F3+C clipboard data to calculate the most likely stronghold location.

### How does it compare to Ninjabrain Bot?
Dol Bot is built with C++ and Qt for maximum performance and a native look. It features a modern, dark-themed UI and includes advanced statistical models for high accuracy.

### Is it allowed in speedruns?
External stronghold calculators are generally allowed in Minecraft Speedrunning. However, this specific calculator has not been approved by the speedrunning community yet. Use at your own risk.

## Troubleshooting

### "High angle error - check your throws" warning?
This means one or more of your throws does not align well with the predicted location.
*   **Solution**: Try removing the throw that looks "off" or simply reset and try again with more careful alignment.
*   **Tip**: Ensure you are not moving your mouse while pressing F3+C.

### The overlay is not showing up?
*   Check the **Settings -> General** tab and ensure "Enable overlay mode" is checked.
*   Ensure the game is in "Windowed" or "Borderless Windowed" mode. Fullscreen exclusive mode might hide the overlay on some systems.
*   Check **Settings -> Window** and try toggling "Always on top".

### Hotkeys are not working?
*   Ensure "Enable global hotkeys" is checked in the **Settings -> Hotkeys** tab.
*   Some anti-virus or other software might intercept global hotkeys. Try running Dol Bot as Administrator.

### My coordinates are way off!
*   Check your **Minecraft Version** in Settings. Different versions generate strongholds differently.
*   Make sure you are not using "Fake Coordinates" by mistake (Settings -> Privacy).

## Advanced

### What is "Divine"?
Divine is a technique that uses fossil generated data (specifically bone block positions) to determine the stronghold ring you are in. This requires F3+I usage on a fossil.

### What is "Boat Mode"?
When you measure from a boat, your angle is snapped to a specific grid, allowing for much higher precision. Dol Bot can take advantage of this to give you 100% certainty with fewer throws.
