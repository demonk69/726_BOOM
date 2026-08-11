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

R1 does not instantiate the decompressor in Frontend. There is no PC+2 update,
halfword cursor, carry parcel, cross-word assembly, mixed-stream sequencing,
Fetch Buffer, or Decode/backend interface change. Those belong to R2.
