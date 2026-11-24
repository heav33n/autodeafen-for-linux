#pragma once
#include <string_view>

enum class AuthStep { UseToken, Refresh, NeedSetup };

// What to try next when logging in. Each step is tried at most once.
inline AuthStep nextAuthStep(bool hasToken, bool hasRefresh, bool tokenRejected, bool refreshFailed) {
    if (hasToken && !tokenRejected) return AuthStep::UseToken;
    if (hasRefresh && !refreshFailed) return AuthStep::Refresh;
    return AuthStep::NeedSetup;
}

// Whether Discord itself turned the login down. Anything else, such as a rate
// limit or a Wi-Fi login page, may work later.
inline bool isRefusal(int httpCode, std::string_view oauthError) {
    return (httpCode == 400 || httpCode == 401) && !oauthError.empty();
}
