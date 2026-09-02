# P2 Predictor Contract Snapshot

P2 is a verified standalone 256-entry, 2-bit BIM using
`index=(pc>>1)&255`, LUTRAM, lazy-valid initialization, commit-qualified
conditional updates, generation validation, and metadata-index validation.
Conditional predictions are valid, JAL is statically taken, and JALR/no-CFI
responses have `prediction_valid=false`. The logical request/response latency
is one step and blocking; this is distinct from the Vitis wrapper latency 3,
II 4 reported by P2.
