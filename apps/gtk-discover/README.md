# BACnet GTK Discovery Application

A GTK-based graphical application for discovering BACnet devices, objects, and properties on a network.

## Features

- **Device Discovery**: Browse and discover BACnet devices on the network
- **Object Discovery**: View all objects within a selected BACnet device
- **Property Inspection**: Examine properties of individual BACnet objects
- **Three-Pane Interface**:
  - Left pane: Device list with device ID, name, model, and address
  - Top-right pane: Object list for selected device
  - Bottom-right pane: Property list for selected object

## Prerequisites

### GTK+ 3.0 Development Libraries

The application requires GTK+ 3.0 development libraries to be installed:

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install libgtk-3-dev pkg-config
```

#### Fedora/RHEL/CentOS:
```bash
sudo yum install gtk3-devel pkgconfig
# or for newer versions:
sudo dnf install gtk3-devel pkgconfig
```

#### openSUSE:
```bash
sudo zypper install gtk3-devel pkg-config
```

#### Arch Linux:
```bash
sudo pacman -S gtk3 pkgconfig
```

### BACnet Stack Library

The BACnet stack library must be built first. From the project root:

```bash
make -C apps/lib all
```

## Building

1. Navigate to the gtk-discover directory:
   ```bash
   cd apps/gtk-discover
   ```

2. Build the application:
   ```bash
   make all BACDL=bip
   ```

   Or for MS/TP datalink:
   ```bash
   make all BACDL=mstp
   ```

3. For debug build:
   ```bash
   make debug BACDL=bip
   ```

## Running

Execute the built application:
```bash
./bacnet-gtk-discovery
```

## Usage

1. **Device View**: The left pane shows discovered BACnet devices.

2. **Object View**: Click on a device in the left pane to view its
   objects in the top-right pane. Objects show:
   - Object type (Analog Input, Binary Output, etc.)
   - Object instance number
   - Object name

3. **Property View**: Click on an object to view its properties
   in the bottom-right pane, including:
   - Property name
   - Property value

4. **Toolbar Functions**:
   - **Discover Devices**: network device discovery using Who-Is requests
   - **Refresh**: refreshing current device data

## Architecture

The application is structured with:

- **GTK Components**: ListStore models for each tree view
- **BACnet Integration**: Uses the BACnet Stack library
- **Three-Pane Layout**: Resizable paned windows for optimal viewing

## Troubleshooting

### Build Issues:

1. **GTK not found**: Ensure GTK+ 3.0 development packages are installed
2. **BACnet library missing**: Build the BACnet library first: `make -C apps/lib all`
3. **pkg-config issues**: Install pkg-config package for your distribution

### Runtime Issues:

1. **Application won't start**: Check that all required libraries are installed
2. **Segmentation faults**: Build with debug symbols: `make debug`

## Files

- `main.c` - Main application source code
- `Makefile` - Build configuration
- `README.md` - This documentation

## Future Enhancements

- Property editing capabilities
- Device configuration management
- Trending and logging
- Alarm and event handling
- Export/import functionality
- Multiple datalink support configuration
- Network diagnostics and monitoring
