// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines the entry point for the console application.
//

#include "stdafx.h"

// Device enumeration
std::string EnumerateAllDevices(DWORD* device_count);
void ChooseDevice(std::string id, _Outptr_result_maybenull_ IPortableDevice** device);

// Content enumeration
void EnumerateAllContent(_In_ IPortableDevice* device);
void ReadHintLocations(_In_ IPortableDevice* device);

// Content transfer
void TransferContentFromDevice(_In_ IPortableDevice* device);
std::string TransferContentToDevice(_In_ IPortableDevice* device,
                                    _In_ const std::string& parent_id,
                                    _In_ const std::string& local_file_name,
                                    _In_ const std::string& target_title);
void TransferContactToDevice(_In_ IPortableDevice*
                                                          device);
void CreateFolderOnDevice(_In_ IPortableDevice* device);
void CreateContactPhotoResourceOnDevice(_In_ IPortableDevice* device);

// Content deletion
std::string DeleteContentFromDevice(_In_ IPortableDevice* device, std::string object_name);

// Content moving
void MoveContentAlreadyOnDevice(_In_ IPortableDevice* device);

// Content update (properties and data simultaneously)
void UpdateContentOnDevice(
    _In_ IPortableDevice* device,
    _In_ REFGUID          contentType,
    _In_ PCWSTR           fileTypeFilter,
    _In_ PCWSTR           defaultFileExtension);
    
// Content properties
void ReadContentProperties(_In_ IPortableDevice* device);
void WriteContentProperties(_In_ IPortableDevice* device);
std::string ReadContentPropertiesBulk(_In_ IPortableDevice* device);
void WriteContentPropertiesBulk(_In_ IPortableDevice* device);
void ReadContentPropertiesBulkFilteringByFormat(_In_ IPortableDevice* device);

// Functional objects
void ListFunctionalObjects(_In_ IPortableDevice* device);
void ListFunctionalCategories(_In_ IPortableDevice* device);
void ListSupportedContentTypes(_In_ IPortableDevice* device);
void ListRenderingCapabilityInformation(_In_ IPortableDevice* device);

// Device events
void ListSupportedEvents(_In_ IPortableDevice* device);
void RegisterForEventNotifications(_In_ IPortableDevice* device, _Inout_ PWSTR* eventCookie);
void UnregisterForEventNotifications(_In_opt_ IPortableDevice* device, _In_opt_ PCWSTR eventCookie);

// Misc.
void GetObjectIdentifierFromPersistentUniqueIdentifier(_In_ IPortableDevice* device);

std::string GetBulkProperties(const std::string& device_id) {
  ComPtr<IPortableDevice> device;
    
  ChooseDevice(device_id, &device);

  if (!device) {
    return "{\"status\":-1}";
  } else {
    return ReadContentPropertiesBulk(device.Get());
  }
}

std::string DoDeleteObject(const std::string& device_id,
                           const std::string& object_name) {
  ComPtr<IPortableDevice> device;

  ChooseDevice(device_id, &device);

  if (!device) {
    return "{\"status\":-1}";
  } else {
    return DeleteContentFromDevice(device.Get(), object_name);
  }
}

std::string DoSendFile(const std::string& device_id,
                           const std::string& parent_name, const std::string& local_file_name, const std::string& target_title) {
  ComPtr<IPortableDevice> device;

  ChooseDevice(device_id, &device);

  if (!device) {
    return "{\"status\":-1}";
  } else {
    return TransferContentToDevice(device.Get(), parent_name, local_file_name, target_title);
  }
}

void DoServerLoop() {
  httplib::Server svr;

  svr.Get("/devices", [](const httplib::Request&, httplib::Response& res) {
    DWORD unused;
    std::string device_list = EnumerateAllDevices(&unused);
    res.set_content(device_list.c_str(), "application/json");
  });

  svr.Get("/bulk-properties/:id",
          [&](const httplib::Request& req, httplib::Response& res) {
            auto device_id = req.path_params.at("id");
            res.set_content(GetBulkProperties(device_id), "application/json");
  });

  svr.Post("/delete-object/:id", [&](const httplib::Request& req,
                                     httplib::Response& res) {
    auto device_id = req.path_params.at("id");
    if (!req.has_param("objectid")) {
      res.status = 500;
      return;
    }
    std::string object = req.get_param_value("objectid");

    res.set_content(DoDeleteObject(device_id, object), "application/json");
  });

  svr.Post("/send-file/:id", [&](const httplib::Request& req,
                                     httplib::Response& res) {
    auto device_id = req.path_params.at("id");
    if (!req.has_param("parentid")) {
      res.status = 500;
      return;
    }
    if (!req.has_param("localfilename")) {
      res.status = 500;
      return;
    }
    if (!req.has_param("targettitle")) {
      res.status = 500;
      return;
    }
    std::string parent = req.get_param_value("parentid");
    std::string localfilename = req.get_param_value("localfilename");
    std::string targettitle = req.get_param_value("targettitle");

    res.set_content(DoSendFile(device_id, parent, localfilename, targettitle), "application/json");
  });


#ifdef _DEBUG
  int port = 1072;
  svr.bind_to_port("127.0.0.1", port);
#else
  int port = svr.bind_to_any_port("127.0.0.1");
#endif
  fwprintf(stdout, L"{\"port\":%d}\n", port);
  fflush(stdout);
  svr.listen_after_bind();
}

void DoCommandLoop()
{
    HRESULT hr                  = S_OK;
    UINT    selectionIndex      = 0;
    PWSTR   eventCookie         = nullptr;
    WCHAR   selectionString[SELECTION_BUFFER_SIZE] = {0};
    ComPtr<IPortableDevice> device;

    /* ChooseDevice(&device);

    if (device == nullptr)
    {
        // No device was selected, so exit immediately.
        return;
    }*/

    while (selectionIndex != 99)
    {
        ZeroMemory(selectionString, sizeof(selectionString));
        /*
        fwprintf(stderr, L"\n\n");
        fwprintf(stderr, L"WPD Sample Application \n");
        fwprintf(stderr, L"=======================================\n\n");
        fwprintf(stderr, L"0.  Enumerate all Devices\n");
        fwprintf(stderr, L"1.  Choose a Device\n");
        fwprintf(stderr, L"2.  Enumerate all content on the device\n");
        fwprintf(stderr, L"3.  Transfer content from the device\n");
        fwprintf(stderr, L"4.  Delete content from the device\n");
        fwprintf(stderr, L"5.  Move content already on the device to another location on the device\n");
        fwprintf(stderr, L"6   Transfer Image content to the device\n");
        fwprintf(stderr, L"7.  Transfer Music content to the device\n");
        fwprintf(stderr, L"8.  Transfer Contact (VCARD file) content to the device\n");
        fwprintf(stderr, L"9.  Transfer Contact (Defined by Properties Only) to the device\n");
        fwprintf(stderr, L"10. Create a folder on the device\n");
        fwprintf(stderr, L"11. Add a Contact Photo resource to an object\n");
        fwprintf(stderr, L"12. Read properties on a content object\n");
        fwprintf(stderr, L"13. Write properties on a content object\n");
        fwprintf(stderr, L"14. Get an object identifier from a Persistent Unique Identifier (PUID)\n");
        fwprintf(stderr, L"15. List all functional categories supported by the device\n");
        fwprintf(stderr, L"16. List all functional objects on the device\n");
        fwprintf(stderr, L"17. List all content types supported by the device\n");
        fwprintf(stderr, L"18. List rendering capabilities supported by the device\n");
        fwprintf(stderr, L"19. Register to receive device event notifications\n");
        fwprintf(stderr, L"20. Unregister from receiving device event notifications\n");
        fwprintf(stderr, L"21. List all events supported by the device\n");
        fwprintf(stderr, L"22. List all hint locations supported by the device\n");
        fwprintf(stderr, L"==(Advanced BULK property operations)==\n");
        fwprintf(stderr, L"23. Read properties on multiple content objects\n");
        fwprintf(stderr, L"24. Write properties on multiple content objects\n");
        fwprintf(stderr, L"25. Read properties on multiple content objects using object format\n");
        fwprintf(stderr, L"==(Update content operations)==\n");
        fwprintf(stderr, L"26. Update Image content (properties and data) on the device\n");
        fwprintf(stderr, L"27. Update Music content (properties and data) on the device\n");
        fwprintf(stderr, L"28. Update Contact content (properties and data) on the device\n");
        fwprintf(stderr, L"99. Exit\n");
        */
        hr = StringCchGetsW(selectionString, ARRAYSIZE(selectionString));
        if (SUCCEEDED(hr) && selectionString[0] != 0)
        {
            selectionIndex = static_cast<UINT>(_wtoi(selectionString));
            switch (selectionIndex)
            {
              /* case 0:
                DWORD device_count;
                    EnumerateAllDevices(&device_count);
                    break;
                case 1:
                    // Unregister any device event registrations before
                    // creating a new IPortableDevice
                    UnregisterForEventNotifications(device.Get(), eventCookie);
                    CoTaskMemFree(eventCookie);
                    eventCookie = nullptr;

                    // Release the old IPortableDevice interface before
                    // obtaining a new one.
                    device = nullptr;
                    ChooseDevice("", &device);
                    break;*/
                case 2:
                    EnumerateAllContent(device.Get());
                    break;
                case 3:
                    TransferContentFromDevice(device.Get());
                    break;
                /*case 4:
                    DeleteContentFromDevice(device.Get());
                    break;*/
                case 5:
                    MoveContentAlreadyOnDevice(device.Get());
                    break;
                    /*
                case 6:
                    TransferContentToDevice(device.Get(),
                                            WPD_CONTENT_TYPE_IMAGE,
                                            L"JPEG (*.JPG)\0*.JPG\0JPEG (*.JPEG)\0*.JPEG\0JPG (*.JPE)\0*.JPE\0JPG (*.JFIF)\0*.JFIF\0\0",
                                            L"JPG");
                    break;
                case 7:
                    TransferContentToDevice(device.Get(),
                                            WPD_CONTENT_TYPE_AUDIO,
                                            L"MP3 (*.MP3)\0*.MP3\0\0",
                                            L"MP3");
                    break;
                case 8:
                    TransferContentToDevice(device.Get(),
                                            WPD_CONTENT_TYPE_CONTACT,
                                            L"VCARD (*.VCF)\0*.VCF\0\0",
                                            L"VCF");
                    break;
                    */
                case 9:
                    TransferContactToDevice(device.Get());
                    break;
                case 10:
                    CreateFolderOnDevice(device.Get());
                    break;
                case 11:
                    CreateContactPhotoResourceOnDevice(device.Get());
                    break;
                case 12:
                    ReadContentProperties(device.Get());
                    break;
                case 13:
                    WriteContentProperties(device.Get());
                    break;
                case 14:
                    GetObjectIdentifierFromPersistentUniqueIdentifier(device.Get());
                    break;
                case 15:
                    ListFunctionalCategories(device.Get());
                    break;
                case 16:
                    ListFunctionalObjects(device.Get());
                    break;
                case 17:
                    ListSupportedContentTypes(device.Get());
                    break;
                case 18:
                    ListRenderingCapabilityInformation(device.Get());
                    break;
                case 19:
                    RegisterForEventNotifications(device.Get(), &eventCookie);
                    break;
                case 20:
                    UnregisterForEventNotifications(device.Get(), eventCookie);
                    CoTaskMemFree(eventCookie);
                    eventCookie = nullptr;
                    break;
                case 21:
                    ListSupportedEvents(device.Get());
                    break;
                case 22:
                    ReadHintLocations(device.Get());
                    break;
                case 23:
                    ReadContentPropertiesBulk(device.Get());
                    break;
                case 24:
                    WriteContentPropertiesBulk(device.Get());
                    break;
                case 25:
                    ReadContentPropertiesBulkFilteringByFormat(device.Get());
                    break;
                case 26:
                    UpdateContentOnDevice(device.Get(),
                                          WPD_CONTENT_TYPE_IMAGE,
                                          L"JPEG (*.JPG)\0*.JPG\0JPEG (*.JPEG)\0*.JPEG\0JPG (*.JPE)\0*.JPE\0JPG (*.JFIF)\0*.JFIF\0\0",
                                          L"JPG");
                    break;
                case 27:
                    UpdateContentOnDevice(device.Get(),
                                          WPD_CONTENT_TYPE_AUDIO,
                                          L"MP3 (*.MP3)\0*.MP3\0\0",
                                          L"MP3");
                    break;
                case 28:
                    UpdateContentOnDevice(device.Get(),
                                          WPD_CONTENT_TYPE_CONTACT,
                                          L"VCARD (*.VCF)\0*.VCF\0\0",
                                          L"VCF");
                    break;
                default:
                    break;
            }
        }
        else
        {
            fwprintf(stderr, L"! Failed to read menu selection string input, hr = 0x%lx\n", hr);
        }
    
        // Response terminator.
        fwprintf(stdout, L"---\n");
        fflush(stdout);
    }
    CoTaskMemFree(eventCookie);
}

#if 1
int main(int, char**) {
#else
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
    // Enable the heap manager to terminate the process on heap error.
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // Initialize COM for COINIT_MULTITHREADED
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        if (argc == 2 && wcscmp(argv[1], L"--daemon") == 0) {
            DoServerLoop();
        }
        
        // Uninitialize COM
        CoUninitialize();
    }

    LocalFree(argv);

    return 0;
}
