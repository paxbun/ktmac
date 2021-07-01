// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/gui/ResourceUtils.hh>

#include <Windows.h>

namespace ktmac::gui
{

std::string_view LoadResource(int name)
{
    HMODULE module = GetModuleHandle(NULL);
    HRSRC   rc = FindResource(module, MAKEINTRESOURCE(name), MAKEINTRESOURCE(KTMAC_RESTYPE_HTML));
    if (rc == NULL)
        return {};

    HGLOBAL rcData = LoadResource(module, rc);
    if (rcData == NULL)
        return {};

    DWORD size = SizeofResource(module, rc);
    if (size == 0)
        return {};

    auto data = (char const*)LockResource(rcData);
    return std::string_view { data, size };
}

}