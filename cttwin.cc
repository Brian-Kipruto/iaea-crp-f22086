// =============================================================================
// cttwin.cc — main entry point for the CTTwin first-generation gamma CT
// simulator.
//
// IAEA CRP F22086 — Advanced Nuclear Imaging Techniques for Industrial
// Process Analysis and Component Testing.
// Kenya contribution, University of Nairobi.
//
// Placeholder: this file will be implemented in Pass 1 (see docs/architecture.md).
// =============================================================================

#include <iostream>

int main(int /*argc*/, char** /*argv*/) {
    std::cout
        << "cttwin: placeholder main(). The real implementation lands in Pass 1.\n"
        << "Expected behaviour once complete:\n"
        << "  - construct G4RunManagerFactory in MT mode\n"
        << "  - register DetectorConstruction, physics list, ActionInitialization\n"
        << "  - hand off to either UImanager (batch) or UIExecutive (interactive)\n";
    return 0;
}
