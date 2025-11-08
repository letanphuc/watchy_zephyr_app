#!/usr/bin/env python3
"""
Image to C array converter for SSD16xx displays
Converts PNG images to 1-bit monochrome C arrays compatible with Zephyr SSD16xx driver
"""

import sys
import os
from PIL import Image
import argparse


def png_to_gray_c_array(input_path, output_path=None, array_name=None, target_size=(200, 200)):
    """
    Convert PNG image to 1-bit monochrome C array for display buffer

    Args:
        input_path: Path to input PNG file
        output_path: Path to output C file (optional)
        array_name: Name for the C array (optional)
        target_size: Target size tuple (width, height) for resizing
    """    # Open and convert image to grayscale
    try:
        img = Image.open(input_path)
        print(f"Original image size: {img.size}")
        print(f"Original image mode: {img.mode}")

        # Resize image to target size
        if img.size != target_size:
            img = img.resize(target_size, Image.Resampling.LANCZOS)
            print(f"Resized to: {target_size}")

        # Convert to grayscale
        if img.mode != 'L':
            img = img.convert('L')

        # Get image dimensions
        width, height = img.size

        print(f"Converted to 1-bit monochrome: {width}x{height}")

        # Convert to packed bit array
        # Format: 8 pixels per byte, column-major within each row
        # Each byte contains 8 rows for one column
        # 0 = black, 1 = white

        # Calculate buffer size: width * ceil(height/8)
        bytes_per_col = (height + 7) // 8  # Round up to nearest byte
        buffer_size = width * bytes_per_col
        byte_array = [0] * buffer_size

        for col in range(width):
            for row in range(height):
                # Get pixel value and convert to binary (threshold at 128)
                pixel = img.getpixel((col, row))
                is_white = pixel >= 128  # True for white, False for black
                
                # Calculate byte and bit position
                byte_index = col + (row // 8) * width
                bit_index = 7 - (row % 8)
                
                if is_white:
                    byte_array[byte_index] |= (1 << bit_index)  # Set white bit
                # Black pixels remain 0 (already initialized)

        # Generate C array
        if not array_name:
            base_name = os.path.splitext(os.path.basename(input_path))[0]
            array_name = f"{base_name}_gray"

        # Create C header content
        c_content = f"""/* Generated from {os.path.basename(input_path)} */
/* Image size: {width}x{height} pixels */
/* Array size: {len(byte_array)} bytes */
/* Format: 1-bit monochrome, 8 pixels per byte */
/* Pixel format: 0=black, 1=white */
/* Memory layout: column-major, 8 rows per byte */

#include <stdint.h>

#define {array_name.upper()}_WIDTH  {width}
#define {array_name.upper()}_HEIGHT {height}
#define {array_name.upper()}_BYTES  {len(byte_array)}

static const uint8_t {array_name}[] = {{
"""

        # Add byte data (16 bytes per line)
        for i in range(0, len(byte_array), 16):
            line_bytes = byte_array[i:i + 16]
            hex_bytes = [f"0x{b:02x}" for b in line_bytes]
            c_content += f"    {', '.join(hex_bytes)}"
            if i + 16 < len(byte_array):
                c_content += ","
            c_content += "\n"

        c_content += "};\n"

        # Output to file or stdout
        if output_path:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(c_content)
            print(f"C array written to: {output_path}")
        else:
            print("\nGenerated C array:")
            print(c_content)

        # Print usage example
        print(f"""
Usage example in your code:

#include "generated_header.h"

// Buffer size calculation: width * ceil(height/8)
#define BUFFER_SIZE {array_name.upper()}_BYTES

// Copy image data directly to display buffer
uint8_t display_buffer[BUFFER_SIZE];
memcpy(display_buffer, {array_name}, {array_name.upper()}_BYTES);

// For display buffer descriptor
struct display_buffer_descriptor desc = {{
    .buf_size = {array_name.upper()}_BYTES,
    .width = {array_name.upper()}_WIDTH,
    .height = {array_name.upper()}_HEIGHT,
    .pitch = {array_name.upper()}_WIDTH,
}};

// Display the monochrome image
display_write(display_dev, 0, 0, &desc, display_buffer);
""")

        return True

    except (IOError, OSError, ValueError) as e:
        print(f"Error processing image: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='Convert PNG to 1-bit monochrome C array')
    parser.add_argument('input', help='Input PNG file path')
    parser.add_argument('-o', '--output', help='Output C file path')
    parser.add_argument('-n', '--name', help='C array name')
    parser.add_argument('--threshold', type=int, default=128, help='Threshold for black/white conversion (0-255)')
    parser.add_argument('-s', '--size', type=str, default='200x200', help='Target size (e.g., 200x200)')

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: Input file '{args.input}' not found")
        return 1

    # Parse size argument
    try:
        width, height = map(int, args.size.split('x'))
        target_size = (width, height)
    except ValueError:
        print(f"Error: Invalid size format '{args.size}'. Use WIDTHxHEIGHT (e.g., 200x200)")
        return 1

    success = png_to_gray_c_array(args.input, args.output, args.name, target_size)
    return 0 if success else 1


if __name__ == "__main__":
    # If run without arguments, process the cat.png file
    if len(sys.argv) == 1:
        # Default to cat.png in imgs directory
        script_dir = os.path.dirname(os.path.abspath(__file__))
        app_dir = os.path.dirname(script_dir)
        cat_png = os.path.join(app_dir, "imgs", "cat.png")

        if os.path.exists(cat_png):
            print(f"Processing default file: {cat_png}")
            output_file = os.path.join(app_dir, "src", "cat_image.h")
            # Use 200x200 as default size for display
            png_to_gray_c_array(cat_png, output_file, "cat_gray", (200, 200))
        else:
            print(f"Default file {cat_png} not found")
            print("Usage: python img_convert.py <input.png> [-o output.h] [-n array_name] [-s WIDTHxHEIGHT]")
    else:
        sys.exit(main())
