# Product Redirect Priority

Prediction-directed next PC is normal speculative selection, not recovery. Unified future priority is:

1. runtime reset
2. architectural exception/interrupt redirect
3. validated branch mispredict or no-prediction recovery redirect
4. generic/local flush redirect
5. prediction-directed next PC
6. sequential/fallthrough PC

Higher priority cancels pending prediction and packet admission in the same step. Architectural and branch recovery must arbitrate before either mutates shared Frontend/FTQ state; the current split behavior, where branch recovery mutates Frontend before Frontend selects an architectural redirect, must be consolidated in a recovery gate. A prediction never performs ROB/rename rollback and never overrides a recovery redirect.
