"""
run_scan.py — drive cttwin through a full translate-rotate CT scan.

Loops over (angle, translation) pairs, generates a Geant4 macro per
projection from full_scan_template.mac, runs cttwin, and collects the
per-projection CSV output into data/raw/.

Implementation lands in Pass 5.
"""

from __future__ import annotations


def main() -> None:
    raise NotImplementedError(
        "run_scan.py is a Pass 5 deliverable; see docs/architecture.md"
    )


if __name__ == "__main__":
    main()
