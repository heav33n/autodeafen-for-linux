# AutoDeafen for Linux

## Intent
Deafen Discord at a chosen % of a Geometry Dash level and undeafen afterwards.
The owner plays through Proton on Wayland, where mods that fake Discord's
keybind (Mega Hack's AutoDeafen) do nothing. It should be shareable, so it must
not need a Discord client plugin.

## Goals
- Deafen at the set %; undeafen on death, completion, restart and quit.
- Optional undeafen %: one slider with a start and an end handle (added 2026-11-24).
- Per-level % and on/off from a pause menu button; defaults in mod settings.
  Practice mode off by default.
- If the player was already deafened, leave them deafened at the end.
- One-time setup: user's own Discord app ID and secret, then Authorize.
- Discord closed or not set up: GD plays normally, pause menu says "Disconnected".

## Non-goals
Windows, Mac, multiplayer, keybind simulation, Vesktop.

## Anti-goals
- Stuttering the game. All socket and web work stays off the main thread.
- Leaving someone deafened after a run, or undeafening someone who chose it.
- Spoofing StreamKit's client ID or origin.
- Logging the secret or tokens.
- Copying code from Lynxdeer AutoDeafen (no license) or AutoDeafen-Linux (GPL-3.0).

## Constraints
- From scratch (user's decision after weighing two forks), so it can be published
  on the owner's GitHub under its own license. Builder's call: MIT.
- GD 2.2091, Geode 6.0.0-beta.3, cross-compiled like better-hitboxes.
- Discord allows unapproved RPC apps only for owner and testers: one app per user.
- AI-written and labelled so in the README. Not submitted to the Geode index,
  whose reject-vibecoded rule likely excludes it.
- Publishing to GitHub needs the owner's yes at that time.

## Approaches
1. Winsock AF_UNIX from the mod. PICKED (the test passed on the owner's Proton).
2. Linux helper for Valve Proton. Agreed second step, before wider use.
3. Discord's WebSocket port. Rejected: deprecated and checks origin.
4. Equicord plugin. Rejected by the user: no Discord plugins.
5. Fork Lynxdeer/AutoDeafen (unlicensed, declines Linux) or AutoDeafen-Linux
   (GPL, plugin-based). Both considered, then dropped by the user.

### Test results, 2026-11-24 (throwaway; handshake only, host prefix, not in GD)
| Runtime | Winsock AF_UNIX |
|---|---|
| proton-cachyos-slr 11 (owner's) | works, Discord answered READY |
| proton-ge-bin (wine 11) | works |
| Valve Proton 9, 10, 11, Experimental, Hotfix | error 10047, unsupported |
| proton-cachyos 10, system wine 11.22 | error 10047, unsupported |

Raw Linux system calls were blocked on both builds tried (proton-cachyos 11 and
system wine), so approach 1 now means Winsock AF_UNIX. Launching a Linux script
from Wine worked on those two, so approach 2 is open for the other runtimes.

## Decision this needs
Step 1: Winsock AF_UNIX only. Covers the owner.
Step 2, before public release: helper used only when AF_UNIX returns 10047.

## Left to the builder
Popup layout, token refresh, socket search (discord-ipc-0..9 and Flatpak path).
Check hooks beside Mega Hack, which now loads as a Geode mod.

## Must check live (docs describe it, not yet observed)
- Deafening as the user's own app, with rpc + rpc.voice.read/write scopes.
- Whether the player can still use Discord's own deafen button while connected.
- Docs say settings reset when the controlling app disconnects. If so, a GD
  crash undeafens by itself. Decides whether to stay connected between runs.
