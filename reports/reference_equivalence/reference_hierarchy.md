# SmallBoomConfig Reference RTL Hierarchy

## Scope and Actual Tops

Reference directory: `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig`.

- Generated system FIRRTL circuit: `TestHarness` in `chipyard.TestHarness.SmallBoomConfig.fir`.
- Generated core/system Verilog top module: `ChipTop` in `chipyard.TestHarness.SmallBoomConfig.top.v` (module declaration line 357645); it instantiates `DigitalTop` in the FIRRTL source.
- Simulator/harness top: `TestHarness` in `chipyard.TestHarness.SmallBoomConfig.harness.v` (module declaration line 27549); it instantiates `ChipTop` as `chiptop`.
- The harness-level `top.fir` records `circuit TestHarness` and an `extmodule ChipTop`; the full system FIRRTL `chipyard.TestHarness.SmallBoomConfig.fir` contains the implementation of `ChipTop` and the BOOM hierarchy.

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

- `BoomTile` explicitly wires core LSU to `lsu.io.core`, PTW requestors from LSU/frontend/core, LSU to DCache, and DCache/ICache TileLink masters to the tile interconnect (`chipyard.TestHarness.SmallBoomConfig.fir:297391-297392`, `297498-297510`).
- The full FIRRTL hierarchy has real `inst` declarations. The Verilog file confirms corresponding synthesized module definitions: `ICache` 184929, `BranchPredictor` 204066, `TLB` 205238, `FPU` 229167, `Rob` 260741, `CSRFile` 273427, `BoomCore` 275666, `LSU` 283681, `PTW` 296125, `BoomTile` 297690, `ChipTop` 357645.
- `L2` exists at the DigitalTop/configuration level, not as an HLS core module.
- Feature inventory evidence is in `reference_feature_inventory.csv`. HLS mappings and gap classes are in `rtl_to_hls_module_mapping.csv` and `functional_gap_matrix.csv`.
