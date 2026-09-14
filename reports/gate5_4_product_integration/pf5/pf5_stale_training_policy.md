# Stale Training Policy

Commit first validates FTQ allocation generation, lane liveness, and conditional
CFI identity through `lookup_prediction`. It then requires the retained
predictor generation to equal the active product predictor generation and the
retained metadata index to equal `(branch_pc >> 1) & 255`. Any mismatch emits no
predictor update. A reused FTQ slot therefore cannot be trained by an old ROB
reference.
