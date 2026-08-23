# Phone Link — Calls, Messages, Music, Nav Relay, Quiet Mode

Four things bundled together because they're all the same underlying
pattern: the companion app has information the dashboard doesn't (calls,
texts, what's playing, the next turn from Maps/Waze), and the dashboard
can show it without you touching your phone. `PhoneLinkManager` is the
firmware-side half of this.

## The honesty part, up front

**This firmware cannot read your phone's calls, SMS, WhatsApp messages, or
music by itself.** ESP32 has no access to Android/iOS notifications or
media sessions — that requires a companion app with real OS-level
permissions (Android's `NotificationListenerService`, media session APIs,
`READ_PHONE_STATE`, etc.). That companion app is a separate project, not
built here. Everything in this doc describes the JSON contract that app
needs to speak over the existing BLE command channel — the firmware side
(receiving it, displaying it, sending button presses back) is what's
actually implemented in this repo.

If you don't have that companion app yet, none of these four widgets will
ever show anything — they're not broken, they're just waiting for data
nobody's sending. `arm_security`/`mark_serviced`/etc. from earlier docs
work standalone because the firmware itself decides when to send those;
phone-link is different by nature, it's mirroring, not generating.

## Commands the companion app sends TO the dashboard

```json
{"cmd": "incoming_call", "caller": "Mom"}
{"cmd": "call_ended"}
{"cmd": "push_message", "app": "WhatsApp", "sender": "Amit", "preview": "On my way"}
{"cmd": "music_update", "title": "...", "artist": "...", "playing": true}
{"cmd": "nav_update", "instruction": "Turn right", "distance_m": 200}
{"cmd": "nav_end"}
```

- `push_message` is routed straight into the existing `NotificationManager`
  queue (title = `"<app>: <sender>"`, body = preview, INFO priority) — it
  already had exactly the right shape (title/body/priority, shown in a
  list, tap to dismiss) for a message preview, so no new UI was needed for
  that one specifically.
- `incoming_call`/`music_update`/`nav_update` go to `PhoneLinkManager`,
  which the dashboard's call modal / music widget / nav banner read from.
- `music_update` and `nav_update` should be sent periodically (or on every
  change) while active — see "Staleness" below for why a one-time push
  isn't enough.

## What the dashboard sends BACK to the phone

Only for the two things that need a response — the app is expected to
actually act on these (answer/reject the call, control playback):

```json
{"phone_action": "call_accept"}
{"phone_action": "call_reject"}
{"phone_action": "music_play_pause"}
{"phone_action": "music_next"}
```

This rides in the normal telemetry notify packet (same mechanism as
`cmd_result` in `docs/remote_control.md`), delivered **once** — pressing
Accept sets a pending action, the next telemetry tick (≤500ms later)
includes it, and it's cleared immediately after so the app doesn't see the
same action resent every tick. The app needs to actually place the call
accept/reject and media control calls on the phone's OS — the dashboard
button press is a request, not the action itself.

There's no `music_prev`/skip-back button on the dashboard — three small
touch targets in the music widget's footprint was already tight for a
gloved hand, and play-pause/skip covers the actual "change what's playing
without grabbing the phone" need. `PhoneLinkManager::requestMusicPrev()`
exists in the API for a companion app that wants to offer it in its own
UI, it's just not wired to a physical button here.

## Staleness — why widgets hide themselves

Every phone-link widget has a freshness window (`PHONE_LINK_STALE_MS`,
15s default; calls get `CALL_STALE_MS`, 90s, since a lost `call_ended`
message would otherwise leave the modal on screen indefinitely).
`DisplayPolicyMath::isDataFresh()` — pure, unit-tested — decides this. If
the phone disconnects mid-ride or the companion app stops sending updates,
the nav banner and music widget disappear on their own rather than showing
stale, possibly-wrong information. This means **the companion app needs to
keep sending `music_update`/`nav_update` periodically while that state is
active**, not just once when it starts — a single push and then silence
will make the widget vanish after 15 seconds even though the song is still
playing.

## The incoming call modal

Deliberately not part of the swipe/MODE-button screen cycle
(`docs/screen_flow.md`) — a call isn't a screen you navigate to, it's an
interrupt. It's drawn on LVGL's top layer (`lv_layer_top()`), so it
appears over whatever screen was active underneath without any of the
four screens needing to know it exists. Reject ends the call locally
immediately (no need to wait for phone confirmation); Accept just dismisses
the modal and waits for the phone to send `call_ended` when the actual
call (probably over a Bluetooth headset) finishes.

## Quiet mode — the one item here that ISN'T phone-mirrored

Unlike the other three, quiet mode needs no companion app and no BLE data
— it's a pure firmware behavior change: non-critical (INFO/WARNING)
notification banners on the Main Dashboard suppress themselves above
`QUIET_MODE_SPEED_THRESHOLD_KMH` (20 km/h default), so a "Chain Lube Due"
banner doesn't pop up while you're actually riding. **CRITICAL warnings
are never suppressed, at any speed** — `DisplayPolicyMath::shouldSuppressBanner()`
guarantees this and is unit-tested for it explicitly. The suppressed
notification isn't deleted or acknowledged — it's still in the
Notifications screen's list, still contributing to the badge/queue, this
only hides the passive Main-Dashboard popup while you're moving.

## What's still open

- No actual companion app — this doc is the contract it needs to
  implement, not the app itself
- No way to reply to a message from the dash (display-only, matches the
  original spec's "Message Notifications" item, not "Message Replies")
- Music widget shows title/artist only, no album art (the spec listed
  "Album Art" — would need the phone to send an image, a meaningfully
  bigger BLE payload than everything else in this doc, deferred)
- No voice assistant trigger (spec item, would need the phone app to
  listen for a BLE-triggered "start listening" signal and invoke
  Siri/Google Assistant — plausible follow-up, not built)
