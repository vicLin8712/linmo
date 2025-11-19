#!/usr/bin/env bash

# Generate logo PNG from ASCII art in README.md
#
# This script extracts ASCII art from a specially formatted comment block
# in README.md and converts it to a PNG image using ImageMagick.
#
# Requirements:
#   - ImageMagick (magick or convert command)
#   - Intel One Mono font (with fallback to Menlo/Courier)

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
README="$PROJECT_ROOT/README.md"
OUTPUT_DIR="$PROJECT_ROOT/Documentation"
OUTPUT_FILE="$OUTPUT_DIR/logo.png"

# Font preferences (in order of preference)
FONTS=("Intel-One-Mono" "Menlo-Regular" "Courier-New" "Courier")

# ImageMagick parameters
POINTSIZE=15
INTERLINE_SPACING=-4
KERNING=0
CANVAS_WIDTH=1600

# Expected minimum number of lines in ASCII art
MIN_LINES=10

# Extract ASCII art from README.md comment block
extract_ascii_art() {
    if [[ ! -f "$README" ]]; then
        echo "Error: README.md not found at $README" >&2
        return 1
    fi

    local ascii_art
    ascii_art=$(awk '/<!--- Linmo logo source/,/-->/' "$README" \
        | sed '1d;$d' \
        | sed 's/^   //')

    if [[ -z "$ascii_art" ]]; then
        echo "Error: No ASCII art found in README.md comment block" >&2
        echo "Expected block format:" >&2
        echo "  <!--- Linmo logo source" >&2
        echo "  [ASCII art content]" >&2
        echo "  -->" >&2
        return 1
    fi

    # Validate line count
    local line_count
    line_count=$(echo "$ascii_art" | wc -l | tr -d ' ')
    if [[ "$line_count" -lt "$MIN_LINES" ]]; then
        echo "Warning: ASCII art has only $line_count lines (expected at least $MIN_LINES)" >&2
    fi

    echo "$ascii_art"
}

# Detect available ImageMagick command
detect_imagemagick() {
    if command -v magick &> /dev/null; then
        echo "magick"
    elif command -v convert &> /dev/null; then
        echo "convert"
    else
        return 1
    fi
}

# Detect available font from preference list
detect_font() {
    local available_fonts
    available_fonts=$(magick -list font 2> /dev/null | grep -i "Font:" | awk '{print $2}' || echo "")

    for font in "${FONTS[@]}"; do
        if echo "$available_fonts" | grep -qi "^$font$"; then
            echo "$font"
            return 0
        fi
    done

    # No preferred font found, return first available monospace font
    echo "Courier"
}

# Generate PNG using ImageMagick
generate_png() {
    local ascii_art="$1"

    # Detect ImageMagick command
    local magick_cmd
    if ! magick_cmd=$(detect_imagemagick); then
        echo "Error: ImageMagick is not installed." >&2
        echo "Please install ImageMagick:" >&2
        echo "  macOS:         brew install imagemagick" >&2
        echo "  Ubuntu/Debian: sudo apt-get install imagemagick" >&2
        echo "  Fedora/RHEL:   sudo dnf install ImageMagick" >&2
        exit 1
    fi

    # Detect best available font
    local font
    font=$(detect_font)
    echo "Using font: $font"

    # Ensure output directory exists
    if [[ ! -d "$OUTPUT_DIR" ]]; then
        echo "Creating output directory: $OUTPUT_DIR"
        mkdir -p "$OUTPUT_DIR"
    fi

    # Generate PNG with optimized parameters
    # - background: black for dark theme
    # - fill: white text for maximum contrast
    # - font: Intel One Mono for excellent Unicode support
    # - pointsize: 16 for compact yet readable text
    # - interline-spacing: -4 for tight vertical spacing
    # - kerning: 0 for standard character spacing
    # - caption: better text rendering than label
    # - trim: remove excess whitespace
    # - +repage: reset virtual canvas
    echo "$ascii_art" | $magick_cmd \
        -background black \
        -fill white \
        -font "$font" \
        -pointsize "$POINTSIZE" \
        -interline-spacing "$INTERLINE_SPACING" \
        -kerning "$KERNING" \
        -size "${CANVAS_WIDTH}x" \
        caption:@- \
        -trim \
        +repage \
        "$OUTPUT_FILE"

    # Verify output file was created
    if [[ ! -f "$OUTPUT_FILE" ]]; then
        echo "Error: Failed to generate $OUTPUT_FILE" >&2
        return 1
    fi

    # Display output information
    local size
    size=$(identify "$OUTPUT_FILE" 2> /dev/null | awk '{print $3}' || echo "unknown")
    echo "✓ Logo generated successfully"
    echo "  Output: $OUTPUT_FILE"
    echo "  Size:   $size"
}

# Display usage information
usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Generate Linmo logo PNG from ASCII art in README.md

Options:
  -h, --help     Display this help message

Environment Variables:
  POINTSIZE             Font size (default: $POINTSIZE)
  INTERLINE_SPACING     Line spacing (default: $INTERLINE_SPACING)
  KERNING              Character spacing (default: $KERNING)

Example:
  $0
  POINTSIZE=20 $0

EOF
}

# Main execution
main() {
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h | --help)
                usage
                exit 0
                ;;
            *)
                echo "Error: Unknown option: $1" >&2
                usage
                exit 1
                ;;
        esac
        shift
    done

    echo "Extracting ASCII art from README.md..."
    local ascii_art
    if ! ascii_art=$(extract_ascii_art); then
        exit 1
    fi

    echo "Generating PNG image..."
    if ! generate_png "$ascii_art"; then
        exit 1
    fi
}

main "$@"
