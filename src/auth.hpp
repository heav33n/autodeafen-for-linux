#pragma once
#include <string>

// The user's own Discord app and login. Safe to call from any thread.
namespace auth {
    enum class Result { Ok, Rejected, NoNetwork };

    struct Login {
        std::string appId, secret, token, refresh;
        bool hasApp() const { return !appId.empty() && !secret.empty(); }
    };

    void load();
    Login current();
    void setApp(std::string id, std::string secret);
    // Both block on the network, so only the link thread calls them.
    Result exchange(std::string const& code);
    Result refresh();
}
