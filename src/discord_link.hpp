#pragma once

enum class LinkStatus { NeedSetup, Disconnected, Unsupported, WaitingForApproval, Connected };

// Keeps a connection to Discord on its own thread. Nothing here blocks the game.
namespace link {
    void start();
    // Undeafens if we deafened, waiting at most half a second.
    void stop();
    void want(bool deaf);
    // Asks Discord to show its Authorize popup, using the saved app.
    void authorize();
    LinkStatus status();
}
