# BOOM Reference Programs

These are small bare-metal RV64 programs intended for BOOM Verilator reference trace generation.

Current workspace status: the RISC-V cross compiler is missing. `scripts/build_reference_programs.sh` therefore copies hand-encoded RV64 `*.hex` loadmem images into `build/` instead of producing ELF files.

When `riscv64-unknown-elf-gcc` and `riscv64-unknown-elf-objdump` are available, run:

```sh
scripts/build_reference_programs.sh
```

Expected outputs go under `tb/programs/boom_reference/build/`. When a full RISC-V toolchain is available, ELF files are built as well.
