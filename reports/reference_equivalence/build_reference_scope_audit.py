#!/usr/bin/env python3
"""Build a read-only scope/evidence audit for the generated SmallBoomConfig RTL."""
import csv
import hashlib
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
REF = Path("/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig")
OUT = REPO / "reports/reference_equivalence"
RTL = "chipyard.TestHarness.SmallBoomConfig.top.v"
FIR = "chipyard.TestHarness.SmallBoomConfig.fir"
HARNESS = "chipyard.TestHarness.SmallBoomConfig.harness.v"
TOP_FIR = "chipyard.TestHarness.SmallBoomConfig.top.fir"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_csv(path, header, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def source_files():
    # This is the complete HDL/FIRRTL input set present in the delivered
    # generated-reference directory. It deliberately excludes emulator build
    # products in the same-name subdirectory.
    suffixes = {".v", ".sv", ".fir", ".f"}
    return sorted(p for p in REF.iterdir() if p.is_file() and p.suffix in suffixes)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    manifest_rows = []
    for path in source_files():
        role = {
            ".fir": "Generated FIRRTL circuit/module source",
            ".v": "Generated Verilog or black-box resource source",
            ".sv": "Generated SystemVerilog resource source",
            ".f": "Generated simulator/black-box source file list",
        }[path.suffix]
        manifest_rows.append([str(path), path.name, role, path.stat().st_size, digest(path)])
    for path in sorted(REF.glob("*.top.mems.conf")) + sorted(REF.glob("*.harness.mems.conf")):
        manifest_rows.append([str(path), path.name, "Generated memory initialization/config input", path.stat().st_size, digest(path)])
    for name in ["chipyard.TestHarness.SmallBoomConfig.core.config", "chipyard.TestHarness.SmallBoomConfig.json", "chipyard.TestHarness.SmallBoomConfig.l2.json"]:
        path = REF / name
        if path.exists():
            manifest_rows.append([str(path), path.name, "Generated configuration/provenance input (not functional proof)", path.stat().st_size, digest(path)])
    write_csv(OUT / "reference_source_manifest.csv",
              ["absolute_path", "file", "role", "size_bytes", "sha256"], manifest_rows)

    features = [
        ["FPU", "YES", "FPU instance is in BoomCore execution hierarchy; FPU/FPUFMAPipe RTL has datapath, valid/ready and exception/fflags signals.", f"{FIR}:212244; {FIR}:211094; {RTL}:229167", "FPU", "src/boom_core_step.cpp; src/execute.cpp; src/completion.cpp; src/boom_core_merged.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS FP issue lane is forced not-ready and execute FP lane is cleared; canonical merged source contains an FPU pass-through stub. HAS_FPU macro is not implementation evidence."],
        ["FP Issue Queue", "YES", "FP pipeline instantiates IssueUnitCollapsing with IssueSlot instances and connects to FPUExeUnit.", f"{FIR}:229344-229347; {FIR}:224454-224477", "IssueUnitCollapsing (FP pipeline)", "src/boom_state.hpp (IssueState only); src/issue.cpp; src/boom_core_step.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS state declares only the ALU queue; no operational FPU issue queue/FP wakeup path is established, and FP lane is disabled."],
        ["ICache", "YES", "BoomFrontend instantiates ICache and exposes TileLink master interface; generated ICache contains state/control and request/response paths.", f"{FIR}:197128-197138; {RTL}:184929", "ICache / BoomFrontend", "src/frontend.cpp; src/boom_core_top.cpp", "REFERENCE_PRESENT_HLS_MISSING", "HLS frontend consumes/produces external IMEM request/response streams; no cache tags, data arrays, refill/MSHR or TileLink ICache implementation found."],
        ["DCache", "YES", "BoomTile instantiates BoomNonBlockingDCache as dcache and connects LSU and TileLink hierarchy.", f"{FIR}:297255; {FIR}:297509-297510", "BoomNonBlockingDCache / LSU", "src/lsu.cpp; src/boom_core_top.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS LSU implements queueing and external request/response protocol, but not the reference DCache arrays, MSHRs, coherence/refill and TileLink cache interface."],
        ["MMU", "YES", "Sv39 translation is wired through frontend/TLB, tile PTW, CSR privilege/status and physical memory system interfaces.", f"{FIR}:197147-197150; {FIR}:297498-297510; {RTL}:205238; {RTL}:296125", "TLB / PTW / CSRFile", "src/csr.cpp; src/lsu.cpp; src/boom_state.hpp", "REFERENCE_PRESENT_HLS_MISSING", "HLS has satp/privilege state fields, but no virtual-to-physical page-walk/translation behavior or access-fault integration established."],
        ["TLB", "YES", "BoomFrontend instantiates TLB and connects its PTW request interface; tile PTW accepts frontend and core/LSU requestors.", f"{FIR}:197147-197150; {FIR}:297498-297504", "TLB", "src/frontend.cpp; src/lsu.cpp", "REFERENCE_PRESENT_HLS_MISSING", "No TLB set/way/tag/permission state or translation lookup path found in HLS C++."],
        ["PTW", "YES", "BoomTile instantiates PTW and connects requestors from LSU, frontend, core TLB plus memory arbiter.", f"{FIR}:297498-297509", "PTW", "src/lsu.cpp; src/csr.cpp", "REFERENCE_PRESENT_HLS_MISSING", "No page-table-walk FSM, PTE transaction sequencing, or TLB refill path found in HLS."],
        ["CSR/Privilege", "YES", "BoomCore instantiates CSRFile; state/interfaces carry privilege, CSR commands, trap/interrupt/return and status.", f"{FIR}:270843; {RTL}:273427", "CSRFile", "src/csr.cpp; src/reset.cpp; include/boom_state.hpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS implements a subset of machine CSR/trap state. Full CSR map, privilege transitions, delegation, interrupt/exception semantics and reference trace equivalence are not demonstrated."],
        ["AMO/LRSC", "YES", "DCache contains LR/SC reservation count/address state, SC address-match/failure logic, and an instantiated AMOALU datapath; system-bus paths also instantiate TLAtomicAutomata.", f"{RTL}:183370-183391; {RTL}:183963-183968; {FIR}:42534; {RTL}:32319", "BoomNonBlockingDCache LRSC/AMOALU / TLAtomicAutomata", "src/lsu.cpp; include/boom_types.hpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS ordinary load/store transaction path does not establish AMO arithmetic, LR reservation lifetime, SC success/failure, or atomic TileLink semantics."],
        ["Branch Predictor", "YES", "BoomFrontend instantiates BranchPredictor and BoomRAS; prediction/repair interfaces connect to frontend pipeline.", f"{FIR}:197139-197145; {RTL}:204066; {RTL}:204475", "BranchPredictor / BoomRAS", "src/predictor.cpp; src/frontend.cpp; src/branch.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS includes predictor/FTQ state, but reference predictor organization, tables, history, RAS and update timing have not been compared cycle/state-wise."],
        ["FTQ", "YES", "Reference frontend/core interfaces carry ftq_idx; fetch metadata and prediction/retirement interfaces connect through BoomCore and frontend.", f"{FIR}:197118-197122; {FIR}:270105-270108; {RTL}:218105", "BoomFrontend and BoomCore FTQ interfaces", "src/ftq.cpp; src/frontend.cpp; src/branch.cpp; src/commit.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS has FTQ state and dedicated product tests, but no reference-vs-HLS FTQ state transition/timing differential proof has been produced."],
        ["LSU", "YES", "BoomTile instantiates LSU; core LSU interface connects to LSU; LSU dmem connects to DCache; load/store queues and transaction interfaces are explicit.", f"{FIR}:297376-297380; {FIR}:297391-297392; {FIR}:297509-297510; {RTL}:283681", "LSU", "src/lsu.cpp; src/commit.cpp; src/completion.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS includes LQ/SQ, ownership IDs, forwarding and response handling, but compatibility of exceptions, replay, ordering, atomics, translation, cache/MSHR and exact protocol timing remains unverified."],
        ["L2 Cache", "YES", "DigitalTop/configuration hierarchy instantiates InclusiveCache; top-level memory/cache TileLink interfaces connect around it.", f"{FIR}:136595; {RTL}:150913", "InclusiveCache", "No corresponding L2 cache module in HLS core sources; external memory is a top-level interface", "REFERENCE_OUT_OF_SCOPE", "HLS boundary is a core with external IMEM/DMEM streams, not the SoC TileLink fabric/L2. Record as reference-present but outside the current HLS core boundary; do not add as an unstated product requirement."],
    ]
    write_csv(OUT / "reference_feature_inventory.csv",
              ["feature", "reference_present", "reference_rtl_evidence", "evidence_locations", "reference_module_or_instance", "hls_evidence_files", "audit_status", "scope_or_gap_notes"], features)

    mapping = [
        ["TestHarness", "TestHarness", "Simulation harness, serial/JTAG/UART/memory models", "src/boom_core_top.cpp (core interface only)", "REFERENCE_OUT_OF_SCOPE", "HLS core top is not a TestHarness/SoC replacement."],
        ["ChipTop / DigitalTop", "ChipTop -> DigitalTop", "TileLink/clock/reset/periphery/system integration", "src/boom_core_top.cpp (streaming core boundary)", "REFERENCE_OUT_OF_SCOPE", "No ChipTop, buses, devices or complete SoC integration in this HLS design."],
        ["BoomTile", "TileResetDomain.boom_tile", "Tile boundary; connects core, LSU, frontend, PTW, DCache and interrupts", "src/boom_core_top.cpp + BoomCoreState", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS exports IMEM/DMEM streams and lacks the reference tile interconnect and cache/PTW composition."],
        ["BoomCore", "BoomTile.core", "Pipeline control, decode, rename, issue, execute, ROB, commit, CSR", "src/boom_core_step.cpp; src/decode.cpp; src/rename.cpp; src/issue.cpp; src/execute.cpp; src/rob.cpp; src/commit.cpp; src/csr.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Structural correspondence exists; architectural/event-level differential validation is not yet supplied."],
        ["BoomFrontend", "BoomTile.frontend", "Fetch, predecode, prediction, FTQ, ICache/TLB boundary", "src/frontend.cpp; src/fetch_buffer.cpp; src/fetch_packet.cpp; src/predecode.cpp; src/predictor.cpp; src/ftq.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "No cache/MMU implementation; predictor and FTQ state/timing equivalence remains open."],
        ["Rob", "BoomCore.rob", "Allocation, completion, commit, flush/recovery", "src/rob.cpp; src/commit.cpp; src/completion.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Need commit-by-commit architectural differential and state-transition checks."],
        ["IssueUnitCollapsing (integer/memory/FP)", "BoomCore.mem_issue_unit/int_issue_unit plus FP pipeline fp_issue_unit", "Wakeup/select/grant and queue state", "src/issue.cpp; include/boom_state.hpp; src/boom_core_step.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "HLS active queue/lanes do not implement reference FP issue pipeline."],
        ["FPU / FPUExeUnit", "BoomCore FP pipeline", "FP arithmetic, conversion, divide/sqrt, flags and writeback", "src/execute.cpp; src/completion.cpp; src/boom_core_merged.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "FP lane is disabled/stubbed; configuration macros alone do not establish implementation."],
        ["LSU", "BoomTile.lsu", "Load/store queues, ordering, replay, forwarding and request/response", "src/lsu.cpp; src/commit.cpp; src/completion.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Some functional concepts map; cache, MMU, AMO and full protocol behavior do not."],
        ["CSRFile", "BoomCore.csr", "CSR and privilege/trap state", "src/csr.cpp; src/reset.cpp; include/boom_state.hpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Subset implementation; compare only reference-enabled CSR/privilege behavior."],
        ["BranchPredictor / BoomRAS", "BoomFrontend prediction path", "Prediction, metadata, training and redirect repair", "src/predictor.cpp; src/frontend.cpp; src/branch.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Algorithm/state layout and cycle-level training behavior not shown equivalent."],
        ["FTQ", "BoomFrontend/BoomCore FTQ interfaces", "Fetch-target metadata, prediction records, retirement and redirect ownership", "src/ftq.cpp; src/frontend.cpp; src/branch.cpp; src/commit.cpp", "REFERENCE_PRESENT_HLS_PARTIAL", "Functional FTQ tests are not reference differential equivalence."],
        ["ICache / DCache / TLB / PTW", "BoomFrontend.icache/tlb; BoomTile.dcache/ptw", "Instruction/data caching, address translation and page walks", "No corresponding cache/TLB/PTW implementation; src/frontend.cpp and src/lsu.cpp provide external-stream boundary", "REFERENCE_PRESENT_HLS_MISSING", "Keep external-memory interface as explicit HLS boundary unless scope is revised."],
        ["InclusiveCache (L2)", "DigitalTop.l2", "SoC-level inclusive L2 and TileLink coherence", "No corresponding HLS module", "REFERENCE_OUT_OF_SCOPE", "Outside current core HLS boundary; not a new requirement absent user scope expansion."],
    ]
    write_csv(OUT / "rtl_to_hls_module_mapping.csv",
              ["reference_rtl_module", "reference_instance_path", "reference_responsibility", "hls_cpp_mapping", "mapping_status", "notes"], mapping)

    gaps = [
        ["RV64 integer architectural state/instructions", "BoomCore/ROB/rename/execute/commit", "decode/rename/execute/ROB/commit C++", "Architectural behavior", "REFERENCE_PRESENT_HLS_PARTIAL", "Reference-derived instruction streams; compare retirement PC/instruction/register/memory/trap records, not HLS self-test alone."],
        ["Fetch/redirect/FTQ", "BoomFrontend + FTQ", "frontend.cpp + ftq.cpp + branch.cpp", "Architecture + microarchitectural state/timing", "REFERENCE_PRESENT_HLS_PARTIAL", "Differentially compare fetch packets, redirect owner, FTQ allocation/generation/retirement state and cycles."],
        ["Branch prediction", "BranchPredictor + BoomRAS", "predictor.cpp + frontend.cpp", "Microarchitectural state/timing", "REFERENCE_PRESENT_HLS_PARTIAL", "Compare predictor-visible predictions and updates when configured state is observable; no requirement to reproduce unrelated predictor internals unless timing/state equivalence is in scope."],
        ["Load/store and response ownership", "LSU + DCache + TileLink", "lsu.cpp + commit.cpp + completion.cpp + external stream ports", "Transaction protocol + architectural memory behavior", "REFERENCE_PRESENT_HLS_PARTIAL", "Compare request/response ordering, IDs, masks, retries/faults, forwarding, committed stores and final memory."],
        ["FPU arithmetic and FP issue", "FPU + FP IssueUnit + FP register path", "FP lane disabled; pass-through stub observed", "Architectural behavior + transaction/issue timing", "REFERENCE_PRESENT_HLS_MISSING", "Required if SmallBoomConfig reference FP instructions are in scope; do not infer support from HAS_FPU/FP_DEPTH macros."],
        ["ICache/DCache implementation", "ICache + BoomNonBlockingDCache", "External IMEM/DMEM stream interfaces", "Transaction protocol + microarchitectural state/timing", "REFERENCE_PRESENT_HLS_MISSING", "Reference cache behavior is absent from HLS implementation boundary; explicitly decide if equivalence target includes caches or abstracts memory at tile boundary."],
        ["MMU/TLB/PTW", "TLB + PTW + CSR translation state", "CSR satp/priv fields only; no translation/page-walk machinery located", "Architectural behavior + transaction protocol", "REFERENCE_PRESENT_HLS_MISSING", "Use virtual-memory enabled reference tests and compare translation/fault/page-table traffic if included."],
        ["CSR/privilege/traps", "CSRFile", "csr.cpp + reset.cpp", "Architectural behavior", "REFERENCE_PRESENT_HLS_PARTIAL", "Reference-directed CSR read/write, privilege transitions, interrupts, traps, return and delegation tests needed."],
        ["AMO/LRSC", "BoomNonBlockingDCache.lrsc_count/lrsc_addr/AMOALU; TLAtomicAutomata", "No established HLS AMO/LRSC path", "Transaction protocol + architectural atomic behavior", "REFERENCE_PRESENT_HLS_MISSING", "Test LR reservation, SC success/failure, AMO atomicity and external protocol only if reference paths are in-scope."],
        ["L2/cache coherence fabric", "DigitalTop.InclusiveCache and TileLink network", "No HLS SoC/L2 module", "Transaction protocol + microarchitectural state/timing", "REFERENCE_OUT_OF_SCOPE", "Current HLS component is a core boundary with external streams. Do not expand product requirements absent explicit scope change."],
        ["SmallBoomConfig reference identity", "Generated SmallBoomConfig files", "HLS build defaults/params and source", "Configuration/provenance gate", "UNRESOLVED", "Extract effective reference parameters and establish exact HLS parameters/source baseline before interpreting any differential result."],
    ]
    write_csv(OUT / "functional_gap_matrix.csv",
              ["behavior", "reference_evidence", "hls_evidence", "equivalence_kind", "status", "verification_gap"], gaps)

    hierarchy = f"""# SmallBoomConfig Reference RTL Hierarchy

## Scope and Actual Tops

Reference directory: `{REF}`.

- Generated system FIRRTL circuit: `TestHarness` in `{FIR}`.
- Generated core/system Verilog top module: `ChipTop` in `{RTL}` (module declaration line 357645); it instantiates `DigitalTop` in the FIRRTL source.
- Simulator/harness top: `TestHarness` in `{HARNESS}` (module declaration line 27549); it instantiates `ChipTop` as `chiptop`.
- The harness-level `top.fir` records `circuit TestHarness` and an `extmodule ChipTop`; the full system FIRRTL `{FIR}` contains the implementation of `ChipTop` and the BOOM hierarchy.

The actual Verilator target selected by this generated package is `TestHarness`; `ChipTop` is the generated chip/system boundary. The reference's generated core configuration records 1-wide commit, 3-wide issue, ROB 32, LQ/SQ `8/8`, FPU and VM enabled, ICache/DCache geometry, and TLB sizes in `chipyard.TestHarness.SmallBoomConfig.core.config` (lines 37-98). Thus HLS `8_8` is the primary parameter match; other LQ/SQ depths are extension configurations. None of these configuration values alone prove HLS functional equivalence.

## Relevant Hierarchy

```text
TestHarness (harness.v:27549; top.fir module TestHarness:8821)
└── chiptop: ChipTop (top.fir:8826; top.v:357645)
    └── system: DigitalTop (full fir:394326)
        ├── subsystem_l2_wrapper: CoherenceManagerWrapper (full fir:393046)
        │   └── l2: InclusiveCache (full fir:136595)
        └── TileResetDomain.boom_tile: BoomTile (full fir:297527)
            ├── core: BoomCore (full fir:297376)
            │   ├── mem_issue_unit / int_issue_unit (full fir:270057-270060)
            │   ├── rob: Rob (full fir:270085)
            │   ├── csr: CSRFile (full fir:270843)
            │   └── FP execution pipeline: FP issue unit, FPU execution unit, FPU
            ├── frontend: BoomFrontend (full fir:297262)
            │   ├── icache: ICache (full fir:197128)
            │   ├── bpd: BranchPredictor (full fir:197139)
            │   ├── ras: BoomRAS (full fir:197143)
            │   └── tlb: TLB (full fir:197147)
            ├── lsu: LSU (full fir:297379)
            ├── dcache: BoomNonBlockingDCache (full fir:297255)
            └── ptw: PTW (full fir:297498)
```

## Evidence Notes

- `BoomTile` explicitly wires core LSU to `lsu.io.core`, PTW requestors from LSU/frontend/core, LSU to DCache, and DCache/ICache TileLink masters to the tile interconnect (`{FIR}:297391-297392`, `297498-297510`).
- The full FIRRTL hierarchy has real `inst` declarations. The Verilog file confirms corresponding synthesized module definitions: `ICache` 184929, `BranchPredictor` 204066, `TLB` 205238, `FPU` 229167, `Rob` 260741, `CSRFile` 273427, `BoomCore` 275666, `LSU` 283681, `PTW` 296125, `BoomTile` 297690, `ChipTop` 357645.
- `L2` exists at the DigitalTop/configuration level, not as an HLS core module.
- Feature inventory evidence is in `reference_feature_inventory.csv`. HLS mappings and gap classes are in `rtl_to_hls_module_mapping.csv` and `functional_gap_matrix.csv`.
"""
    (OUT / "reference_hierarchy.md").write_text(hierarchy, encoding="utf-8")

    plan = """# BOOM HLS Reference RTL Equivalence Verification Plan

## Reference and Configuration Policy

The only primary reference is the generated design in `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/`. Its generated FIRRTL/Verilog and hashes are recorded in `reference_source_manifest.csv`. SmallBoomConfig's effective default is the primary target; establish its actual LQ/SQ and other effective parameters from generated RTL/FIRRTL before comparison. `8_8` is the main HLS configuration when confirmed against that reference. Other LQ/SQ depths (`4_4`, `16_16`, `4_16`, `16_4`) are parameterized HLS extension runs and do not replace or redefine the reference.

`PPA_GATE_STATUS`, HLS unit tests, RTL self-tests, and prior capacity reports do not establish reference equivalence. A PASS requires matched reference-derived input traces and an independent Verilog-vs-HLS differential result.

## Equivalence Classes

| Class | Required correspondence | Examples | Comparison |
| --- | --- | --- | --- |
| Architectural behavior | Same committed architectural state and externally visible outcomes for same instruction/environment stream | integer/FP registers, memory, exceptions, CSR/privilege, branch redirect architectural result | Retirement stream by PC/instruction/rd/value, memory transactions/results, final memory, trap/interrupt/CSR state |
| Transaction protocol | Same legal externally visible request/response contract at a declared abstraction boundary | fetch, load/store, AMO if in scope, TileLink if tile/SoC boundary included | Handshake cycle or latency window, address/opcode/size/mask/data/ID, response ownership/order, retry/fault, reset behavior |
| Microarchitectural state/timing | Corresponding internal state transitions when the implementation claims those structures | ROB, issue queues, FTQ, predictor, LQ/SQ, cache/TLB if in scope | Mapped state snapshots at agreed sample points plus cycle-level transition/occupancy traces; do not compare unrelated encoding bits |

Architectural equality is mandatory for behaviors supported by both sides. Transaction protocol equality is mandatory at the agreed interface boundary. Microarchitectural state/timing equality applies only to structures explicitly mapped as corresponding; state encoding differences require semantic mapping, not bitwise assumptions.

## Differential Harness Route

1. Freeze the manifest hashes and effective SmallBoomConfig parameters. Build a machine-readable reference module/instance path map from FIRRTL `inst`/connections and generated Verilog module declarations.
2. Choose the boundary explicitly. Initially drive the same cycle-indexed instruction-memory responses, data-memory responses, interrupts, reset and debug inputs into the reference `TestHarness/ChipTop` path and the HLS core's external IMEM/DMEM stream boundary. Add adapters only in the verification harness; do not alter product C++.
3. Use the same directed and constrained-random RV64 instruction streams. Include integer/control-flow/load-store, mispredict/recovery, CSR/trap/interrupt, FP, virtual memory, and AMO/LRSC only when the reference inventory confirms the behavior is actually instantiated and connected.
4. Capture reference Verilog commit/trace signals and HLS `commit_trace_out`; normalize fields (PC, instruction, destination register/value, memory op/address/data/mask, exception/privilege) into a canonical event schema.
5. Compare ordered architectural events and final state. Compare transaction traces at the chosen boundary with transaction IDs mapped by request owner, not presumed equal internal IDs.
6. Add semantic microarchitecture probes only where there is a one-to-one structure mapping: ROB occupancy/allocation/retire, LQ/SQ ownership, FTQ identity/generation and predictor request/update. Record cycle offset and allowed latency windows; strict cycle equality is a separate result.
7. Run negative controls: perturb one expected register, memory response, exception, transaction ID owner, and redirect result in the checker inputs. The differential checker must fail each mutation.
8. Report per-feature PASS/PARTIAL/FAIL/NOT_IN_REFERENCE with trace hashes, simulator/tool versions, source hashes and exact test seeds. Never promote HLS self-test PASS to reference-equivalence PASS.

## Scope Rules

- `reference_feature_inventory.csv` is the authority for existence, based on RTL module definitions plus instance/connection/state/interface evidence.
- A macro, parameter declaration, module name with no reachable instance, or HLS test result alone is insufficient evidence of a feature.
- L2 and SoC/TestHarness integration are outside the current HLS core boundary; retain as `REFERENCE_OUT_OF_SCOPE` unless the requested equivalence boundary is explicitly expanded.
- Where the reference has no feature, mark `REFERENCE_ABSENT` and do not create a new HLS requirement. Where evidence is incomplete, retain `UNRESOLVED`.
- This audit makes no product-source changes and does not claim equivalence PASS.

## Exit Criteria

1. Reference manifest and top/hierarchy are reviewed and immutable for the comparison run.
2. The effective HLS `8_8` parameters and build source hash are tied to SmallBoomConfig's actual reference parameters.
3. Every in-scope feature has a reproducible differential test set, normalized traces, checker results and negative-control PASS.
4. Architectural and transaction mismatches are zero for the selected suite; microarchitectural results are reported separately from architectural PASS.
5. Each remaining unsupported feature is explicitly classified as partial, missing, absent or out of scope. No gate-wide signoff flag is inferred from partial evidence.
"""
    (OUT / "equivalence_verification_plan.md").write_text(plan, encoding="utf-8")


if __name__ == "__main__":
    main()
