# PF2 Frontend State Machine

| Logical state | Entry | Stored data | Request policy | Output/admission | Redirect/reset |
|---|---|---|---|---|---|
| NORMAL | no held packet | IMEM owner/carry | one IMEM request allowed | build complete canonical packet | higher-priority redirect clears path |
| BYPASS_ADMIT | no-CFI, JAL, JALR, or fault packet built | packet and final mask | no-CFI/JAL/JALR may issue next request | packet attempts atomic FB enqueue next step | packet killed |
| PREDICT_WAIT | conditional request accepted | packet, masks, P1 selected CFI, epoch/generation/token | no younger IMEM request | matching response N+1; held if FB blocked | context invalidated; response drained stale |

The implementation preserves the existing packet-build N/admission N+1 boundary. P2 is called once per Frontend logical step and has no same-step turnover.
