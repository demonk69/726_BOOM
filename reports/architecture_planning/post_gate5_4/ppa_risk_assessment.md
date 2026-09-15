# Full LSU PPA Risk Assessment

The +10% review threshold (234780 LUT) and +25% blocker (266795 LUT) are cumulative limits for all of Gate6, not allowances repeated at every subgate. L1 is register/control dominated. L2 and L4 carry the largest LUT/timing risk because parallel address and byte-mask comparisons plus youngest-match selection form CAM and priority-mux cones. L3 adds identity routing. L5 has no feature allowance.

Small 4/4 and 8/8 queues should remain registers or distributed storage. At 16/16, data payloads may use LUTRAM or a separately banked RAM, but searchable address, age, valid, and mask metadata must remain parallel-accessible; a monolithic single-port BRAM is unsuitable. No BRAM increase is expected or budgeted through L4 without explicit review.
