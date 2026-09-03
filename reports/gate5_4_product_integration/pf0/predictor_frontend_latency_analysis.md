# Predictor Frontend Latency Analysis

```text
PRODUCT_PREDICTOR_WAIT_STATE_REQUIRED=true
```

The state is required for packets whose selected earliest CFI is conditional. It holds the canonical packet and exact predecode/request identity from accepted request N until matching response N+1. P2 response hold permits direct atomic consumption when FB and FTQ are ready; no extra response payload latch is required initially.

## Best-Case Cycle Model

Current packet construction in step N normally becomes Fetch Buffer enqueue-eligible in step N+1. A conditional request can be accepted in N and its P2 response can participate in the same N+1 enqueue fire. Thus the protocol wait is one logical step, while best-case admission delta versus the current staged frontend is zero. The throughput cost is real: the next IMEM request that current Frontend could issue in N must be delayed until the prediction-selected path is known.

| Packet scenario | P2 wait | Added best-case admission steps vs current | Next-request effect |
|---|---:|---:|---|
| no CFI | 0 | 0 | none |
| conditional lane 0 | 1 | 0 | delay one step; taken masks lane 1 |
| conditional lane 1 | 1 | 0 | delay one step |
| JAL | 0 | 0 | static target selects next request |
| JALR | 0 | 0 | fallthrough request; Execute later redirects |
| all-RVC/all-32/mixed | determined by selected CFI | 0 ready case | instruction width alone adds no wait |
| cross-word completion + RVC | determined after complete packet | 0 ready case | carry-only step remains unchanged |
| FB or FTQ blocked | response holds until both ready | variable | no younger request while conditional response owns path |
| redirect/reset pending | response/request is drained or invalidated | n/a | higher-priority path wins |

The HLS wrapper's latency 3 and II 4 are excluded from this model.

## Cancellation

Runtime reset, architectural exception redirect, branch recovery, and generic flush dominate predictor response, clear pending packet/request context, and change the relevant stale identity. A late response is consumed without changing PC, packet mask, FB, FTQ, or BIM state.
