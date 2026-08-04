# Remote Control (Phone App) — Design & Safety Rules

AEZEL supports phone-triggered actions over the existing BLE command
channel (`BleManager` → `RemoteControlManager`). This doc explains what's
implemented, what it needs physically, and — most importantly — why each
command is designed the way it is. Read this before flipping any
`ENABLE_REMOTE_*` flag in `Config.h`.

## The one rule everything else follows

**`BleManager` never touches a GPIO. `RemoteControlManager` is the only
code allowed to actuate anything a phone command reaches.** That's not
just tidiness — it means every interlock lives in exactly one file, so
"can the app really do that right now" has one place to audit instead of
being scattered through a JSON parser.

## Commands, by risk tier

### Tier A — Low risk, on by default once wired (`ENABLE_REMOTE_HORN`, `ENABLE_REMOTE_INDICATORS`)

| Command | What it does | Interlock |
|---|---|---|
| `horn_on` / `horn_off` | Sounds horn via relay | None — same risk as a car's panic-alarm horn |
| `hazard_on` / `hazard_off` | Flashes both indicators | None — "find my bike" use case |
| `indicator_left/right_on/off` | Flashes one side | **Refused if speed ≥ 1 km/h.** Turn signals belong to the physical switch the instant the bike is moving — a phone can never become a second, conflicting source of truth for what the following traffic sees. |
| `find_bike` | Horn + hazard together | Same as above, combined |

These need exactly one relay each (horn) or two (indicators) wired in
parallel with the stock circuits — see `docs/wiring.md`. Low consequence
if something glitches: worst case is a horn honks or a light flashes when
you didn't mean it to.

### Tier B — Locks the bike (`ENABLE_REMOTE_IMMOBILIZER`)

| Command | What it does | Interlock |
|---|---|---|
| `lock` | Opens the ignition-enable relay (cuts spark) | **Refused unless `!engineRunning && speed < 1km/h`.** Locking a moving or running bike is the actual danger case for any remote immobilizer — refused unconditionally, not just discouraged in the app UI. |
| `unlock` | Closes the relay | Always allowed — see fail-safe note below |

**Fail-safe requirement:** wire the immobilizer relay **normally-closed**
in the ignition-enable circuit, and drive the control pin **LOW = unlocked
(relay energized), HIGH = locked**. That way a dead battery, a firmware
crash, or an ESP32 that never boots leaves the bike *startable*, never
stranded. A remote-lock feature whose failure mode is "rider can't start
their own bike" is a feature nobody should ship. `RemoteControlManager::begin()`
explicitly drives the pin LOW on every boot for this reason.

### Tier C — Starts the engine (`ENABLE_REMOTE_STARTER`) — off by default, read this fully before enabling

This is the one that can actually hurt someone if built carelessly: an
engine that starts itself while unattended, in gear, or with the stand up.
`remoteStart()` refuses unless **all** of the following hold, checked at
the moment of the command:

- Engine not already running
- Transmission in **neutral** (`inNeutral`)
- Side stand **down** (`inSideStand`) — this is the actual safety
  mechanism: even if something bumps the bike into gear after starting,
  the side stand being down means the bike can't move under its own power
- Kill switch not engaged

Once started, `RemoteControlManager::tick()` re-checks all four **every
200ms** for as long as the remote-started engine is running, plus a hard
5-minute unattended-runtime cap (`MAX_UNATTENDED_RUN_MS`). Any interlock
breaking — gear engaged, stand raised, motion detected, kill switch — cuts
the starter/ignition immediately, no confirmation round-trip to the phone
required (that round-trip is exactly the kind of delay that turns a
one-second problem into a real one).

**What this build does NOT do, and what you'd need to add before trusting
it further:**
- It doesn't cut *spark* on interlock failure, only releases the starter
  relay — see the `cutRemoteStart()` comment. Wire the immobilizer relay
  from Tier B into this path too so a broken interlock kills ignition, not
  just the starter motor, before relying on this outdoors.
- It has no confirmation step in the app before sending `remote_start` —
  a production app should require a deliberate two-step gesture (not a
  single tap) given what's being triggered.
- It doesn't verify the bike is actually upright/stationary via IMU in
  addition to the side-stand switch — a stuck or miswired side-stand
  switch is a single point of failure. Cross-checking `leanAngleDeg` near
  vertical before allowing start is a reasonable Phase 4 hardening step.

If you're not planning to leave the bike unattended and remote-start it
from indoors, you likely don't need this tier at all — Tier A/B cover
"where's my bike" and "lock it remotely," which is most of what people
actually want from a phone-controlled dashboard.

## What the phone app sees back

Every command result — success or the specific interlock that blocked it
— rides back in the next BLE telemetry packet as `cmd_result`
(`"ok"`, `"rejected_moving"`, `"rejected_not_neutral"`, etc. — see
`RemoteControlManager::lastResultString()`). Silently ignoring a refused
command is worse than telling the rider exactly why it didn't happen.

## Build order

1. Horn + hazard (Tier A) — cheap, low-risk, immediately useful
2. Immobilizer (Tier B) — needs the relay wired per the fail-safe note
   above; test the "dead ESP32 still lets you start the bike" case
   explicitly before trusting it
3. Remote start (Tier C) — only if you actually want it, and only after
   reading the gaps listed above and deciding how you'll close them for
   your specific build
