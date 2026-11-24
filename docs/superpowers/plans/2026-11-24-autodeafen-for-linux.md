# AutoDeafen for Linux Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A Geode mod that deafens Discord at a set % under Proton, with no keybind or Discord plugin.

**Architecture:** GD hooks feed a pure `RunTracker` that says deafen or undeafen. A worker thread owns a Winsock AF_UNIX socket to Discord and applies it through a pure `DeafenIntent`. Login is Discord's AUTHORIZE over that same socket, then a token exchange with Geode web requests.

**Tech Stack:** C++23, Geode 6.0.0-beta.3, GD 2.2091, clang cross-build copied from better-hitboxes; native clang++ tests.

**Spec:** docs/superpowers/specs/2026-11-24-autodeafen-for-linux-design.md

## Global Constraints

- Mod id `heav33n.autodeafen-for-linux`, name `AutoDeafen for Linux`, v0.1.0, MIT, win 2.2091 only.
- No socket or web work on the main thread.
- Never log the secret, code or tokens. Send them only to Discord.
- Never use StreamKit's client ID. Never copy code from Lynxdeer AutoDeafen or AutoDeafen-Linux.
- OAuth redirect `http://localhost`; scopes `rpc rpc.voice.read rpc.voice.write`.
- README and about.md state that the mod is AI-written and unrelated to Lynxdeer AutoDeafen and snytex AutoDeafen-Linux.
- Commit locally per task. Never push or publish without the owner's yes.

## Review Focus

1. Player already deafened before the threshold: still deafened after. (Task 1)
2. Dying or restarting before Discord answers: ends undeafened. (Task 1)
3. Discord restarts mid-run: never undeafens a player it no longer deafened. (Task 1)
4. Split or oversized frames: reassembled, or the link is dropped. (Task 2)
5. Expired login: one refresh, then "setup needed", never a retry loop. (Task 3)

### Task 1: Run logic

**Files:** Create `src/run_tracker.hpp`, `src/deafen_intent.hpp`, `tests/run.cpp`, `tests/run.sh`, `.gitignore`, `LICENSE`

**Produces:**
- `struct RunConfig { bool enabled; float percent; bool practice; };`
- `struct RunTracker { std::optional<bool> frame(RunConfig, float percent, bool inPractice, bool platformer); std::optional<bool> ended(); };` true = deafen, false = undeafen, nullopt = nothing. `ended()` covers death, completion, restart and quit.
- `struct DeafenIntent { void want(bool); std::optional<bool> next(bool discordDeaf); void applied(bool); void linkLost(); bool needsCheck() const; };` `next` is the value to send.

- [ ] Write failing tests:
  - 49.9 at 50% gives nullopt; 50 gives true; the next frame gives nullopt.
  - `ended()` gives false once, then nullopt; nullopt if never deafened.
  - Nullopt when disabled, platformer, percent 100, or practice with `practice=false`.
  - `want(true); next(true)` nullopt, then `want(false); next(true)` nullopt. (Focus 1)
  - `want(true); want(false); next(false)` nullopt. `want(true); next(false)==true; applied(true); want(false); next(true)==false`. (Focus 2)
  - `want(true); next(false); applied(true); linkLost(); want(false); next(true)` nullopt. (Focus 3)
- [ ] Run `sh tests/run.sh`. Expected: compile failure.
- [ ] Implement both headers.
- [ ] Run `sh tests/run.sh`. Expected: all PASS, exit 0.
- [ ] `git init` and commit `feat: run logic`

### Task 2: Discord framing and socket paths

**Files:** Create `src/frame.hpp`, `src/socket_paths.hpp`; modify `tests/run.cpp`

**Produces:**
- `std::string encodeFrame(uint32_t op, std::string_view json)`: little-endian op, length, body.
- `struct FrameReader { void feed(std::string_view); std::optional<std::pair<uint32_t,std::string>> pop(); bool broken() const; };` broken above 1 MiB.
- `std::vector<std::string> socketPaths(std::string_view runtimeDir)`: for N in 0..9, `discord-ipc-N` in the dir, then in `app/com.discordapp.Discord/`. Skips paths over 107 bytes. Empty dir means none.

- [ ] Write failing tests: round trip; one byte at a time; two frames in one feed; length 1048577 sets broken (Focus 4); first path for `/run/user/1000` is `/run/user/1000/discord-ipc-0`, 20 in total; a 120-byte dir gives 0.
- [ ] Run. Expected: compile failure. Implement. Run. Expected: PASS.
- [ ] Commit `feat: Discord framing`

### Task 3: Discord link and login

**Files:** Create `src/auth_step.hpp`, `src/auth.hpp`, `src/auth.cpp`, `src/discord_link.hpp`, `src/discord_link.cpp`, `src/main.cpp`, `mod.json`, `CMakeLists.txt`, `build.sh`, `about.md`; modify `tests/run.cpp`

**Consumes:** `DeafenIntent`, `encodeFrame`, `FrameReader`, `socketPaths`

**Produces:**
- `enum class AuthStep { UseToken, Refresh, NeedSetup }; AuthStep nextAuthStep(bool hasToken, bool hasRefresh, bool tokenRejected, bool refreshFailed);`
- `enum class LinkStatus { NeedSetup, Disconnected, Unsupported, WaitingForApproval, Connected };`
- `namespace link { void start(); void stop(); void want(bool deaf); void authorize(); LinkStatus status(); }`
- `namespace auth { void setApp(std::string id, std::string secret); bool hasApp(); void exchange(std::string code); void refresh(); }` main thread only; web completion wakes the link.

- [ ] Write failing tests (Focus 5): token present gives UseToken; rejected with refresh gives Refresh; rejected with refreshFailed gives NeedSetup; nothing saved gives NeedSetup.
- [ ] Run. Expected: compile failure. Implement `nextAuthStep`. Run. Expected: PASS.
- [ ] Implement the link. Worker waits on a condition variable plus `select` (250 ms). Sequence: handshake, AUTHENTICATE, then GET_VOICE_SETTINGS and SET_VOICE_SETTINGS `{"deaf":bool}`. Replies match by nonce. Reconnect every 5 s. WSA error 10047 gives Unsupported. `stop()` undeafens if ours, within 500 ms.
- [ ] Implement login: AUTHORIZE, then POST `https://discord.com/api/oauth2/token` form-encoded with `grant_type=authorization_code`, `code`, `redirect_uri`, or `grant_type=refresh_token`, `refresh_token`. Save with `setSavedValue`.
- [ ] Run `./build.sh`. Expected: a `.geode` in `build-win/`. `grep -nE 'log::' src/auth.cpp src/discord_link.cpp` shows no secret, code or token.
- [ ] Commit `feat: Discord link`

### Task 4: Hooks, popups, README

**Files:** Create `src/setup_popup.hpp`, `src/level_popup.hpp`, `src/popups.cpp`, `README.md`; modify `src/main.cpp`, `mod.json`

**Consumes:** everything above

**Produces:** `SetupPopup::create()`, `LevelPopup::create(std::string levelKey)`

- [ ] mod.json settings: `enabled` bool false, `percent` float 50 (0-100), `practice` bool false. Dependency `geode.node-ids`.
- [ ] Hooks: PlayLayer `init`, `postUpdate`, `destroyPlayer` (only when the player died), `resetLevel`, `levelComplete`, `onQuit`; PauseLayer `customSetup` adds `deafen-button`_spr to `right-button-menu`. `start()` on load, `stop()` on exit. Level key `level-<type>-<id>` with local levels using `m_M_ID`.
- [ ] LevelPopup: on/off, percent input, status text, Setup button. SetupPopup: ID input, password-mode secret input, portal link, Authorize. It shows the redirect `http://localhost`.
- [ ] README: AI-written notice, setup, supported runtimes table from the spec, and that the secret is stored in plain text in Geode's save folder.
- [ ] Run tests and `./build.sh`. Expected: PASS and a `.geode`.
- [ ] Commit `feat: in-game UI`

### Task 5: Live check with the owner

- [ ] Owner closes GD. Install the `.geode`; keep the old autodeafen file as is.
- [ ] Owner sets up and authorizes. Expected: status Connected.
- [ ] In a voice call, check each: deafens at %; undeafens on death, completion, restart, quit; already deafened stays so; Discord closed mid-run; GD killed while deafened; Discord's own deafen button while connected; no visible stutter; Mega Hack on.
- [ ] If settings lock or do not reset, connect only around a run and update the spec.
- [ ] Record results in the spec and commit.

Later, separate plan: the helper for Valve Proton.
