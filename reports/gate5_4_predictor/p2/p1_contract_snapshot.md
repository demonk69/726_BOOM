# Frozen P1 Predecode Contract

```text
GATE5_4_P1_CFI_PREDECODE_VERIFIED=true
PREDECODE_STATEFUL=false
PREDECODE_REQUEST_STAGE=PARCEL_PREDECODE_STAGE
FIRST_CFI_POLICY=EARLIEST_VALID_CFI_IN_PC_ORDER
SUPPORTED_CFI_TYPES=CFI_NONE,CFI_CONDITIONAL_BRANCH,CFI_JAL,CFI_JALR
STATIC_BRANCH_TARGET_SUPPORTED=true
STATIC_JAL_TARGET_SUPPORTED=true
STATIC_JALR_TARGET_SUPPORTED=false
```

P2 consumes complete canonical instructions through `predecode_cfi`. RV64C composition uses the canonical `decompress_rvc` output rather than a duplicate decoder. The P1 rerun remains zero-error: directed 994, RVC exhaustive 65,536, packet 256 x 4,096, and random 1,000,000.
