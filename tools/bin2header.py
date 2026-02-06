#!/usr/bin/env python3
"""
bin2header.py - Convert binary files to C header arrays
Usage: python bin2header.py <input.sys> <output.h> <array_name>
"""
import sys
import os

def bin_to_header(input_path: str, output_path: str, array_name: str) -> None:
    """Convert binary file to C header with const array."""
    with open(input_path, 'rb') as f:
        data = f.read()
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(f"// Auto-generated from {os.path.basename(input_path)}\n")
        f.write(f"// Size: {len(data)} bytes\n")
        f.write(f"// DO NOT EDIT - regenerate with bin2header.py\n\n")
        f.write(f"#pragma once\n\n")
        f.write(f"STATIC CONST UINT8 {array_name}[] = {{\n")
        
        for i, b in enumerate(data):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{b:02X}")
            if i < len(data) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(data) - 1:
                f.write("\n")
            else:
                f.write(" ")
        
        f.write(f"}};\n\n")
        f.write(f"STATIC CONST UINT32 {array_name}Size = sizeof({array_name});\n")
    
    print(f"[+] Generated {output_path} ({len(data)} bytes)")

def main() -> int:
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input.sys> <output.h> <array_name>")
        return 1
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    array_name = sys.argv[3]
    
    if not os.path.exists(input_path):
        print(f"[!] Error: Input file not found: {input_path}")
        return 1
    
    bin_to_header(input_path, output_path, array_name)
    return 0

if __name__ == "__main__":
    sys.exit(main())
