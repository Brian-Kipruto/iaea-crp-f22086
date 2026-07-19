#ifndef CTTWIN_ActionInitialization_h
#define CTTWIN_ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

namespace CTTwin
{

class ActionInitialization : public G4VUserActionInitialization
{
  public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;
};

}  // namespace CTTwin

#endif
