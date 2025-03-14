"""
This is a script for converting all 0b literals to 0x
in a given text file.
"""

import re

def convert_binary_to_hex(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    
    # Regex to match 0b binary constants
    binary_pattern = r'0b[01]+'
    
    # Function to convert binary to hex
    def binary_to_hex(match):
        binary_value = match.group(0)
        hex_value = hex(int(binary_value, 2))
        return hex_value
    
    # Replace all binary constants with their hex equivalents
    updated_content = re.sub(binary_pattern, binary_to_hex, content)
    
    # Write the updated content back to the file
    with open(file_path, 'w') as file:
        file.write(updated_content)

# Replace with the path to your C source file
file_path = 'quixo.c'
convert_binary_to_hex(file_path)
