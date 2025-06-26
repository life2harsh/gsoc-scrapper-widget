# GSoC Desktop Widget

A Windows desktop widget that displays the latest Google Summer of Code (GSoC) blog posts from the official Google Open Source Blog. The widget provides a clean, modern interface that sits on your desktop and automatically updates with new posts.

## Features

- **Desktop Integration**: Embeds directly into the Windows desktop wallpaper
- **Auto-refresh**: Updates every 30 minutes with new GSoC posts
- **System Tray**: Minimizes to system tray with context menu options
- **Drag & Drop**: Click and drag to reposition the widget
- **Hover Effects**: Interactive hover states for better user experience
- **DPI Awareness**: Scales properly on high-DPI displays
- **Auto-startup**: Optionally starts with Windows
- **Clickable Links**: Click on post titles to open them in your browser

## Screenshots
![image](https://github.com/user-attachments/assets/3b325e2b-2c2b-4f2d-8cc2-d3adf38834a8)


![image](https://github.com/user-attachments/assets/eb21ce9f-89eb-43cb-ac0d-2d15ffaff393)


![image](https://github.com/user-attachments/assets/755bbd7e-8d7e-41a7-a4f9-5dec6e9acb6a)

The widget displays:
- Header with "GSoC Updates" title
- List of recent blog posts with dates and titles
- Hover effects and clickable links

## Prerequisites

### Development Dependencies

- **Windows SDK** (Windows 10/11)
- **Visual Studio 2019/2022** with C++ development tools
- **vcpkg** (recommended for package management)

### Runtime Dependencies

- **libcurl** - HTTP client library
- **libxml2** - XML/HTML parsing library
- **Windows API libraries**:
  - `Shcore.lib` (Shell scaling)
  - `Shlwapi.lib` (Shell utilities)
  - `User32.lib` (Windows API)
  - `Gdi32.lib` (Graphics)
  - `Shell32.lib` (Shell operations)
  - `Advapi32.lib` (Registry operations)

## Installation

### Option 1: Using vcpkg (Recommended)

1. Install vcpkg if you haven't already:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
```

2. Install dependencies:
```bash
vcpkg install curl:x64-windows
vcpkg install libxml2:x64-windows
```

3. Clone this repository:
```bash
git clone https://github.com/life2harsh/gsoc-scrapper-widget.git
cd gsoc-widget
```

4. Build the project:
```bash
# Using Visual Studio
# Open the project in Visual Studio and build
# Make sure to integrate vcpkg: vcpkg integrate install

# Or using MSBuild
msbuild GSoCWidget.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Option 2: Manual Installation

1. Download and compile libcurl for Windows
2. Download and compile libxml2 for Windows
3. Update the include and library paths in your project settings
4. Compile the project

## Usage

### Running the Widget

1. **First Run**: Simply run the executable. The widget will appear on your desktop.

2. **System Tray**: The widget adds an icon to your system tray for easy access.

3. **Repositioning**: Click and drag the widget to move it around your desktop.

4. **Refresh**: Right-click the widget or system tray icon to manually refresh posts.

### System Tray Options

Right-click the system tray icon to access:
- **Show/Hide Widget**: Toggle widget visibility
- **Refresh Posts**: Manually update the post list
- **Exit**: Close the application

### Configuration

The widget saves its configuration in `widget.ini`:
- **Position**: Last window position
- **AutoStart**: Whether to start with Windows

## Building from Source

### Visual Studio Setup

1. Create a new C++ Console Application project
2. Copy the source code into your project
3. Configure project properties:
   - **C/C++ → General → Additional Include Directories**: Add paths to libcurl and libxml2 headers
   - **Linker → General → Additional Library Directories**: Add paths to libcurl and libxml2 libraries
   - **Linker → Input → Additional Dependencies**: Add required libraries:
     ```
     libcurl.lib
     libxml2.lib
     Shcore.lib
     Shlwapi.lib
     User32.lib
     Gdi32.lib
     Shell32.lib
     Advapi32.lib
     ```

### Compiler Flags

Ensure these preprocessor definitions are set:
- `CURL_STATICLIB` (if using static libcurl)
- `LIBXML_STATIC` (if using static libxml2)
- `UNICODE`
- `_UNICODE`

## Troubleshooting

### Common Issues

1. **Widget not appearing**: Check if it's hidden behind other windows or minimized to tray
2. **No posts loading**: Verify internet connection and firewall settings
3. **Blurry text**: The widget includes DPI awareness, but older Windows versions may need compatibility settings
4. **Startup issues**: Check Windows startup programs and registry permissions

### Debug Build

For debugging, uncomment the debug output lines in the source code:
```cpp
// Uncomment these lines for debugging
std::cerr << "curl_easy_perform failed: " << curl_easy_strerror(res) << std::endl;
std::cerr << "Failed to create XPath context" << std::endl;
```

## Customization

### Styling

The widget's appearance can be customized by modifying these values in the source:
- Colors: `RGB(18, 18, 18)` for background, `RGB(240, 240, 240)` for text
- Fonts: `"Segoe UI"` with various sizes
- Dimensions: Window size, padding, and spacing values

### Update Frequency

Change the timer interval (currently 30 minutes):
```cpp
SetTimer(hWnd, 1, 30 * 60 * 1000, nullptr); // 30 minutes in milliseconds
```

## License

This project is licensed under the GNU License - see the [LICENSE](LICENSE) file for details.

## Support

If you encounter any issues or have questions:
1. Check the [Issues](https://github.com/life2harsh/gsoc-scrapper-widget/issues) page
2. Create a new issue with detailed information about your problem
3. Include your Windows version, build configuration, and any error messages
