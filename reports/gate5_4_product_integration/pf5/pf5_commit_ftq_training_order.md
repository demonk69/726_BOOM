# Commit, Training, and FTQ Order

The product step order is:

1. `rob_commit_module` makes the final architectural commit decision.
2. Commit performs a narrow FTQ lookup and snapshots BIM index, predictor
   generation, and actual direction into `predictor_update_pending`.
3. Commit publishes the FTQ lane retire event.
4. `frontend_product_module` supplies the update to the one canonical predictor
   step. A simultaneous request uses P2 forwarding.
5. Frontend then supplies the retire event to `FtqFoundation::step`.
6. FTQ may clear the lane and reclaim a zero-live head.

The update snapshot and predictor consumption therefore precede destructive FTQ
reclaim. PF3R redirect-retire retry cannot duplicate training because training
belongs to the one architectural ROB commit, not to retire acceptance.
