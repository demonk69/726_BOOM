# PF1 Frontend Redirect Contract

`rob_commit_module` runs before `frontend_module`. A real exception take sets
both `exception_commit.valid` and `frontend_redirect` in that cycle. Frontend
accepts an exception redirect only when those events agree on the target; this
permits recovery after the ROB is cleared without accepting stale fabricated
redirects.

On acceptance, Frontend advances the epoch, clears retained response/carry and
Fetch Buffer state, drains any old response, and requests the trap target. A
runtime reset has highest priority. An architectural exception redirect has
priority over branch recovery. Old-epoch responses have no architectural side
effect.
