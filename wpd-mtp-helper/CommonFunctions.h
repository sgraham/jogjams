// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

// Implemented in ContentProperties.cpp
void DisplayStringProperty(
    _In_ IPortableDeviceValues*  properties,
    _In_ REFPROPERTYKEY          key,
    _In_ PCWSTR                  keyName);

// Implemented in ContentProperties.cpp
void DisplayGuidProperty(
    _In_ IPortableDeviceValues*  properties,
    _In_ REFPROPERTYKEY          key,
    _In_ PCWSTR                  keyName);

// Determines if a device supports a particular functional category.
BOOL SupportsCommand(
    _In_ IPortableDevice*  device,
    _In_ REFPROPERTYKEY    command);


// Helper class to convert a GUID to a string
class CGuidToString
{
private:
    WCHAR m_stringGUID[64];

public:
    CGuidToString(_In_ REFGUID guid)
    {
        if (!::StringFromGUID2(guid, m_stringGUID, ARRAYSIZE(m_stringGUID)))
        {
            m_stringGUID[0] = L'\0';
        }
    }

    operator PCWSTR()
    {
        return m_stringGUID;
    }
};


static inline std::wstring multi_to_wide(const std::string& str,
                                         UINT codePage = CP_THREAD_ACP) {
    if (str.empty()) {
        return std::wstring();
    }

    int required = ::MultiByteToWideChar(codePage, 0, str.data(),
                                         (int)str.size(), NULL, 0);
    if (0 == required) {
        return std::wstring();
    }

    std::wstring str2;
    str2.resize(required);

    int converted =
        ::MultiByteToWideChar(codePage, 0, str.data(), (int)str.size(),
                              &str2[0], (int)str2.capacity());
    if (0 == converted) {
        return std::wstring();
    }

    return str2;
}

static inline std::wstring utf8_to_wide(const std::string& str) {
    return multi_to_wide(str, CP_UTF8);
}

static inline std::string wide_to_utf8(const wchar_t* wide) {
    int buf_size = ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0,
                                         nullptr, nullptr);
    std::string utf8(buf_size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &utf8[0], buf_size, nullptr,
                        nullptr);
    utf8.resize(buf_size - 1);
    return utf8;
}

static inline std::string quoted_utf8(const WCHAR* str) {
    std::string utf8 = wide_to_utf8(str);
    std::string result("\"");
    for (DWORD i = 0; i < utf8.size(); ++i) {
        if (utf8[i] == '\"') {
            result += "\\\"";
        } else if (utf8[i] == '\\') {
            result += "\\\\";
        } else {
            result += utf8[i];
        }
    }
    result += "\"";
    return result;
}

static inline std::string int_to_str(int i){
    std::stringstream ss;
    ss << i;
    return ss.str();
}