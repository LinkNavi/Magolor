#!/bin/bash
# Magolor Standard Library Installation Script
# Usage: ./install-stdlib.sh [--user|--system|--local]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored messages
info() { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
info "Detected OS: $OS"

# Default installation mode
MODE="${1:---system}"

# Determine installation directory
case "$MODE" in
    --user)
        if [[ "$OS" == "windows" ]]; then
            INSTALL_DIR="$USERPROFILE/.magolor/stdlib"
        else
            INSTALL_DIR="$HOME/.magolor/stdlib"
        fi
        NEEDS_SUDO=false
        ;;
    --system)
        if [[ "$OS" == "windows" ]]; then
            INSTALL_DIR="C:/Program Files/Magolor/stdlib"
        elif [[ "$OS" == "macos" ]]; then
            # Check if installed via Homebrew
            if command -v brew &> /dev/null && brew --prefix magolor &> /dev/null 2>&1; then
                INSTALL_DIR="$(brew --prefix)/share/magolor/stdlib"
                NEEDS_SUDO=false
            else
                INSTALL_DIR="/usr/local/share/magolor/stdlib"
                NEEDS_SUDO=true
            fi
        else
            INSTALL_DIR="/usr/local/share/magolor/stdlib"
            NEEDS_SUDO=true
        fi
        ;;
    --local)
        INSTALL_DIR="./stdlib"
        NEEDS_SUDO=false
        ;;
    *)
        error "Unknown mode: $MODE. Use --user, --system, or --local"
        ;;
esac

info "Installation directory: $INSTALL_DIR"

# Check if stdlib source exists
if [[ ! -d "stdlib" ]]; then
    error "stdlib/ directory not found. Run this script from the magolor project root."
fi

# Count stdlib files
STDLIB_FILES=$(find stdlib -name "*.mg" | wc -l)
info "Found $STDLIB_FILES stdlib modules"

if [[ $STDLIB_FILES -eq 0 ]]; then
    error "No .mg files found in stdlib/"
fi

# Verify critical modules exist
REQUIRED_MODULES=("io.mg" "string.mg" "array.mg")
for module in "${REQUIRED_MODULES[@]}"; do
    if [[ ! -f "stdlib/$module" ]]; then
        error "Required module missing: stdlib/$module"
    fi
done
success "All required modules found"

# Create installation directory
info "Creating installation directory..."
if [[ "$NEEDS_SUDO" == true ]]; then
    sudo mkdir -p "$INSTALL_DIR"
else
    mkdir -p "$INSTALL_DIR"
fi

# Copy stdlib files
info "Copying stdlib files..."
if [[ "$NEEDS_SUDO" == true ]]; then
    sudo cp -r stdlib/* "$INSTALL_DIR/"
    sudo chmod -R 755 "$INSTALL_DIR"
else
    cp -r stdlib/* "$INSTALL_DIR/"
    chmod -R 755 "$INSTALL_DIR"
fi

# Verify installation
info "Verifying installation..."
INSTALLED_FILES=$(find "$INSTALL_DIR" -name "*.mg" 2>/dev/null | wc -l)

if [[ $INSTALLED_FILES -eq $STDLIB_FILES ]]; then
    success "Successfully installed $INSTALLED_FILES stdlib modules to $INSTALL_DIR"
else
    error "Installation verification failed. Expected $STDLIB_FILES files, found $INSTALLED_FILES"
fi

# List installed modules
echo ""
info "Installed modules:"
for module in "$INSTALL_DIR"/*.mg; do
    if [[ -f "$module" ]]; then
        basename "$module" .mg | sed 's/^/  - Std./'
    fi
done

# Test compilation
echo ""
info "Testing stdlib availability..."

TEST_FILE=$(mktemp --suffix=.mg)
cat > "$TEST_FILE" << 'EOF'
using Std.IO;

fn main() {
    println("Stdlib test successful!");
}
EOF

if command -v magolor &> /dev/null; then
    if magolor build "$TEST_FILE" -o /tmp/stdlib_test 2>/dev/null; then
        if /tmp/stdlib_test 2>/dev/null; then
            success "Stdlib is working correctly!"
        else
            warning "Stdlib installed but test program failed to run"
        fi
        rm -f /tmp/stdlib_test
    else
        warning "Stdlib installed but test compilation failed"
        warning "The compiler may need to be rebuilt or PATH updated"
    fi
else
    warning "magolor command not found. Install the compiler to test stdlib."
fi

rm -f "$TEST_FILE"

# Post-installation instructions
echo ""
info "Installation complete!"
echo ""

case "$MODE" in
    --user)
        echo "User installation successful. The stdlib is available at:"
        echo "  $INSTALL_DIR"
        echo ""
        echo "No additional configuration needed."
        ;;
    --system)
        echo "System-wide installation successful. The stdlib is available at:"
        echo "  $INSTALL_DIR"
        echo ""
        echo "All users can now use the Magolor standard library."
        ;;
    --local)
        echo "Local installation successful. The stdlib is available at:"
        echo "  $INSTALL_DIR"
        echo ""
        echo "This is useful for development. Run magolor from this directory."
        ;;
esac

echo ""
info "To verify stdlib is found, run:"
echo "  magolor check <your_file.mg>"
echo ""
info "To set a custom stdlib location, use:"
echo "  export MAGOLOR_STDLIB=/path/to/stdlib"
echo ""

success "Setup complete! 🎉"
