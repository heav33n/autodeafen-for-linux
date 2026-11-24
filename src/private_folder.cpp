#include "private_folder.hpp"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include <fstream>

namespace {
    bool restrict(std::filesystem::path const& folder) {
        // The folder's owner. Naming the user instead makes Wine let their whole
        // Linux group in as well.
        alignas(DWORD) std::byte sid[SECURITY_MAX_SID_SIZE];
        DWORD sidSize = sizeof(sid);
        alignas(ACL) std::byte list[sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + SECURITY_MAX_SID_SIZE];
        auto acl = reinterpret_cast<ACL*>(list);
        SECURITY_DESCRIPTOR descriptor;
        // Files inside keep their own permissions: the helper has to stay
        // executable, and nobody else can get through the folder to reach them.
        return CreateWellKnownSid(WinCreatorOwnerRightsSid, nullptr, sid, &sidSize)
            && InitializeAcl(acl, sizeof(list), ACL_REVISION)
            && AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, sid)
            && InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION)
            && SetSecurityDescriptorDacl(&descriptor, TRUE, acl, FALSE)
            && SetSecurityDescriptorControl(&descriptor, SE_DACL_PROTECTED, SE_DACL_PROTECTED)
            && SetFileSecurityW(folder.c_str(), DACL_SECURITY_INFORMATION, &descriptor);
    }

    bool usable(std::filesystem::path const& folder) {
        auto file = folder / "file";
        bool ok = false;
        if (std::ofstream out(file); out) ok = static_cast<bool>(out << "x" << std::flush);
        std::error_code error;
        return ok && std::filesystem::remove(file, error);
    }
}

bool makePrivate(std::filesystem::path const& folder) {
    auto trial = folder / "permission-test";
    std::error_code error;
    std::filesystem::remove(trial, error);
    if (!std::filesystem::create_directory(trial, error)) return false;
    bool safe = restrict(trial) && usable(trial);
    std::filesystem::remove(trial, error);
    return safe && restrict(folder);
}
