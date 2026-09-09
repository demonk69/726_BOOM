# PF3 Fetch Buffer and FTQ Atomicity

```text
packet_accept = packet_final_valid && fb_ready && ftq_alloc_ready
                && final_mask_nonzero && !redirect_or_reset
fb_enqueue_fire = packet_accept
ftq_alloc_fire = packet_accept
```

Frontend first presents the canonical F1 allocation only when the complete
final packet and Fetch Buffer are ready. It stamps the returned index and
generation into a local admission packet only after `alloc_accepted`; that same
packet is then enqueued. `packet_accept` requires both F1 acceptance and Fetch
Buffer enqueue fire. Full FTQ state holds the pending packet and suppresses new
IMEM issue through the existing one-outstanding-request state machine.

The allocation and initial live mask use `final_admission_mask`, not the raw
packet mask. Empty masks do not allocate. Redirect/reset has priority.
