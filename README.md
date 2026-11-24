# AutoDeafen for Linux

A Geometry Dash mod that deafens you on Discord for the part of a level you
choose, and undeafens you after it or when you die, finish, restart or quit. It talks to Discord
directly, so it works on pure Wayland with **no keybinds and no Discord plugin**.

> **This mod was written by AI (Claude).** That's why there's a high chance this mod won't be on the Geode index.

## Requirements

Geometry Dash 2.2091, Geode 6, 64-bit Linux, and Proton or Wine:

| Runtime | How it reaches Discord |
|---|---|
| proton-cachyos 11, GE-Proton 11 | Directly. Played in-game on proton-cachyos 11 |
| Valve Proton 9, 10, 11, Experimental, Hotfix | Helper. Connection tested, not yet played |
| proton-cachyos 10, Wine 11.22 | Helper. Connection tested, not yet played |

The helper is a small Linux program inside the mod, started only when Proton
cannot open Discord's socket. It passes messages between the game and Discord on
your PC and closes with the game. Its source is in `helper/`.

Discord must be the desktop app. Vesktop is not supported.

## Setup

1. Put the `.geode` file in `Geometry Dash/geode/mods` and start the game.
2. Pause any level, press the mute button, then **Discord Setup**.
3. In the Discord developer portal, create an application. Under **OAuth2**, add
   the redirect `http://localhost`.
4. Paste the client ID and secret into the game and press **Authorize**.
5. Approve it in Discord.

Everyone needs their own application because Discord only lets unapproved apps
control voice settings for their owner and listed testers.

## Privacy

The client secret and login are stored **in plain text** in Geode's save folder
and sent only to Discord. This mod never logs them. Do not share that folder.

Other users on your PC cannot open the mod's part of it. Other Geode mods, and
programs running as you, can read it, as with anything a mod saves.

## Building

`./build.sh` cross-compiles on Linux. `tests/run.sh` and
`tests/helper_test.py build-helper/autodeafen-helper` run the tests.
`tests/proton_smoke.sh` checks the connection on chosen Proton builds.

## License

MIT
