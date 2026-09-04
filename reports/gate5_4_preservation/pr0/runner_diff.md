# Accepted Versus Current Preservation Runner Diff

The files did not drift after Gate 5.3 acceptance. The preservation selection drifted from the accepted B3I runner/test to a B2-era test with a different producer model.

| Property | Accepted Gate 5.3 | Current failing invocation |
|---|---|---|
| Runner | versioned `run_b3i_random.sh` | ad hoc/unversioned; B2 test selected |
| Test | packet-aware B3I random | scalar-producer B2 integration random |
| RNG seed constant | `0xd1b54a32d192ed03` | `0x9e3779b97f4a7c15` |
| Seeds/cycles | 256/4096 | 256/4096 |
| Producer model | pending packet, mask 1 or 3 | one old `producer_uop` only |
| Atomic two-lane admission | modeled and checked | not modeled |
| Carry/response oracle | independent packet parser | observes product scalar mirror |
| Error name | `order_error` | `ordering_error` |
| Result on `a48e527` | PASS, all zero | FAIL, `425388/174170` |
| Result on `490788d` | PASS, all zero | FAIL, `425388/174170` |

`git diff a48e527..490788d` is empty for both tests, both runner scripts, and all compiled Fetch source files. Therefore this is not file-content test drift and not source drift. It is runner/expectation selection drift across the B2-to-B3I contract boundary.
