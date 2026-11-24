#pragma once
#include <filesystem>

// Lets only this user into the folder. Wine turns this into Linux permissions,
// so other users on the PC cannot read the saved login or pose as the helper.
// Tried on an empty folder inside it first, so it can never lock us out.
bool makePrivate(std::filesystem::path const& folder);
