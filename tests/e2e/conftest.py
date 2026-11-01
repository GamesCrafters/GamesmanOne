"""Shared fixtures for E2E tests."""

import os
import pytest


@pytest.fixture(scope="session")
def gamesman_config() -> tuple[str, dict[str, str]]:
    """
    Determines the binary path and environment variables based on the GAMESMAN_PRESET.
    Defaults to release if the environment variable is not set.
    """
    preset = os.environ.get("GAMESMAN_PRESET", "release")

    # 1. Determine binary path
    if preset == "release":
        bin_path = "./bin/gamesman"
    else:
        bin_path = f"./build/{preset}/src/gamesman"

    # 2. Configure environment variables (e.g., TSAN_OPTIONS)
    env = os.environ.copy()
    if "tsan" in preset.lower():
        existing_tsan = env.get("TSAN_OPTIONS", "")
        # Prepend ignore_noninstrumented_modules=1 to prevent OpenMP false positives
        env["TSAN_OPTIONS"] = (
            f"ignore_noninstrumented_modules=1 {existing_tsan}".strip()
        )

    return bin_path, env


@pytest.fixture(scope="session")
def gamesman_bin(gamesman_config: tuple[str, dict[str, str]]) -> str:
    """
    Convenience fixture returning only the binary path.
    Used by test_interactive.py which doesn't need env vars (pexpect inherits them).
    """
    bin_path, _ = gamesman_config
    return bin_path
