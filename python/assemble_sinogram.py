"""
assemble_sinogram.py — assemble per-projection CSVs into a 2D sinogram.

Reads data/raw/projection_*.csv, stacks them into a NumPy array of
shape (n_angles, n_detector_pixels), saves to data/sinograms/.

Implementation lands in Pass 5.
"""

from __future__ import annotations


def main() -> None:
    raise NotImplementedError(
        "assemble_sinogram.py is a Pass 5 deliverable; see docs/architecture.md"
    )


if __name__ == "__main__":
    main()
