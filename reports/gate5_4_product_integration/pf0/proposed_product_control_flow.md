# Proposed Product Control Flow

This is a staged target contract, not an implementation.

```text
matched IMEM response + valid carry
  -> canonical 1/2-instruction Fetch Packet construction
  -> P1 predecode of complete, nonfaulting canonical instructions
  -> earliest valid CFI selection
  -> no-CFI/JAL/JALR bypass OR conditional P2 request
  -> conditional PREDICT_WAIT (request N, response N+1)
  -> final next-PC selection and post-prediction younger-lane mask
  -> one atomic {Fetch Buffer enqueue + exactly one F1 allocation}
  -> scalar Decode carrying full PC plus exact FTQ reference
  -> Rename/Dispatch -> ROB
  -> Execute actual CFI resolution
  -> prediction comparison and immediate recovery
  -> in-order Commit
  -> commit-qualified conditional BIM update
  -> exact lane retirement and ordered FTQ reclaim
```

Required stage boundaries are `RESPONSE/PACKET_BUILD/PREDECODE`, `PREDICT_WAIT` for conditional branches, and `PACKET_ADMIT`. The response/predecode/BIM/mask/enqueue chain must not be collapsed into one product cycle.

No-CFI bypasses P2. JAL uses its P1 static target and records equivalent prediction metadata without a P2 wait. Foundation JALR is `NO_TARGET_PREDICTION`: it follows fallthrough until Execute redirects an actually taken JALR. Fault-only packets bypass prediction but still receive one FTQ entry at atomic admission.

## Reset Composition

Runtime reset has priority over every request, response, admission, update, and reclaim event. In the same logical reset step it clears Frontend request/response/carry/pending controls and FB controls, invokes P2 lazy-valid generation invalidation, invalidates F1 controls while advancing its allocation generation, and suppresses all handshakes. Payload arrays are not reset. Any later IMEM or predictor response is drained and rejected by its independent epoch/token; any old FTQ reference fails `{idx,generation}` validation.
