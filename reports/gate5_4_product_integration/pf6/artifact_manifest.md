# PF6 Artifact Manifest

All files in this PF6 report directory except `durable_artifact_hashes.csv` are
`DURABLE_ACCEPTED_EVIDENCE`; the hash inventory deliberately does not hash itself.
All Vitis/Vivado/XSim projects, generated RTL bytes, raw reports, binaries, and
logs under `/tmp/boom_hls/pf6` are `EPHEMERAL_BUILD_PROVENANCE` and are not
required at commit. `generated_rtl_hashes.csv` preserves stable content identity.

Text reports are LF, have no intentional trailing spaces, and are reviewed by
`git diff --check`. Raw tool logs were not normalized or copied into durable scope.
