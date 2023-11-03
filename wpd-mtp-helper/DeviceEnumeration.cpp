// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "stdafx.h"

std::wstring multi2wide(const std::string& str, UINT codePage = CP_THREAD_ACP) {
  if (str.empty()) {
    return std::wstring();
  }

  int required =
      ::MultiByteToWideChar(codePage, 0, str.data(), (int)str.size(), NULL, 0);
  if (0 == required) {
    return std::wstring();
  }

  std::wstring str2;
  str2.resize(required);

  int converted = ::MultiByteToWideChar(
      codePage, 0, str.data(), (int)str.size(), &str2[0], (int)str2.capacity());
  if (0 == converted) {
    return std::wstring();
  }

  return str2;
}

std::string wide2utf8(const wchar_t* wide) {
  int buf_size =
      ::WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
  std::string utf8(buf_size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, &utf8[0], buf_size, nullptr,
                      nullptr);
  utf8.resize(buf_size - 1);
  return utf8;
}

std::string quoted_utf8(const WCHAR* str) {
  std::string utf8 = wide2utf8(str);
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

//<SnippetDeviceEnum6>
#define CLIENT_NAME L"Windows"
#define CLIENT_MAJOR_VER 10
#define CLIENT_MINOR_VER 0
#define CLIENT_REVISION 0
//</SnippetDeviceEnum6>
// Reads and displays the device friendly name for the specified PnPDeviceID
// string
std::string DisplayFriendlyName(_In_ IPortableDeviceManager* deviceManager,
                                _In_ PCWSTR pnpDeviceID) {
  std::string result;
  DWORD friendlyNameLength = 0;

  // 1) Pass nullptr as the PWSTR return string parameter to get the total
  // number of characters to allocate for the string value.
  HRESULT hr = deviceManager->GetDeviceFriendlyName(pnpDeviceID, nullptr,
                                                    &friendlyNameLength);
  if (FAILED(hr)) {
    fwprintf(stderr, 
        L"! Failed to get number of characters for device friendly name, hr = "
        L"0x%lx\n",
        hr);
  } else if (friendlyNameLength > 0) {
    // 2) Allocate the number of characters needed and retrieve the string
    // value.
    PWSTR friendlyName = new (std::nothrow) WCHAR[friendlyNameLength];
    if (friendlyName != nullptr) {
      ZeroMemory(friendlyName, friendlyNameLength * sizeof(WCHAR));
      hr = deviceManager->GetDeviceFriendlyName(pnpDeviceID, friendlyName,
                                                &friendlyNameLength);
      if (SUCCEEDED(hr)) {
        result += "    \"friendly\": ";
        result += quoted_utf8(friendlyName);
        result += ",\n";
      } else {
        fwprintf(stderr, L"! Failed to get device friendly name, hr = 0x%lx\n", hr);
      }

      // Delete the allocated friendly name string
      delete[] friendlyName;
      friendlyName = nullptr;
    } else {
      fwprintf(stderr, 
          L"! Failed to allocate memory for the device friendly name string\n");
    }
  } else {
    fwprintf(stderr, L"The device did not provide a friendly name.\n");
  }

  return result;
}

// Reads and displays the device manufacturer for the specified PnPDeviceID
// string
std::string DisplayManufacturer(_In_ IPortableDeviceManager* deviceManager,
                         _In_ PCWSTR pnpDeviceID) {
  std::string result;
  DWORD manufacturerLength = 0;

  // 1) Pass nullptr as the PWSTR return string parameter to get the total
  // number of characters to allocate for the string value.
  HRESULT hr = deviceManager->GetDeviceManufacturer(pnpDeviceID, nullptr,
                                                    &manufacturerLength);
  if (FAILED(hr)) {
    fwprintf(stderr, 
        L"! Failed to get number of characters for device manufacturer, hr = "
        L"0x%lx\n",
        hr);
  } else if (manufacturerLength > 0) {
    // 2) Allocate the number of characters needed and retrieve the string
    // value.
    PWSTR manufacturer = new (std::nothrow) WCHAR[manufacturerLength];
    if (manufacturer != nullptr) {
      ZeroMemory(manufacturer, manufacturerLength * sizeof(WCHAR));
      hr = deviceManager->GetDeviceManufacturer(pnpDeviceID, manufacturer,
                                                &manufacturerLength);
      if (SUCCEEDED(hr)) {
        result += "    \"manufacturer\": ";
        result += quoted_utf8(manufacturer);
        result += ",\n";
      } else {
        fwprintf(stderr, L"! Failed to get device manufacturer, hr = 0x%lx\n", hr);
      }

      // Delete the allocated manufacturer string
      delete[] manufacturer;
      manufacturer = nullptr;
    } else {
      fwprintf(stderr, 
          L"! Failed to allocate memory for the device manufacturer string\n");
    }
  } else {
    fwprintf(stderr, L"The device did not provide a manufacturer.\n");
  }

  return result;
}

// Reads and displays the device discription for the specified PnPDeviceID
// string
std::string DisplayDescription(_In_ IPortableDeviceManager* deviceManager,
                        _In_ PCWSTR pnpDeviceID) {
  std::string result;
  DWORD descriptionLength = 0;

  // 1) Pass nullptr as the PWSTR return string parameter to get the total
  // number of characters to allocate for the string value.
  HRESULT hr = deviceManager->GetDeviceDescription(pnpDeviceID, nullptr,
                                                   &descriptionLength);
  if (FAILED(hr)) {
    fwprintf(stderr, 
        L"! Failed to get number of characters for device description, hr = "
        L"0x%lx\n",
        hr);
  } else if (descriptionLength > 0) {
    // 2) Allocate the number of characters needed and retrieve the string
    // value.
    PWSTR description = new (std::nothrow) WCHAR[descriptionLength];
    if (description != nullptr) {
      ZeroMemory(description, descriptionLength * sizeof(WCHAR));
      hr = deviceManager->GetDeviceDescription(pnpDeviceID, description,
                                               &descriptionLength);
      if (SUCCEEDED(hr)) {
        result += "    \"description\": ";
        result += quoted_utf8(description);
        result += "\n";
      } else {
        fwprintf(stderr, L"! Failed to get device description, hr = 0x%lx\n", hr);
      }

      // Delete the allocated description string
      delete[] description;
      description = nullptr;
    } else {
      fwprintf(stderr, 
          L"! Failed to allocate memory for the device description string\n");
    }
  } else {
    fwprintf(stderr, L"The device did not provide a description.\n");
  }
  return result;
}

std::string EnumerateAllDevices(DWORD* device_id_count) {
  DWORD pnpDeviceIDCount = 0;
  ComPtr<IPortableDeviceManager> deviceManager;
  std::string result;

  // CoCreate the IPortableDeviceManager interface to enumerate
  // portable devices and to get information about them.
  //<SnippetDeviceEnum1>
  HRESULT hr =
      CoCreateInstance(CLSID_PortableDeviceManager, nullptr,
                       CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceManager));
  if (FAILED(hr)) {
    fwprintf(stderr, 
        L"! Failed to CoCreateInstance CLSID_PortableDeviceManager, hr = "
        L"0x%lx\n",
        hr);
  }
  //</SnippetDeviceEnum1>

  // 1) Pass nullptr as the PWSTR array pointer to get the total number
  // of devices found on the system.
  //<SnippetDeviceEnum2>
  if (SUCCEEDED(hr)) {
    hr = deviceManager->GetDevices(nullptr, &pnpDeviceIDCount);
    if (FAILED(hr)) {
      fwprintf(stderr, L"! Failed to get number of devices on the system, hr = 0x%lx\n",
              hr);
    }
  }

  *device_id_count = pnpDeviceIDCount;

  // Report the number of devices found.  NOTE: we will report 0, if an error
  // occured.

  // fwprintf(stderr, L"\n%u Windows Portable Device(s) found on the system\n\n",
  // pnpDeviceIDCount);
  result = "[\n";
  //</SnippetDeviceEnum2>
  // 2) Allocate an array to hold the PnPDeviceID strings returned from
  // the IPortableDeviceManager::GetDevices method
  //<SnippetDeviceEnum3>
  if (SUCCEEDED(hr) && (pnpDeviceIDCount > 0)) {
    PWSTR* pnpDeviceIDs = new (std::nothrow) PWSTR[pnpDeviceIDCount];
    if (pnpDeviceIDs != nullptr) {
      ZeroMemory(pnpDeviceIDs, pnpDeviceIDCount * sizeof(PWSTR));
      DWORD retrievedDeviceIDCount = pnpDeviceIDCount;
      hr = deviceManager->GetDevices(pnpDeviceIDs, &retrievedDeviceIDCount);

      if (SUCCEEDED(hr)) {
        _Analysis_assume_(retrievedDeviceIDCount <= pnpDeviceIDCount);
        // For each device found, display the devices friendly name,
        // manufacturer, and description strings.
        for (DWORD index = 0; index < retrievedDeviceIDCount; index++) {
          result += "  {\n";
          result += "    \"index\": ";
          std::stringstream ss;
          ss << index;
          result += ss.str();
          result += ",\n";

          result += "    \"id\": " + quoted_utf8(pnpDeviceIDs[index]) + ",\n";
          result +=
              DisplayFriendlyName(deviceManager.Get(), pnpDeviceIDs[index]);
          result +=
              DisplayManufacturer(deviceManager.Get(), pnpDeviceIDs[index]);
          result +=
              DisplayDescription(deviceManager.Get(), pnpDeviceIDs[index]);
          result += "  }";
          if (index < retrievedDeviceIDCount - 1) {
            result += ",";
          }
          result += "\n";
        }
      } else {
        fwprintf(stderr, 
            L"! Failed to get the device list from the system, hr = 0x%lx\n",
            hr);
      }
      //</SnippetDeviceEnum3>

      // Free all returned PnPDeviceID strings by using CoTaskMemFree.
      // NOTE: CoTaskMemFree can handle nullptr pointers, so no nullptr
      //       check is needed.
      for (DWORD index = 0; index < pnpDeviceIDCount; index++) {
        CoTaskMemFree(pnpDeviceIDs[index]);
        pnpDeviceIDs[index] = nullptr;
      }

      // Delete the array of PWSTR pointers
      delete[] pnpDeviceIDs;
      pnpDeviceIDs = nullptr;
    } else {
      fwprintf(stderr, L"! Failed to allocate memory for PWSTR array\n");
    }
  }
  result += "]\n";

  return result;
}

// Creates and populates an IPortableDeviceValues with information about
// this application.  The IPortableDeviceValues is used as a parameter
// when calling the IPortableDevice::Open() method.
void GetClientInformation(
    _Outptr_result_maybenull_ IPortableDeviceValues** clientInformation) {
  // Client information is optional.  The client can choose to identify itself,
  // or to remain unknown to the driver.  It is beneficial to identify yourself
  // because drivers may be able to optimize their behavior for known clients.
  // (e.g. An IHV may want their bundled driver to perform differently when
  // connected to their bundled software.)

  // CoCreate an IPortableDeviceValues interface to hold the client information.
  //<SnippetDeviceEnum7>
  HRESULT hr =
      CoCreateInstance(CLSID_PortableDeviceValues, nullptr,
                       CLSCTX_INPROC_SERVER, IID_PPV_ARGS(clientInformation));
  //</SnippetDeviceEnum7>
  //<SnippetDeviceEnum8>
  if (SUCCEEDED(hr)) {
    // Attempt to set all bits of client information
    hr = (*clientInformation)->SetStringValue(WPD_CLIENT_NAME, CLIENT_NAME);
    if (FAILED(hr)) {
      fwprintf(stderr, L"! Failed to set WPD_CLIENT_NAME, hr = 0x%lx\n", hr);
    }

    hr = (*clientInformation)
             ->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION,
                                       CLIENT_MAJOR_VER);
    if (FAILED(hr)) {
      fwprintf(stderr, L"! Failed to set WPD_CLIENT_MAJOR_VERSION, hr = 0x%lx\n", hr);
    }

    hr = (*clientInformation)
             ->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION,
                                       CLIENT_MINOR_VER);
    if (FAILED(hr)) {
      fwprintf(stderr, L"! Failed to set WPD_CLIENT_MINOR_VERSION, hr = 0x%lx\n", hr);
    }

    hr = (*clientInformation)
             ->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, CLIENT_REVISION);
    if (FAILED(hr)) {
      fwprintf(stderr, L"! Failed to set WPD_CLIENT_REVISION, hr = 0x%lx\n", hr);
    }

    //  Some device drivers need to impersonate the caller in order to function
    //  correctly.  Since our application does not need to restrict its
    //  identity, specify SECURITY_IMPERSONATION so that we work with all
    //  devices.
    hr = (*clientInformation)
             ->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE,
                                       SECURITY_IMPERSONATION);
    if (FAILED(hr)) {
      fwprintf(stderr, 
          L"! Failed to set WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, hr = "
          L"0x%lx\n",
          hr);
    }
  } else {
    fwprintf(stderr, 
        L"! Failed to CoCreateInstance CLSID_PortableDeviceValues, hr = "
        L"0x%lx\n",
        hr);
  }
  //</SnippetDeviceEnum8>
}

// Calls EnumerateDevices() function to display devices on the system
// and to obtain the total number of devices found.  If 1 or more devices
// are found, this function prompts the user to choose a device using
// a zero-based index.
void ChooseDevice(std::string device_id, _Outptr_result_maybenull_ IPortableDevice** device) {
    *device = nullptr;

    HRESULT                        hr                  = S_OK;
    //UINT                           currentDeviceIndex  = 0;
//    DWORD                          pnpDeviceIDCount    = 0;
  //  PWSTR*                         pnpDeviceIDs        = nullptr;
//    WCHAR                          selection[SELECTION_BUFFER_SIZE] = {0};

    //ComPtr<IPortableDeviceManager> deviceManager;
    ComPtr<IPortableDeviceValues>  clientInformation;

    // Fill out information about your application, so the device knows
    // who they are speaking to.

    GetClientInformation(&clientInformation);

    //<SnippetDeviceEnum5>
    // CoCreate the IPortableDevice interface and call Open() with
    // the chosen PnPDeviceID string.
    hr = CoCreateInstance(CLSID_PortableDeviceFTM,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(device));
    if (SUCCEEDED(hr))
    {
        hr = (*device)->Open(multi2wide(device_id).c_str(), clientInformation.Get());

        if (hr == E_ACCESSDENIED)
        {
            fwprintf(stderr, L"Failed to Open the device for Read Write access, will open it for Read-only access instead\n");
            clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_DESIRED_ACCESS, GENERIC_READ);
            hr = (*device)->Open(multi2wide(device_id).c_str(), clientInformation.Get());
        }

        if (FAILED(hr))
        {
            fwprintf(stderr, L"! Failed to Open the device, hr = 0x%lx\n", hr);
            // Release the IPortableDevice interface, because we cannot proceed
            // with an unopen device.
            (*device)->Release();
            *device = nullptr;
        }
    }
    else
    {
        fwprintf(stderr, L"! Failed to CoCreateInstance CLSID_PortableDeviceFTM, hr = 0x%lx\n", hr);
    }

    // If no devices were found on the system, just exit this function.
}
