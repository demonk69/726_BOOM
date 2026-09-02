# BIM Reset Policy

The selected policy is `LAZY_VALID_INIT`:

- Reset clears the separate valid bitmap and pending-response control.
- Counter payload is not reset.
- An invalid entry reads as logical `01` (weak not-taken).
- Its first qualified update starts from logical `01`, writes the new counter, and marks the entry valid.

The full-payload experiment writes `01` to every counter and has equivalent logical behavior, but costs 678 LUT versus 658 LUT for 256-entry AUTO. Lazy reset is selected. In the HLS top transaction, reset executes a 256-iteration loop to clear validity; at the C++ architectural boundary, reset remains one `PredictorFoundation::step` call. Neither statement implies one physical RTL clock.
