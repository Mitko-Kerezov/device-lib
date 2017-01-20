#ifndef Declarations_h
#define Declarations_h

#ifndef _WIN32
typedef void* HANDLE;
typedef unsigned long long SOCKET;
#endif // _WIN32


#pragma region Data_Structures_Definition

struct LengthEncodedMessage {
    char *message;
    size_t length;
    
    ~LengthEncodedMessage()
    {
        free(message);
    }
};

struct DeviceInfo {
    unsigned char unknown0[16]; /* 0 - zero */
    unsigned int device_id;     /* 16 */
    unsigned int product_id;    /* 20 - set to AMD_IPHONE_PRODUCT_ID */
    char *serial;               /* 24 - set to AMD_IPHONE_SERIAL */
    unsigned int unknown1;      /* 28 */
    unsigned char unknown2[4];  /* 32 */
    unsigned int lockdown_conn; /* 36 */
    unsigned char unknown3[8];  /* 40 */
};

struct DevicePointer {
    DeviceInfo* device_info;
    unsigned msg;
};

struct LiveSyncApplicationInfo {
    bool is_livesync_supported;
    std::string application_identifier;
    std::string configuration;
};

typedef unsigned long long afc_file_ref;

struct afc_connection {
    unsigned int handle;            /* 0 */
    unsigned int unknown0;          /* 4 */
    unsigned char unknown1;         /* 8 */
    unsigned char padding[3];       /* 9 */
    unsigned int unknown2;          /* 12 */
    unsigned int unknown3;          /* 16 */
    unsigned int unknown4;          /* 20 */
    unsigned int fs_block_size;     /* 24 */
    unsigned int sock_block_size;   /* 28: always 0x3c */
    unsigned int io_timeout;        /* 32: from AFCConnectionOpen, usu. 0 */
    void *afc_lock;                 /* 36 */
    unsigned int context;           /* 40 */
};

struct afc_dictionary {
    unsigned char unknown[0];   /* size unknown */
};

struct afc_directory {
    unsigned char unknown[0];   /* size unknown */
};

struct afc_file {
	afc_file_ref file_ref;
	afc_connection* afc_conn_p;
};

struct DeviceData {
    DeviceInfo* device_info;
    std::map<const char*, HANDLE> services;
	int sessions;
	afc_connection* afc_conn_p;

	//~DeviceData() {
	//	//device_info
	//}
};

#pragma endregion Data_Structures_Definition

#pragma region Dll_Type_Definitions

typedef unsigned(__cdecl *device_notification_subscribe_ptr)(void(*f)(const DevicePointer*), long, long, long, HANDLE*);

#ifdef _WIN32
typedef void(__cdecl *run_loop_ptr)();
typedef void* CFStringRef;
typedef void* CFURLRef;
typedef void* CFDictionaryRef;
typedef void*(__cdecl *device_copy_device_identifier)(const DeviceInfo*);
typedef void*(__cdecl *device_copy_value)(const DeviceInfo*, CFStringRef, CFStringRef);
typedef unsigned(__cdecl *device_uninstall_application)(HANDLE, CFStringRef, void*, void(*f)(), void*);
typedef unsigned(__cdecl *device_connection_operation)(const DeviceInfo*);
typedef unsigned(__cdecl *device_start_service)(const DeviceInfo*, CFStringRef, HANDLE*, void*);
typedef const char*(__cdecl *cfstring_get_c_string_ptr)(void*, unsigned);
typedef bool(__cdecl *cfstring_get_c_string)(void*, char*, unsigned, unsigned);
typedef CFStringRef(__cdecl *cfstring_create_with_cstring)(void*, const char*, unsigned);
typedef unsigned(__cdecl *device_secure_operation_with_path)(int, const DeviceInfo*, CFURLRef, CFDictionaryRef, void(*f)(), int);
typedef void(__cdecl *cfrelease)(CFStringRef);

typedef CFDictionaryRef(__cdecl *cfdictionary_create)(void *, void*, void*, int, void*, void*);
typedef void*(__cdecl *cfurl_create_with_string)(void *, CFStringRef, void*);

typedef unsigned(__cdecl *afc_connection_open)(HANDLE, const char*, void*);
typedef unsigned(__cdecl *afc_connection_close)(afc_connection*);
typedef unsigned(__cdecl *afc_file_info_open)(afc_connection*, const char*, afc_dictionary**);
typedef unsigned(__cdecl *afc_directory_read)(afc_connection*, afc_directory*, char**);
typedef unsigned(__cdecl *afc_directory_open)(afc_connection*, const char*, afc_directory**);
typedef unsigned(__cdecl *afc_directory_close)(afc_connection*, afc_directory*);
typedef unsigned(__cdecl *afc_directory_create)(afc_connection*, const char *);
typedef unsigned(__cdecl *afc_remove_path)(afc_connection*, const char *);
typedef unsigned(__cdecl *afc_fileref_open)(afc_connection*, const char *, unsigned long long, afc_file_ref*);
typedef unsigned(__cdecl *afc_fileref_read)(afc_connection*, afc_file_ref, void *, size_t*);
typedef unsigned(__cdecl *afc_get_device_info_key)(afc_connection*, const char *, char**);
typedef unsigned(__cdecl *afc_fileref_write)(afc_connection*, afc_file_ref, const void*, size_t);
typedef unsigned(__cdecl *afc_fileref_close)(afc_connection*, afc_file_ref);
typedef unsigned(__cdecl *device_start_house_arrest)(const DeviceInfo*, CFStringRef, void*, HANDLE*, unsigned int*);

#endif // _WIN32
#pragma endregion Dll_Type_Definitions

#pragma region Dll_Variable_Definitions

#ifdef _WIN32
device_notification_subscribe_ptr __AMDeviceNotificationSubscribe;
HINSTANCE mobile_device_dll;
HINSTANCE core_foundation_dll;

device_copy_device_identifier __AMDeviceCopyDeviceIdentifier;
device_copy_value __AMDeviceCopyValue;
device_start_service __AMDeviceStartService;
device_uninstall_application __AMDeviceUninstallApplication;
device_connection_operation __AMDeviceStartSession;
device_connection_operation __AMDeviceStopSession;
device_connection_operation __AMDeviceConnect;
device_connection_operation __AMDeviceDisconnect;
device_connection_operation __AMDeviceIsPaired;
device_connection_operation __AMDevicePair;
device_connection_operation __AMDeviceValidatePairing;
device_secure_operation_with_path __AMDeviceSecureTransferPath;
device_secure_operation_with_path __AMDeviceSecureInstallApplication;
device_start_house_arrest __AMDeviceStartHouseArrestService;

cfstring_get_c_string_ptr __CFStringGetCStringPtr;
cfstring_get_c_string __CFStringGetCString;
cfstring_create_with_cstring __CFStringCreateWithCString;
cfurl_create_with_string __CFURLCreateWithString;
cfdictionary_create __CFDictionaryCreate;
cfrelease __CFRelease;

afc_connection_open __AFCConnectionOpen;
afc_connection_close __AFCConnectionClose;
afc_file_info_open __AFCFileInfoOpen;
afc_directory_read __AFCDirectoryRead;
afc_directory_open __AFCDirectoryOpen;
afc_directory_close __AFCDirectoryClose;
afc_directory_create __AFCDirectoryCreate;
afc_remove_path __AFCRemovePath;
afc_fileref_open __AFCFileRefOpen;
afc_fileref_read __AFCFileRefRead;
afc_get_device_info_key __AFCGetDeviceInfoKey;
afc_fileref_write __AFCFileRefWrite;
afc_fileref_close __AFCFileRefClose;
#endif // _WIN32

#pragma endregion Dll_Variable_Definitions

#pragma region Dll_Method_Definitions

#ifdef _WIN32
#define AMDeviceNotificationSubscribe GET_IF_EXISTS(__AMDeviceNotificationSubscribe, device_notification_subscribe_ptr, mobile_device_dll, "AMDeviceNotificationSubscribe")
#define AMDeviceCopyDeviceIdentifier GET_IF_EXISTS(__AMDeviceCopyDeviceIdentifier, device_copy_device_identifier, mobile_device_dll, "AMDeviceCopyDeviceIdentifier")
#define AMDeviceCopyValue GET_IF_EXISTS(__AMDeviceCopyValue, device_copy_value, mobile_device_dll, "AMDeviceCopyValue")
#define AMDeviceStartService GET_IF_EXISTS(__AMDeviceStartService, device_start_service, mobile_device_dll, "AMDeviceStartService")
#define AMDeviceUninstallApplication GET_IF_EXISTS(__AMDeviceUninstallApplication, device_uninstall_application, mobile_device_dll, "AMDeviceUninstallApplication")
#define AMDeviceStartSession GET_IF_EXISTS(__AMDeviceStartSession, device_connection_operation, mobile_device_dll, "AMDeviceStartSession")
#define AMDeviceStopSession GET_IF_EXISTS(__AMDeviceStopSession, device_connection_operation, mobile_device_dll, "AMDeviceStopSession")
#define AMDeviceConnect GET_IF_EXISTS(__AMDeviceConnect, device_connection_operation, mobile_device_dll, "AMDeviceConnect")
#define AMDeviceDisconnect GET_IF_EXISTS(__AMDeviceDisconnect, device_connection_operation, mobile_device_dll, "AMDeviceDisconnect")
#define AMDeviceIsPaired GET_IF_EXISTS(__AMDeviceIsPaired, device_connection_operation, mobile_device_dll, "AMDeviceIsPaired")
#define AMDevicePair GET_IF_EXISTS(__AMDevicePair, device_connection_operation, mobile_device_dll, "AMDevicePair")
#define AMDeviceValidatePairing GET_IF_EXISTS(__AMDeviceValidatePairing, device_connection_operation, mobile_device_dll, "AMDeviceValidatePairing")
#define AMDeviceSecureTransferPath GET_IF_EXISTS(__AMDeviceSecureTransferPath, device_secure_operation_with_path, mobile_device_dll, "AMDeviceSecureTransferPath")
#define AMDeviceSecureInstallApplication GET_IF_EXISTS(__AMDeviceSecureInstallApplication, device_secure_operation_with_path, mobile_device_dll, "AMDeviceSecureInstallApplication")
#define AMDeviceStartHouseArrestService GET_IF_EXISTS(__AMDeviceStartHouseArrestService, device_start_house_arrest, mobile_device_dll, "AMDeviceStartHouseArrestService")

#define CFStringGetCStringPtr GET_IF_EXISTS(__CFStringGetCStringPtr, cfstring_get_c_string_ptr, core_foundation_dll, "CFStringGetCStringPtr")
#define CFStringGetCString GET_IF_EXISTS(__CFStringGetCString, cfstring_get_c_string, core_foundation_dll, "CFStringGetCString")
#define CFStringCreateWithCString GET_IF_EXISTS(__CFStringCreateWithCString, cfstring_create_with_cstring, core_foundation_dll, "CFStringCreateWithCString")
#define CFURLCreateWithString GET_IF_EXISTS(__CFURLCreateWithString, cfurl_create_with_string, core_foundation_dll, "CFURLCreateWithString")
#define CFDictionaryCreate GET_IF_EXISTS(__CFDictionaryCreate, cfdictionary_create, core_foundation_dll, "CFDictionaryCreate")
#define CFRelease GET_IF_EXISTS(__CFRelease, cfrelease, core_foundation_dll, "CFRelease")

#define AFCConnectionOpen GET_IF_EXISTS(__AFCConnectionOpen, afc_connection_open, mobile_device_dll, "AFCConnectionOpen")
#define AFCConnectionClose GET_IF_EXISTS(__AFCConnectionClose, afc_connection_close, mobile_device_dll, "AFCConnectionClose")
#define AFCRemovePath GET_IF_EXISTS(__AFCRemovePath, afc_remove_path, mobile_device_dll, "AFCRemovePath")
#define AFCFileInfoOpen GET_IF_EXISTS(__AFCFileInfoOpen, afc_file_info_open, mobile_device_dll, "AFCFileInfoOpen")
#define AFCDirectoryRead GET_IF_EXISTS(__AFCDirectoryRead, afc_directory_read, mobile_device_dll, "AFCDirectoryRead")
#define AFCDirectoryOpen GET_IF_EXISTS(__AFCDirectoryOpen, afc_directory_open, mobile_device_dll, "AFCDirectoryOpen")
#define AFCDirectoryClose GET_IF_EXISTS(__AFCDirectoryClose, afc_directory_close, mobile_device_dll, "AFCDirectoryClose")
#define AFCDirectoryCreate GET_IF_EXISTS(__AFCDirectoryCreate, afc_directory_create, mobile_device_dll, "AFCDirectoryCreate")
#define AFCFileRefOpen GET_IF_EXISTS(__AFCFileRefOpen, afc_fileref_open, mobile_device_dll, "AFCFileRefOpen")
#define AFCFileRefRead GET_IF_EXISTS(__AFCFileRefRead, afc_fileref_read, mobile_device_dll, "AFCFileRefRead")
#define AFCGetDeviceInfoKey GET_IF_EXISTS(__AFCGetDeviceInfoKey, afc_get_device_info_key, mobile_device_dll, "AFCGetDeviceInfoKey")
#define AFCFileRefWrite GET_IF_EXISTS(__AFCFileRefWrite, afc_fileref_write, mobile_device_dll, "AFCFileRefWrite")
#define AFCFileRefClose GET_IF_EXISTS(__AFCFileRefClose, afc_fileref_close, mobile_device_dll, "AFCFileRefClose")
#endif // _WIN32


#pragma endregion Dll_Method_Definitions

#ifndef _WIN32

#include <CoreFoundation/CoreFoundation.h>

extern "C"
{
    unsigned AMDeviceNotificationSubscribe(void(*f)(const DevicePointer*), long, long, long, HANDLE*);
    CFStringRef AMDeviceCopyDeviceIdentifier(const DeviceInfo*);
    CFStringRef AMDeviceCopyValue(const DeviceInfo*, CFStringRef, CFStringRef);
    unsigned AMDeviceStartService(const DeviceInfo*, CFStringRef, HANDLE*, void*);
    unsigned AMDeviceUninstallApplication(HANDLE, CFStringRef, void*, void(*f)(), void*);
    unsigned AMDeviceStartSession(const DeviceInfo*);
    unsigned AMDeviceStopSession(const DeviceInfo*);
    unsigned AMDeviceConnect(const DeviceInfo*);
    unsigned AMDeviceDisconnect(const DeviceInfo*);
    unsigned AMDeviceIsPaired(const DeviceInfo*);
    unsigned AMDevicePair(const DeviceInfo*);
    unsigned AMDeviceValidatePairing(const DeviceInfo*);
    unsigned AMDeviceSecureTransferPath(int, const DeviceInfo*, CFURLRef, CFDictionaryRef, void(*f)(), int);
    unsigned AMDeviceSecureInstallApplication(int, const DeviceInfo*, CFURLRef, CFDictionaryRef, void(*f)(), int);
    unsigned AMDeviceStartHouseArrestService(const DeviceInfo*, CFStringRef, void*, HANDLE*, unsigned int*);
    unsigned AFCConnectionOpen(HANDLE, const char*, void*);
    unsigned AFCConnectionClose(afc_connection*);
    unsigned AFCRemovePath(afc_connection*, const char*);
    unsigned AFCFileInfoOpen(afc_connection*, const char*, afc_dictionary**);
    unsigned AFCDirectoryRead(afc_connection*, afc_directory*, char**);
    unsigned AFCDirectoryOpen(afc_connection*, const char*, afc_directory**);
    unsigned AFCDirectoryClose(afc_connection*, afc_directory*);
    unsigned AFCDirectoryCreate(afc_connection*, const char*);
    unsigned AFCFileRefOpen(afc_connection*, const char*, unsigned long long, afc_file_ref*);
    unsigned AFCFileRefRead(afc_connection*, afc_file_ref, void*, size_t*);
    unsigned AFCFileRefWrite(afc_connection*, afc_file_ref, const void*, size_t);
    unsigned AFCFileRefClose(afc_connection*, afc_file_ref);

}

#endif // !_WIN32

#endif /* Declarations_h */
