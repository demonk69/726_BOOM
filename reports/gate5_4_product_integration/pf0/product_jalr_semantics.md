# Product JALR Semantics

Foundation JALR is `NO_TARGET_PREDICTION`, not a predictor miss and not a direction prediction. BTB and RAS are absent, so `prediction_valid=false`, `target_valid=false`, and execution continues at JALR fallthrough until Execute computes `(rs1 + imm) & ~1`.

An actually executed JALR then causes `NO_TARGET_PREDICTION` recovery to the actual target. It is not classified as `DIRECTION_MISPREDICT`; there was no valid direction/target prediction to compare. The FTQ records selected CFI lane/type and invalid prediction/target fields so recovery and future statistics remain explicit.

For lane-0 JALR, lane 1 remains admitted on the fallthrough path. It is younger and must be killed when Execute redirects. Future BTB/RAS support may turn this into normal target prediction, but is outside PF0.
