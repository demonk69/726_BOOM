# Gate 5.3 B2 boom_core_top Kill Diagnostics

## Classification Before Retry

- The prior `boom_core_top` run ended with `rdiArgs.sh: ... Killed` during `try_issue_load` scheduling.
- This is not classified as an HLS scheduling, design, or PPA blocker.
- No Linux kernel OOM or cgroup OOM evidence was available.
- The exact SIGKILL source cannot be proven from accessible system logs.

## Host Memory

Captured 2026-08-16T00:27:54+08:00:

```text
               total        used        free      shared  buff/cache   available
Mem:            62Gi       4.0Gi        54Gi        38Mi       4.1Gi        57Gi
Swap:             0B          0B          0B
```

The host had 57 GiB available at audit time. No swap is configured.

## Filesystems

```text
Filesystem      Size  Used Avail Use% Mounted on
/dev/nvme0n1p1   46G   35G  8.4G  81% /
/dev/nvme0n1p3  892G  449G  398G  54% /home
```

The workspace is under `/home`; 398 GiB remained available.

## Kernel Evidence

- `journalctl -k --since "2026-08-13 01:20:00" --until "2026-08-13 01:40:00"`: no entries.
- Full journal query for the same interval: no entries.
- `dmesg --ctime`: denied with `Operation not permitted` for the current unprivileged user.

Therefore no accessible kernel OOM evidence identifies the prior kill.

## Cgroup Evidence

Current execution scope:

```text
/user.slice/user-1000.slice/user@1000.service/app.slice/
app-org.gnome.Terminal.slice/vte-spawn-a66de7c1-f08f-4dc5-bd73-d1bcbf035783.scope
```

Captured controls/events:

```text
memory.max: max
memory.swap.max: max
memory.current: 2038128640
memory.peak: 2181738496
memory.events:
  low 0
  high 0
  max 0
  oom 0
  oom_kill 0
  oom_group_kill 0
memory.pressure:
  some avg10=0.00 avg60=0.00 avg300=0.00 total=0
  full avg10=0.00 avg60=0.00 avg300=0.00 total=0
```

There is no cgroup memory limit and no cgroup OOM event in the current persistent terminal scope.

## Workspace Disk Audit

```text
workspace total: 4.3G
.git:             22M
build:            11M
reports:          2.3G
tb:               7.1M
scripts:          53M
```

The main report usage is historical `reports/gate4_0` at 2.0 GiB. It is curated evidence and was preserved. Gate 5.3 B2 reports occupy less than 1 MiB.

At cleanup review time there was no residual `build/gate5_3_fetch_buffer`, `xsim.dir`, or `.autopilot` directory. Consequently no additional directory was removed. Existing native binaries and all curated reports were preserved. `git clean` was not used.

## Retry Policy

Only `boom_core_top` csynth is retried. The retry is wrapped by `/usr/bin/time -v`; the final 11-top sweep and generated RTL remain blocked until this top successfully produces `boom_core_top.v`.
