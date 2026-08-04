# M2A ap_int Portability

## Result

The public API uses standard `uint64_t`. Vitis arbitrary-precision types are selected inside `src/mul.cpp` only when `BOOM_USE_AP_INT` or `__SYNTHESIS__` is defined.

| Probe | Compiler command | Include path | Result |
|---|---|---|---|
| A | `g++ -std=c++11 /tmp/opencode/m2a_ap_int_probe.cpp` | none | FAIL as expected: `ap_int.h` not found |
| B | `g++ -std=c++11 -Iinclude /tmp/opencode/m2a_ap_int_probe.cpp` | canonical project include only | FAIL as expected: `ap_int.h` not found |
| C | `g++ -std=c++11 -Iinclude -I/home/lab_726/Xilinx/Vitis_HLS/2021.2/include /tmp/opencode/m2a_ap_int_probe.cpp` | explicit Vitis 2021.2 include | PASS, including runtime result check |

Full compiler diagnostics are in `logs/ap_int_probe.log`.

## Decision

Adding `ap_int.h` to `mul.hpp` would break every existing plain-g++ compile command. M2A therefore uses option B:

- `mul.hpp` contains only fixed standard integer types.
- dedicated M2A tests add the Vitis include path and define `BOOM_USE_AP_INT`;
- Vitis synthesis defines `__SYNTHESIS__` and uses `ap_int<128>`/`ap_uint<128>`;
- ordinary regressions use a mathematically equivalent `__int128` implementation without a new include dependency.

The successful `synth_mul_top` report, including three inferred multiply operations, confirms that synthesis exercised the arbitrary-precision branch.
