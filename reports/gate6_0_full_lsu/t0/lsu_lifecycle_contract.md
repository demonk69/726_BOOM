# LSU Lifecycle Contract

Current lifecycle is execute-completion allocation, not rename allocation. A load records ROB memory metadata and an LDQ owner, then the LSU scans ROB age order. It issues only if no load is pending, no older valid `is_sta` exists, and request backpressure is clear. A valid response completes ROB/PRF and reclaims LDQ. A mismatched response is consumed and dropped without clearing the real pending owner.

A store computes address and data in one execute step, atomically fills ROB and SQ, and marks execute complete. It cannot create an external side effect until it is nonbusy ROB head and has no exception. Commit enqueues exactly one committed store request and then retires/reclaims it. External completion is request acceptance; there is no write acknowledgment.

Load/store misalignment or access fault is represented only if Execute or `DmemResponse.exception` supplies an exception. This LSU contains no translation, PMP, cache, or independent alignment checker. A load fault suppresses PRF write and traps precisely at ROB head. Older exceptions flush all younger memory state. Wrong-path stores cannot issue because only commit can emit them.
