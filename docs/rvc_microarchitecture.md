# RVC Decompressor Microarchitecture

The R1 decompressor is a stateless combinational block with one 16-bit input.
It separates parcel recognition from current-core legality and emits a
canonical 32-bit integer instruction only for supported encodings.

The public interface is:

```cpp
RvcDecodeResult decompress_rvc(uint16_t compressed);
```

`RvcDecodeResult` contains `valid`, `legal`, `instruction`, `compressed`, and
`length_bytes`. `decompress_rv64c` and `RvcDecompressResult` are compatibility
aliases; new code must use the canonical API and type.

The independent HLS wrapper is `synth_rvc_top`, with scalar outputs `valid`,
`legal`, `decompressed`, and `length`. Local helper inlining permits constant
bit-slice propagation without changing global directives.

R1 did not instantiate the decompressor in Frontend. Gate 5.2 R2 now integrates it into the one-wide Frontend, adds PC+2 progression, retained-word parcel selection, and one 16-bit cross-word carry, and verifies mixed streams in native, csim, and generated RTL. This state is not a Fetch Buffer or queue.

R2 deliberately protects 288 legal RV64C forms as illegal cause 2: one `C.EBREAK`, 256 `C.SRLI` forms with shamt[5], and 31 `C.JALR` forms. `C.JALR` is deferred because frozen Execute writes a PC+4 link rather than compressed PC+2. Full RV64C remains unverified pending R3.
