# L1 Parameterization

- Canonical parameters: `LQ_DEPTH`, `SQ_DEPTH`; defaults 8/8
- Supported independently: 4, 8, 16
- Index bits: 4->2, 8->3, 16->4
- Count bits: 4->3, 8->4, 16->5
- Validation: C++11 recursive `constexpr boom_clog2` and static assertions
- HLS state types: derived-width `ap_uint`; native state types use safe `uint8_t`
- Five native and Vitis CSim configurations passed
