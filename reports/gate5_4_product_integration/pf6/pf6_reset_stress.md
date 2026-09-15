# PF6 Reset Stress

`PF6_RESET_STRESS_PASS checks=13 failures=0 scenarios=4`.

Covered pending predictor request, occupied FTQ with old reference, resolved branch
before Commit, and pending eligible training. Reset advanced predictor generation,
made the old FTQ reference stale, removed speculative ROB state, cleared pending
training, and left the BIM entry lazy-invalid.
