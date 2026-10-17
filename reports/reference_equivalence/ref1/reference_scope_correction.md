# REF1 Reference Scope Correction

## Target Layers

| Layer/feature | Generated evidence | REF1 scope status | Target role |
|---|---|---|---|
| TestHarness | harness top instantiates `chiptop: ChipTop`; supplies clocks/resets, serial/JTAG/UART and external simulation memory | `SIMULATION_ONLY` | Reference simulation environment; not an independent processor equivalence boundary |
| ChipTop | Top generated Verilog module and FIRRTL instance in TestHarness | `IN_SCOPE_DEFERRED` | `FINAL_REFERENCE_BOUNDARY` |
| DigitalTop | `ChipTop.system` | `IN_SCOPE_DEFERRED` | `FINAL_REFERENCE_BOUNDARY` |
| BoomTile | DigitalTop reachable tile reset domain contains `boom_tile` | `IN_SCOPE_CURRENT` | `FIRST_DIFFERENTIAL_TARGET`, supported architectural subset |
| BoomCore | `BoomTile.core`; retirement/architectural state boundary | `IN_SCOPE_CURRENT` | `FIRST_DIFFERENTIAL_TARGET`, supported architectural subset |
| L2 InclusiveCache | reachable via `DigitalTop.subsystem_l2_wrapper.l2` | `IN_SCOPE_DEFERRED` | reachable final-boundary integration, not absent/out of scope |
| TileLink buses/atomic adapters | DigitalTop/Tiles contain TileLink interconnect, monitors, buffers and atomic adapter instances | `IN_SCOPE_DEFERRED` | final boundary transaction protocol target |
| ICache/DCache | BoomFrontend.ICache and BoomTile.BoomNonBlockingDCache instances are reachable | `IN_SCOPE_DEFERRED` | cache behavior deferred while first target abstracts memory streams |
| MMU/TLB/PTW | frontend/TLB and tile PTW instances plus connections from frontend/LSU/core | `IN_SCOPE_DEFERRED` | translation and page walks deferred, not removed from final reference scope |
| Optional RoCC accelerator endpoint | BoomTile FIRRTL marks `core.io.rocc.*` signals invalid; no reachable RoCC implementation instance was found | `REFERENCE_ABSENT` | no product or equivalence requirement derived from absent endpoint |

## Reachability Evidence

Generated hierarchy path: `TestHarness.chiptop:ChipTop.system:DigitalTop.subsystem_l2_wrapper:CoherenceManagerWrapper.l2:InclusiveCache`; `DigitalTop.tile_prci_domain.tile_reset_domain.boom_tile:BoomTile`; within BoomTile: `core:BoomCore`, `frontend:BoomFrontend`, `lsu:LSU`, `dcache:BoomNonBlockingDCache`, and `ptw:PTW`. `BoomFrontend` instantiates ICache, BranchPredictor, BoomRAS and TLB. Thus L2, TileLink, cache and MMU blocks are reachable within ChipTop/DigitalTop.

The first target does not claim those deferred blocks equivalent. It starts at the BoomCore/BoomTile architectural subset. Transaction comparison across TileLink versus HLS IMEM/DMEM streams requires the explicit contract in `transaction_boundary_contract.md`.

Previous audit files are unchanged; this scope correction is additive in REF1.

`FINAL_REFERENCE_BOUNDARY=ChipTop/DigitalTop`

`FIRST_DIFFERENTIAL_TARGET=BoomCore/BoomTile supported architectural subset`

`TestHarness=SIMULATION_ONLY`
