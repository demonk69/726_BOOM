# PF2 Fault and JAL Precedence

`PF2_FAULT_JAL_PRECEDENCE_POLICY=OLDEST_SURVIVING_ARCHITECTURAL_TOKEN_IN_PC_ORDER`

`PF2_FAULT_JAL_PRECEDENCE_VERIFIED=true`

## Fault Granularity

Precedence is resolved per instruction lane after `build_fetch_packet` has produced complete canonical instructions. The original packet mask records all produced lanes. The first fault in PC order forms the provisional through-fault mask and removes only younger lanes. P1 receives only valid, nonfaulting lanes older than that fault, so a fault is never decoded as a CFI and cannot suppress an older architectural token. The fault remains the owner unless an older surviving CFI, specifically a static JAL in PF2, masks that fault lane.

After P1, the oldest surviving CFI controls PF2 behavior. A static-target JAL masks all younger lanes, including a younger fault. Conditional branches preserve the through-fault mask because PF2 is `SHADOW_ONLY`. JALR has no Frontend target prediction, and no-CFI packets bypass P2. The resulting mask is frozen as the packet's final admission mask.

## Cases A-F

| Case | Packet in PC order | P1-visible lanes | Final mask and fault owner | Frontend behavior |
|---|---|---|---|---|
| A | lane 0 fault, any younger lane absent or invalidated | none | `01`; lane 0 is the fault owner | No predictor request or steering; fault is admitted precisely. |
| B | lane 0 no-CFI, lane 1 fault | lane 0 | `11`; lane 1 remains the fault owner | No predictor request; sequential PC and atomic through-fault admission. |
| C | lane 0 JAL, lane 1 fault | lane 0 JAL | `01`; younger lane 1 fault is masked and has no architectural ownership | Static JAL target steers Frontend; no P2 wait or request. |
| D | lane 0 conditional, lane 1 fault | lane 0 conditional | `11`; lane 1 remains the fault owner | P2 request/response occurs, but taken and not-taken responses are shadow-only: no PC steering and no younger-lane masking. |
| E | lane 0 JALR, lane 1 fault | lane 0 JALR | `11`; lane 1 remains the fault owner | No target prediction or predictor request; Frontend follows fallthrough. |
| F | no fault | all lanes in the original mask | no fault owner; final mask follows ordinary earliest-CFI semantics | no-CFI bypasses P2, JAL statically steers and masks younger lanes, conditional waits in shadow mode, and JALR falls through without target prediction. |

## Evidence

- Native directed PF2: exact `2239/2239`, including lane-0 compressed JAL plus lane-1 illegal-fault precedence.
- Vitis CSim directed PF2: exact `2239/2239` using the same product Frontend path.
- Focused generated RTL: exact `116/116`, including original mask `11`, selected lane 0 JAL, final mask `01`, static target steering, no predictor request, and one admitted architectural lane for the younger-fault case.
- Persistent PF2 random: `256 x 8192`, with all 15 error fields and aggregate errors equal to zero.

These cases do not enable conditional steering, JALR prediction, Commit BIM training, prediction metadata propagation, or FTQ product integration.
