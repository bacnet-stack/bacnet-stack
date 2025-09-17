#!/bin/bash

# Install script for BACnet GTK Discovery
# This script helps install the necessary dependencies and build the application

set -e

echo "BACnet GTK Discovery - Installation Script"
echo "========================================="

# Function to detect the Linux distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo $ID
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    else
        echo "unknown"
    fi
}

# Function to install GTK dependencies
install_gtk_deps() {
    local distro=$(detect_distro)

    echo "Detected distribution: $distro"
    echo "Installing GTK+ 3.0 development libraries..."

    case $distro in
        ubuntu|debian|linuxmint)
            sudo apt-get update
            sudo apt-get install -y libgtk-3-dev pkg-config build-essential
            ;;
        fedora)
            sudo dnf install -y gtk3-devel pkgconfig gcc make
            ;;
        rhel|centos)
            sudo yum install -y gtk3-devel pkgconfig gcc make
            ;;
        opensuse|suse)
            sudo zypper install -y gtk3-devel pkg-config gcc make
            ;;
        arch|manjaro)
            sudo pacman -S --needed gtk3 pkgconfig gcc make
            ;;
        *)
            echo "Unknown distribution. Please install GTK+ 3.0 development libraries manually."
            echo "Required packages: gtk3-devel (or libgtk-3-dev), pkgconfig, gcc, make"
            exit 1
            ;;
    esac
}

# Function to check if GTK is available
check_gtk() {
    if pkg-config --exists gtk+-3.0; then
        echo "GTK+ 3.0 development libraries are available."
        echo "Version: $(pkg-config --modversion gtk+-3.0)"
        return 0
    else
        echo "GTK+ 3.0 development libraries are not available."
        return 1
    fi
}

# Function to build the GTK application
build_gtk_app() {
    echo "Building BACnet GTK Discovery..."

    # Default to BACnet/IP
    DATALINK=${1:-bip}

    make -C ../../ clean || true
    make -C ../../ gtk-discover BACDL=$DATALINK

    if [ -f "./bacdiscover-gtk" ]; then
        echo "BACnet GTK Discovery built successfully!"
        echo "Run with: ./bacdiscover-gtk"
    else
        echo "Build failed. Please check the error messages above."
        exit 1
    fi
}

# Main installation process
main() {
    echo "Checking for GTK+ 3.0 development libraries..."

    if ! check_gtk; then
        echo "GTK+ 3.0 development libraries not found."
        read -p "Do you want to install them automatically? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_gtk_deps
            if ! check_gtk; then
                echo "Failed to install GTK+ 3.0 development libraries."
                exit 1
            fi
        else
            echo "Please install GTK+ 3.0 development libraries manually and run this script again."
            exit 1
        fi
    fi

    # Ask user for datalink type
    echo ""
    echo "Choose BACnet datalink type:"
    echo "1) BACnet/IP (default)"
    echo "2) MS/TP (Master-Slave/Token-Passing)"
    echo "3) Ethernet"
    echo ""
    read -p "Enter choice (1-3) [default: 1]: " choice

    case $choice in
        2)
            DATALINK="mstp"
            ;;
        3)
            DATALINK="ethernet"
            ;;
        *)
            DATALINK="bip"
            ;;
    esac

    echo "Building with datalink: $DATALINK"
    build_gtk_app $DATALINK

    echo ""
    echo "Installation completed successfully!"
    echo ""
    echo "To run the application:"
    echo "  ./bacdiscover-gtk"
    echo ""
    echo "To install system-wide:"
    echo "  sudo make install"
}

# Run main function
main "$@"
