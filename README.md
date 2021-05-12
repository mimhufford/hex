# Hex
A simple GUI hex viewer developed as a teaching aid.

## Build Instructions

### Dependencies
- [raylib](https://www.raylib.com/)

### Windows
1. Enter the [Visual Studio Developer Command Prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line)
2. Run `build.bat`

### MacOS & Unix
- Untested, but there's no reason it shouldn't work.

## Usage

### Launching from the command line
Add the executable to your path (or navigate to the directory containing the executable) and provide a file to view, for example:
```console
> hex img.png
```

### Launching from your file explorer
Drop a file onto the executable and it will be loaded in Hex.

### Using Hex
- `Up`, `Down`, `Left`, and `Right` move the cursor
- `Home`, `Pg Up`, `Pg Down`, and `End` move the view
- `Ctrl+Up` and `Ctrl+Down` move the view by one row
- `Ctrl+0`, `Ctrl+-`, and `Ctrl+=` control the font size