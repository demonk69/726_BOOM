# L3 Critical Path

Full-core fresh synthesis was not completed in this run.

Expected L3-sensitive structures to inspect after synthesis:

| Structure | Expected implementation |
| --- | --- |
| Response lookup | `LQ_DEPTH` parallel compare on `valid && response_pending && txid` |
| Owner validation | Selected LQ owner plus ROB tuple checks |
| Txid allocation | Bounded `LQ_DEPTH + 1` probe against active LQ txids |
| Completion mux | Existing single load-response completion slot |

`L3_FINAL_CRITICAL_PATH=NOT_MEASURED`
