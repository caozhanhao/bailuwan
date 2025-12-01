import json
import os
import sys
import subprocess
import textwrap
import unicodedata
import argparse
from shutil import get_terminal_size

DEFAULT_WIDTH = 80
KEY_WIDTH = 25

def get_str_width(text):
    return sum(2 if unicodedata.east_asian_width(c) in 'WF' else 1 for c in str(text))

def pad_text(text, target_width):
    text = str(text)
    current_width = get_str_width(text)
    return text + " " * max(0, target_width - current_width)

def print_row(key, value, val_width, is_header=False):
    val_str = str(value)
    lines = textwrap.wrap(val_str, width=val_width) if not is_header else [val_str]
    if not lines: lines = [""]

    print(f"| {pad_text(key, KEY_WIDTH)} | {pad_text(lines[0], val_width)} |")

    for line in lines[1:]:
        print(f"| {pad_text('', KEY_WIDTH)} | {pad_text(line, val_width)} |")

def print_separator(char="-", val_width=50):
    # + (key) + (val) +
    line = f"+{char * (KEY_WIDTH + 2)}+{char * (val_width + 2)}+"
    print(line)

def main():
    parser = argparse.ArgumentParser(description="BaiLuWan Simulation Statistics Viewer")
    parser.add_argument("file", help="Path to the statistics JSON file")
    args = parser.parse_args()

    if not os.path.exists(args.file):
        print(f"Error: File '{args.file}' not found.")
        sys.exit(1)

    try:
        with open(args.file, 'r') as f:
            raw_data = json.load(f)
    except json.JSONDecodeError:
        print("Error: Invalid JSON format.")
        sys.exit(1)

    cycles = raw_data.get('simulator_cycles', 0)
    inst_count = raw_data.get('all_ops', 0)
    ipc = inst_count / cycles if cycles > 0 else 0
    cpi = cycles / inst_count if inst_count > 0 else 0

    table_data = [
        ("Sim Cycles", f"{cycles:,}"),
        ("Instructions", f"{inst_count:,}"),
        ("IPC", f"{ipc:.4f}"),
        ("CPI", f"{cpi:.4f}"),
        ("---", "---")
    ]

    for key, val in raw_data.items():
        val_display = f"{val:,}" if isinstance(val, (int, float)) else val
        table_data.append((key, val_display))

    term_width = get_terminal_size((DEFAULT_WIDTH, 20)).columns
    max_table_width = min(term_width, DEFAULT_WIDTH)
    val_col_width = max_table_width - KEY_WIDTH - 7 # 7 is for borders and spaces "|  |  |"

    print_separator("=", val_col_width)
    print_row("Field", "Value", val_col_width, is_header=True)
    print_separator("=", val_col_width)

    for key, val in table_data:
        if key == "---":
            print_separator("-", val_col_width)
        else:
            print_row(key, val, val_col_width)

    print_separator("=", val_col_width)

if __name__ == "__main__":
    main()