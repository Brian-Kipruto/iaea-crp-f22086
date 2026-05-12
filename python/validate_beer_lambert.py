"""
validate_beer_lambert.py — Phase 1 physics validation.

Compares Geant4 transmission counts against the analytical Beer-Lambert
prediction `N = N0 * exp(-mu_rho * thickness)` using NIST XCOM mass
attenuation coefficients.

For Cs-137 (662 keV) through carbon steel:
    mu/rho (Fe at 662 keV) ≈ 0.07375 cm^2/g
    rho_steel              ≈ 7.85 g/cm^3
    => mu                  ≈ 0.5789 /cm = 0.05789 /mm

Implementation lands in Pass 3 (immediately after the source switch to
Cs-137 and the physics list is set).
"""

from __future__ import annotations


def main() -> None:
    raise NotImplementedError(
        "validate_beer_lambert.py is a Pass 3 deliverable; see docs/validation_report.md"
    )


if __name__ == "__main__":
    main()
