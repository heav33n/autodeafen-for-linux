#pragma once

enum class AuthStep { UseToken, Refresh, NeedSetup };

// What to try next when logging in. Each step is tried at most once.
inline AuthStep nextAuthStep(bool hasToken, bool hasRefresh, bool tokenRejected, bool refreshFailed) {
    if (hasToken && !tokenRejected) return AuthStep::UseToken;
    if (hasRefresh && !refreshFailed) return AuthStep::Refresh;
    return AuthStep::NeedSetup;
}
