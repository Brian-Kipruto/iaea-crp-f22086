"""
xcom_reference.py — the analytical attenuation reference for cttwin.

WHY THIS FILE EXISTS
--------------------
The vault carried mu/rho = 0.07375 cm^2/g for iron at 662 keV as a remembered
number with no derivation attached. It is not reproducible from the NIST
elemental tables by either obvious interpolation, and the ambiguity is not
cosmetic: at 40 mm of steel a 0.4% difference in mu becomes a ~0.9% difference
in predicted transmission — roughly half of the 2% acceptance budget, spent
before any physics is tested.

So the reference is computed here, from tabulated source data, in code that can
be re-run and checked. Nothing downstream hardcodes a coefficient.

METHOD
------
1. Take mu/rho for Fe (Z=26) and C (Z=6) from NIST Table 3, at the two grid
   points bracketing the Cs-137 line (0.6 and 0.8 MeV).
2. Interpolate LOG-LOG in energy. This is the interpolation the NIST/XCOM
   tables are built to be read with; mu/rho is close to a power law in this
   region, where Compton scattering dominates and there are no edges. Linear
   interpolation of the same points gives 0.0739 instead of 0.0734 — a 0.65%
   difference, which is exactly the ambiguity being removed here.
3. Combine the elements by mass fraction (Bragg's additivity rule):
       (mu/rho)_compound = sum_i w_i * (mu/rho)_i
   for the CarbonSteel defined in DetectorConstruction.cc: 99% Fe, 1% C.
   Carbon has a higher Z/A than iron, so its mu/rho is higher, and the compound
   sits ~0.05% above pure iron. Small, but computed rather than assumed —
   quoting a steel measurement against an iron coefficient is the kind of thing
   a reviewer asks about.
4. Evaluate at 661.657 keV, the Ba-137m -> Ba-137 transition energy, which is
   the SAME value Constants.hh gives the particle gun. These two must not drift
   apart. See ADR 0004.

SOURCE
------
NIST Standard Reference Database 126 (Hubbell & Seltzer), Table 3, elemental
media: https://physics.nist.gov/PhysRefData/XrayMassCoef/tab3.html

Run directly to print the reference table:
    python3 python/xcom_reference.py
"""

from __future__ import annotations

import math
from dataclasses import dataclass

# --- Locked physical inputs -------------------------------------------------

# Cs-137 gamma line. MUST match Physics::kCs137GammaEnergy in include/Constants.hh.
CS137_ENERGY_MEV = 0.661657

# CarbonSteel as defined in src/DetectorConstruction.cc.
STEEL_DENSITY_G_CM3 = 7.85
STEEL_COMPOSITION = {"Fe": 0.99, "C": 0.01}   # mass fractions

# NIST Table 3, mu/rho in cm^2/g, at the grid points bracketing 0.661657 MeV.
# (energy_MeV, mu_over_rho_cm2_per_g)
NIST_TABLE3 = {
    "Fe": [(0.6, 7.704e-2), (0.8, 6.699e-2)],
    "C":  [(0.6, 8.058e-2), (0.8, 7.076e-2)],
}


@dataclass(frozen=True)
class AttenuationReference:
    """The analytical reference a Beer-Lambert comparison is made against."""

    energy_mev: float
    mu_over_rho_cm2_g: float      # compound, mass-weighted
    density_g_cm3: float

    @property
    def mu_per_cm(self) -> float:
        return self.mu_over_rho_cm2_g * self.density_g_cm3

    @property
    def mu_per_mm(self) -> float:
        return self.mu_per_cm / 10.0

    @property
    def hvl_mm(self) -> float:
        """Half-value layer — the sanity check that catches an order-of-magnitude
        slip instantly. For carbon steel at 662 keV it should be ~12 mm."""
        return math.log(2.0) / self.mu_per_mm

    def transmission(self, thickness_mm: float) -> float:
        """Predicted N/N0 for a narrow beam through `thickness_mm` of steel.

        This is PRIMARY transmission — the unscattered fraction. It is why the
        simulation counts unscattered primaries separately; comparing this
        against a count that includes forward-scattered photons compares two
        different quantities. See ADR 0004.
        """
        return math.exp(-self.mu_per_mm * thickness_mm)


def loglog_interpolate(energy_mev: float,
                       points: list[tuple[float, float]]) -> float:
    """Log-log interpolation between the two tabulated points bracketing `energy_mev`."""
    (e1, y1), (e2, y2) = points[0], points[1]
    if not (e1 <= energy_mev <= e2):
        raise ValueError(
            f"{energy_mev} MeV is outside the tabulated bracket [{e1}, {e2}] MeV. "
            "Extend NIST_TABLE3 with the correct grid points rather than "
            "extrapolating."
        )
    frac = (math.log(energy_mev) - math.log(e1)) / (math.log(e2) - math.log(e1))
    return math.exp(math.log(y1) + frac * (math.log(y2) - math.log(y1)))


def carbon_steel_reference(
    energy_mev: float = CS137_ENERGY_MEV,
) -> AttenuationReference:
    """Build the mass-weighted mu/rho reference for cttwin's CarbonSteel."""
    total = sum(STEEL_COMPOSITION.values())
    if abs(total - 1.0) > 1e-9:
        raise ValueError(f"Mass fractions must sum to 1.0, got {total}")

    mu_rho = sum(
        weight * loglog_interpolate(energy_mev, NIST_TABLE3[element])
        for element, weight in STEEL_COMPOSITION.items()
    )
    return AttenuationReference(
        energy_mev=energy_mev,
        mu_over_rho_cm2_g=mu_rho,
        density_g_cm3=STEEL_DENSITY_G_CM3,
    )


# Thicknesses the Phase 1 acceptance criterion is stated at.
VALIDATION_THICKNESSES_MM = (5.0, 10.0, 20.0, 40.0)


def main() -> None:
    ref = carbon_steel_reference()
    fe_only = loglog_interpolate(CS137_ENERGY_MEV, NIST_TABLE3["Fe"])

    print("cttwin — analytical attenuation reference")
    print("=" * 62)
    print(f"  Energy                 : {ref.energy_mev * 1000:.3f} keV (Cs-137)")
    print(f"  mu/rho, Fe only        : {fe_only:.7f} cm^2/g")
    print(f"  mu/rho, 99Fe+1C        : {ref.mu_over_rho_cm2_g:.7f} cm^2/g"
          f"  ({(ref.mu_over_rho_cm2_g / fe_only - 1) * 100:+.3f}% vs Fe)")
    print(f"  rho                    : {ref.density_g_cm3} g/cm^3")
    print(f"  mu                     : {ref.mu_per_cm:.7f} /cm")
    print(f"  Half-value layer       : {ref.hvl_mm:.4f} mm")
    print()
    print("  Predicted primary transmission and 1-sigma counting precision")
    print("  at 1e6 primaries per configuration:")
    print(f"    {'t (mm)':>8}  {'N/N0':>10}  {'N @ 1e6':>10}  {'1 sigma':>9}")
    for t in VALIDATION_THICKNESSES_MM:
        trans = ref.transmission(t)
        n = trans * 1.0e6
        print(f"    {t:>8.0f}  {trans:>10.6f}  {n:>10.0f}  {100 / math.sqrt(n):>8.3f}%")
    print()
    print("  Acceptance: within 2% relative at every thickness "
          "(Architecture Lockdown / Phase 1).")


if __name__ == "__main__":
    main()
