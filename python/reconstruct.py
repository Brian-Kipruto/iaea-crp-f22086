"""
reconstruct.py — reconstruct CT slices from assembled sinograms.

Loads a sinogram from data/sinograms/, runs TomoPy FBP (gridrec) and
ASTRA SIRT, saves both reconstructions to data/reconstructions/ for
side-by-side comparison.

Implementation lands in Pass 5.
"""

from __future__ import annotations


def main() -> None:
    raise NotImplementedError(
        "reconstruct.py is a Pass 5 deliverable; see docs/architecture.md"
    )


if __name__ == "__main__":
    main()
