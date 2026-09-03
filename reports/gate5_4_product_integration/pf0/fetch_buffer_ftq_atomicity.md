# Fetch Buffer and FTQ Atomicity

```text
PRODUCT_FB_FTQ_ATOMIC_ALLOCATION_POLICY=ONE_NONEMPTY_FINAL_PACKET_ONE_ATOMIC_FB_ENQUEUE_AND_EXACTLY_ONE_FTQ_ALLOCATION
```

Define:

```text
packet_final_valid = bypass_packet_valid || matching_predictor_response_valid
packet_accept = packet_final_valid
             && fb_ready_for_effective_mask
             && ftq_alloc_ready
             && !higher_priority_redirect_or_reset
fb_enqueue_fire = packet_accept
ftq_alloc_fire = packet_accept
predictor_response_ready = conditional_context_valid
                         && fb_ready_for_effective_mask
                         && ftq_alloc_ready
                         && !higher_priority_redirect_or_reset
```

For a bypass packet, there is no predictor response term. For a conditional packet, P2's response-hold contract keeps the response and packet context stable until both destinations can fire. No FB enqueue may occur without the returned FTQ reference, and no FTQ allocation may occur for an unadmitted packet. Empty/carry-only responses allocate neither.

FTQ full therefore backpressures packet admission. With one outstanding IMEM transaction, the safe initial policy is to suppress new IMEM request issuance whenever a final packet or conditional prediction is pending and FTQ cannot accept; an already issued response may be captured/held but cannot overwrite state. Blocking only the enqueue while continuing unbounded request issue is illegal because there is only finite response/packet storage.
