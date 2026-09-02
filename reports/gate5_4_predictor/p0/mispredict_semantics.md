# Mispredict Semantics

Today there is no predictor, so current redirects must not all be called prediction failures. `AluResult.mispredict` is actually the taken outcome under implicit static-not-taken (`src/execute.cpp:155-162`). Future semantics are:

| Type | Required prediction fields | Actual resolution fields | Redirect behavior | FTQ recovery | Predictor update |
|---|---|---|---|---|---|
| `DIRECTION_MISPREDICT` | valid,taken,cfi_lane | actual_taken,fall-through/static target | redirect to actual target if taken else fall-through | retain owner; kill younger lane/entries | train committed conditional direction |
| `TARGET_MISPREDICT` | valid,taken,target_valid,target,cfi_lane | actual_taken,actual_target | redirect to actual target | retain owner; kill younger | update future target structure; no such structure in foundation |
| `CFI_SLOT_MISPREDICT` | valid,cfi_lane,cfi_type | resolved CFI lane/type | redirect to correct control-flow PC | recover from correct owner boundary; invalidate wrong younger path | update slot/target structure when one exists |
| `RETURN_MISPREDICT` | valid,target,cfi_type=RETURN | actual_target and return classification | redirect to actual return target | retain owner; kill younger; repair RAS snapshot when implemented | update/repair RAS or target structure |
| `NO_PREDICTION_REDIRECT` | prediction_valid=false | actual_taken,actual_target | redirect on taken JALR or unsupported CFI | ordinary execute recovery, not a mispredict statistic | no foundation direction training except committed conditional lookup policy |

A correct taken prediction is not a redirect at Execute and not a mispredict. A predicted-taken/actual-not-taken branch redirects to fall-through. An unconditional direct JAL handled by static predecode is a prediction only if the contract records it as valid; otherwise it is a frontend static redirect, never a direction-table success.

Classification is mutually exclusive with this precedence: CFI slot/type mismatch first; conditional direction mismatch second; when both predict and resolve taken, target mismatch is `RETURN_MISPREDICT` for a selected `RETURN` subtype and otherwise `TARGET_MISPREDICT`; no valid prediction is `NO_PREDICTION_REDIRECT`. One resolution increments at most one mispredict class.
