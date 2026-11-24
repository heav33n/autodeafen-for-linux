# AutoDeafen for Linux

A Geometry Dash mod that deafens you on Discord for the part of a level you
choose, and undeafens you after it or when you die, finish, restart or quit. It talks to Discord
directly, so it works on pure Wayland with **no keybinds and no Discord plugin**.

> **This mod was written by AI (Claude).** It is not on the Geode index and is
> not related to Lynxdeer's AutoDeafen or snytex's AutoDeafen-Linux.

## Requirements

Geometry Dash 2.2091, Geode 6, and one of these runtimes:

| Runtime | Works |
|---|---|
| proton-cachyos 11, GE-Proton 11 | Connects |
| Valve Proton 9, 10, 11, Experimental, Hotfix | Not yet |
| proton-cachyos 10, plain Wine 11.22 | Not yet |

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
and sent only to Discord. They are never logged. Do not share that folder.

## Building

`./build.sh` cross-compiles on Linux. `tests/run.sh` runs the tests.

## License

MIT
