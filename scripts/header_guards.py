#!/usr/bin/env python3
import os
import re
import argparse
import sys
import difflib
import subprocess


def get_project_name():
    """Returns the name of the current Git repository root or current directory."""
    try:
        repo_root = (
            subprocess.check_output(
                ["git", "rev-parse", "--show-toplevel"], stderr=subprocess.DEVNULL
            )
            .strip()
            .decode("utf-8")
        )
        return os.path.basename(repo_root)
    except subprocess.CalledProcessError:
        return os.path.basename(os.path.abspath("."))


def get_staged_files():
    """Returns a list of staged C/C++ header files."""
    try:
        output = subprocess.check_output(
            ["git", "diff", "--cached", "--name-only", "--diff-filter=ACM"],
            stderr=subprocess.DEVNULL,
        ).decode("utf-8")
        return [f for f in output.splitlines() if f.endswith((".h", ".hpp"))]
    except subprocess.CalledProcessError:
        print("Error: Must be run inside a git repository to use --staged.")
        sys.exit(1)


def generate_guard(filepath, project_name):
    """
    Generates the Google-style header guard.
    Omits the first-level directory. Example: src/math/vector.h -> PROJECT_MATH_VECTOR_H_
    """
    normalized_path = filepath.replace(os.sep, "/")
    parts = normalized_path.split("/")

    if len(parts) > 1:
        parts = parts[1:]

    adjusted_path = "/".join(parts)
    raw_guard = f"{project_name}/{adjusted_path}_"
    guard = re.sub(r"[^A-Za-z0-9]", "_", raw_guard)
    return re.sub(r"_+", "_", guard.upper())


def process_file(filepath, project_name, mode):
    """
    Processes a single file.
    mode can be: 'fix', 'dry-run', or 'check'.
    Returns True if the file is compliant (or was fixed successfully), False if it fails check mode.
    """
    expected_guard = generate_guard(filepath, project_name)

    try:
        with open(filepath, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Skipping {filepath} (Error reading: {e})")
        return True

    original_lines = lines.copy()
    ifndef_idx, define_idx, endif_idx = -1, -1, -1

    # Scan from top
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("#ifndef") and ifndef_idx == -1:
            ifndef_idx = i
        elif stripped.startswith("#define") and ifndef_idx != -1 and define_idx == -1:
            define_idx = i
            break

    # Scan from bottom
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip().startswith("#endif"):
            endif_idx = i
            break

    has_structure = ifndef_idx != -1 and define_idx != -1 and endif_idx != -1

    if has_structure:
        # Check if already correct
        current_ifndef = lines[ifndef_idx].strip().split()[-1]
        is_endif_correct = f"// {expected_guard}" in lines[endif_idx]

        if current_ifndef == expected_guard and is_endif_correct:
            return True  # No changes needed

        # Malformed: standardize the format
        lines[ifndef_idx] = f"#ifndef {expected_guard}\n"
        lines[define_idx] = f"#define {expected_guard}\n"
        indent = lines[endif_idx][
            : len(lines[endif_idx]) - len(lines[endif_idx].lstrip())
        ]
        lines[endif_idx] = f"{indent}#endif  // {expected_guard}\n"
    else:
        # Missing structure entirely: wrap the whole file
        if not lines:
            lines = [
                f"#ifndef {expected_guard}\n",
                f"#define {expected_guard}\n",
                f"\n#endif  // {expected_guard}\n",
            ]
        else:
            lines.insert(0, f"#define {expected_guard}\n")
            lines.insert(0, f"#ifndef {expected_guard}\n")
            if not lines[-1].endswith("\n"):
                lines[-1] += "\n"
            lines.append(f"\n#endif  // {expected_guard}\n")

    # Handle requested mode
    if mode == "check":
        # If we got here, original_lines != lines, meaning it needed fixing.
        return False

    elif mode == "dry-run":
        print(f"\n--- Proposed changes for: {filepath} ---")
        diff = difflib.unified_diff(
            original_lines, lines, fromfile=f"a/{filepath}", tofile=f"b/{filepath}", n=3
        )
        sys.stdout.writelines(diff)
        return True

    elif mode == "fix":
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(lines)
        action = "Fixed malformed" if has_structure else "Added missing"
        print(f"[{action}] {filepath} -> {expected_guard}")
        return True


def main():
    parser = argparse.ArgumentParser(
        description="Manage C/C++ header guards (Google Style)."
    )

    # Modes
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "--fix", action="store_true", help="Modify files to fix or add header guards."
    )
    group.add_argument(
        "--dry-run",
        action="store_true",
        help="Print diffs to stdout without modifying.",
    )
    group.add_argument(
        "--check",
        action="store_true",
        help="Return exit code 1 if any guards are malformed/missing.",
    )

    # Filters
    parser.add_argument(
        "--staged", action="store_true", help="Only scan files currently staged in Git."
    )
    parser.add_argument(
        "--whitelist",
        nargs="+",
        help="Space-separated list of first-level subdirectories to include.",
    )

    args = parser.parse_args()

    if args.fix:
        mode = "fix"
    elif args.dry_run:
        mode = "dry-run"
    elif args.check:
        mode = "check"

    project_name = get_project_name()
    valid_extensions = (".h", ".hpp")
    files_to_process = []

    # 1. Gather files
    if args.staged:
        files_to_process = get_staged_files()
        if args.whitelist:
            # Filter staged files by whitelist
            files_to_process = [
                f
                for f in files_to_process
                if any(f.startswith(w + "/") for w in args.whitelist)
            ]
    else:
        for root, dirs, files in os.walk("."):
            if root == ".":
                if args.whitelist:
                    dirs[:] = [d for d in dirs if d in args.whitelist]
                    continue
                else:
                    dirs[:] = [d for d in dirs if not d.startswith(".")]
            else:
                dirs[:] = [d for d in dirs if not d.startswith(".")]

            for file in files:
                if file.endswith(valid_extensions):
                    full_path = os.path.join(root, file)
                    rel_path = os.path.relpath(full_path, ".")
                    files_to_process.append(rel_path)

    # 2. Process files
    if not args.check:
        print(f"Running in {mode.upper()} mode on {len(files_to_process)} files...")

    failed_files = []
    for filepath in files_to_process:
        success = process_file(filepath, project_name, mode)
        if mode == "check" and not success:
            failed_files.append(filepath)

    # 3. Finalize
    if mode == "check":
        if failed_files:
            print(
                "\n❌ COMMIT REJECTED: Header guards missing or malformed in the following files:\n"
            )
            for f in failed_files:
                print(f"  - {f}  (Expected: {generate_guard(f, project_name)})")
            print(
                "\nRun `python3 scripts/header_guards.py --fix --staged` to automatically fix them before committing."
            )
            sys.exit(1)
        else:
            print("✅ All staged header guards are perfectly formatted.")
            sys.exit(0)
    else:
        print("\nComplete.")


if __name__ == "__main__":
    main()
