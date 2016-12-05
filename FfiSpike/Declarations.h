#ifndef Declarations_h
#define Declarations_h

struct DeviceInfo;
#ifndef _WIN32
typedef void* HANDLE;
typedef unsigned long long SOCKET;
#endif


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

struct DeviceData {
    DeviceInfo* device_info;
    std::map<const char*, HANDLE> services;
    std::vector<LiveSyncApplicationInfo> livesync_app_infos;
};

#pragma endregion Data_Structures_Definition

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
    unsigned AFCFileRefRead(afc_connection*, afc_file_ref*, void*, unsigned);
    unsigned AFCFileRefWrite(afc_connection*, afc_file_ref, const void*, size_t);
    unsigned AFCFileRefClose(afc_connection*, afc_file_ref);

}
#endif /* Declarations_h */
