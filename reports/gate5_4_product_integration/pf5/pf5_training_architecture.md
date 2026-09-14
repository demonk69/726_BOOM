# PF5 Training Architecture

PF5_ACTUAL_OUTCOME_SOURCE=`ExecuteState::AluResult.actual_taken -> RobCompleteEvent.actual_taken -> RobEntry.branch_actual_taken`

PF5_TRAINING_METADATA_SOURCE=`FtqPredictionLookup.predictor_metadata_index,predictor_generation qualified by ftq_idx/allocation_generation/lane/CFI`

PF5_UPDATE_ARBITRATION_POLICY=`ONE_COMMIT_UPDATE_PENDING; STALL_ELIGIBLE_COMMIT_IF_OCCUPIED; FRONTEND_SINGLE_PREDICTOR_STEP_CONSUMES_UPDATE`

1. Execute computes `actual_taken` for each conditional branch operation.
2. Completion validates the ROB allocation owner and records only
   `branch_resolved` and `branch_actual_taken` in that ROB entry.
3. Commit can read the recorded direction after all busy, exception, memory,
   trace-backpressure, and branch-kill decisions.
4. Commit retains the exact FTQ allocation reference in the uop. Synthesis uses
   the existing `RobInternalState::ftq_generations` sidecar.
5. FTQ clears a live lane in `FtqFoundation::step` after a generation-qualified
   retire event.
6. Core ordering is Commit then Frontend. Frontend applies the pending predictor
   update before calling `ftq.step`, so metadata is consumed before retire/reclaim.
7. The canonical P2 API is `PredictorFoundation<256>::step(PredictorStepInput)`
   with `PredictorStepInput::update`.
8. Request and update can occur in the same predictor step.
9. A same-index request observes the newly updated 2-bit counter
   (`UPDATE_FORWARD_NEW_VALUE`).

No full FTQ entry, predicted target, response payload, or packet metadata is
copied into the ROB.
