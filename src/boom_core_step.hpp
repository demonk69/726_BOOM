#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

static void frontend_step(FrontendState& next, const FrontendState& current,
                          const BoomCoreState& state, const BranchUpdate& brupdate);
static void decode_step(DecodeState& next, const DecodeState& current,
                        const FrontendState& frontend, const BoomCoreState& state);
static void rename_step(RenameState& next, const RenameState& current,
                        const DecodeState& decode, const BoomCoreState& state);
static void issue_step(IssueState& next, const IssueState& current,
                       const RenameState& rename, const ExecuteState& execute,
                       const BoomCoreState& state);
static void execute_step(ExecuteState& next, const ExecuteState& current,
                          const IssueState& issue, const BoomCoreState& state);
static void branch_step(BranchUpdate& brupdate, const ExecuteState& execute,
                         const BoomCoreState& state);
static void lsu_step(LsuState& next, const LsuState& current,
                     const BoomCoreState& state);
static void commit_step(RobInternalState& rob_next, const RobInternalState& rob_current,
                         const ExecuteState& execute, BoomCoreState& state);
static void csr_step(CsrState& next, const CsrState& current,
                     const ExecuteState& execute, const BoomCoreState& state);
