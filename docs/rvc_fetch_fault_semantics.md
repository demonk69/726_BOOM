# RV64C Fetch Fault Semantics

- A faulting response never contributes instruction data.
- A fault for a compressed or aligned 32-bit instruction is published once at the current architectural parcel PC.
- If the first word supplied a cross-word lower half and the second word faults, one instruction-access fault is published at the saved lower-half PC. No upper data is used and the lower half is never decoded.
- Redirect/reset wins over a same-cycle fault response, clears retained word and carry, advances the existing epoch, and drains the response as stale.
- A stale identity or address mismatch cannot set word validity, complete carry, or publish a fault.
- Illegal/reserved/unsupported compressed forms use illegal-instruction cause 2, not instruction-access fault.
- R2 applies that cause-2 policy explicitly to the 288 protected legal forms: `C.EBREAK` (1), RV64 `C.SRLI shamt[5]=1` (256), and `C.JALR` (31). This is deliberate non-support pending R3, not architectural execution support.
