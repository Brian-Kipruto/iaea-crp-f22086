#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"

namespace CTTwin
{

void ActionInitialization::BuildForMaster() const
{
  // Master thread: RunAction only (it merges per-thread accumulables).
  SetUserAction(new RunAction);
}

void ActionInitialization::Build() const
{
  SetUserAction(new PrimaryGeneratorAction);

  // RunAction first, then hand it to EventAction so per-event counts can be
  // accumulated into the run total.
  auto* runAction = new RunAction;
  SetUserAction(runAction);
  SetUserAction(new EventAction(runAction));
}

}  // namespace CTTwin
