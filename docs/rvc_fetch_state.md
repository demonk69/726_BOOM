# RV64C Fetch State

The Frontend remains one-wide with one logical outstanding IMEM request. It retains at most one aligned 32-bit response word and one 16-bit cross-word carry; neither is a queue or Fetch Buffer.

- `pc`: architectural start PC of the next instruction. It changes only when one complete instruction or one explicit fault/illegal packet enters the Frontend hold contract.
- `pending_address`: 4-byte-aligned request word address validated against the response.
- `resp_address`/`resp_instruction`: retained response base and its two little-endian parcels.
- `halfword`/`halfword_pc`: lower 16 bits and original PC of a 32-bit instruction that starts in a word's upper parcel.
- `halfword_epoch`: carry generation. Redirect/reset clears carry before any response match, so a stale or new-generation response cannot complete old carry.

At a lower parcel, a 32-bit instruction is wholly contained in the retained word. At an upper parcel, a 32-bit instruction saves carry and requests the next aligned word. A compressed upper parcel consumes the word and requests the next word. A compressed lower parcel retains the same word for the next Frontend publication.

Encodings with parcel bits `[1:0]=11` and `[4:2]=111` denote instructions longer than 32 bits. The current core publishes an explicit illegal-instruction packet and does not attempt 48/64/80+ bit assembly.

R2 also protects C.EBREAK, RV64 C.SRLI with shamt[5], and C.JALR as explicit illegal instructions. C.JALR is protected because the frozen backend's existing JALR link path uses `pc+4`; R2 does not widen its scope to change execute behavior.
