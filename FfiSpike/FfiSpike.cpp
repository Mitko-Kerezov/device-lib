#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <sys/stat.h>
#include <sys/socket.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#endif

#include "json.hpp"
#include "PlistCpp/Plist.hpp"
#include "PlistCpp/PlistDate.hpp"
#include "PlistCpp/include/boost/any.hpp"

#ifndef _WIN32
#include <CoreFoundation/CoreFoundation.h>
#include "Declarations.h"
#endif

#pragma region Constants
const unsigned kADNCIMessageConnected = 1;
const unsigned kADNCIMessageDisconnected = 2;
const unsigned kADNCIMessageUnknown = 3;
const unsigned kADNCIMessageTrusted = 4;
const unsigned kDeviceLogBytesToRead = 1 << 10;
#ifdef _WIN32
const unsigned kCFStringEncodingUTF8 = 0x08000100;
#endif
const unsigned kAMDNotFoundError = 0xe8000008;
const unsigned kAMDAPIInternalError = 0xe8000067;
const unsigned kAppleServiceNotStartedErrorCode = 0xE8000063;
const char *kAppleFileConnection = "com.apple.afc";
const char *kInstallationProxy = "com.apple.mobile.installation_proxy";
const char *kHouseArrest = "com.apple.mobile.house_arrest";
const char *kNotificationProxy = "com.apple.mobile.notification_proxy";
const char *kSyslog = "com.apple.syslog_relay";
const char *kMobileImageMounter = "com.apple.mobile.mobile_image_mounter";
const char *kDebugServer = "com.apple.debugserver";

const char *kUnreachableStatus = "Unreachable";
const char *kConnectedStatus = "Connected";

const char *kDeviceFound = "deviceFound";
const char *kDeviceLost = "deviceLost";
const char *kDeviceTrusted = "deviceTrusted";
const char *kDeviceUnknown = "deviceUnknown";

const char *kId = "id";
const char *kNullMessageId = "null";
const char *kError = "error";
const char *kMessage = "message";
const char *kCode = "code";
const int kAFCFileModeRead = 2;
const int kAFCFileModeWrite = 3;
const char *kPathSeparators = "/\\";
const char kUnixPathSeparator = '/';
const char *kEventString = "event";
const char *kRefreshAppMessage = "com.telerik.app.refreshApp";

const std::string file_prefix("file:///");


#pragma endregion Constants

#define RETURN_IF_FAILED_RESULT(expr) ; if((__result = (expr))) { return __result; }
#define PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(expr, error_msg, method_id, value) ; if((__result = (expr))) { print_error(error_msg, method_id, __result); return value; }
#define PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(expr, error_msg, method_id) ; if(((expr))) { print_error(error_msg, method_id, __result); return; }
#define GET_IF_EXISTS(variable, type, dll, method_name) (variable ? variable : variable = (type)GetProcAddress(dll, method_name))

using json = nlohmann::json;

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
typedef unsigned(__cdecl *device_start_house_arrest)(DeviceInfo*, CFStringRef, void*, HANDLE*, unsigned int*);

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
typedef unsigned(__cdecl *afc_fileref_read)(afc_connection*, afc_file_ref*, void *, unsigned);
typedef unsigned(__cdecl *afc_fileref_write)(afc_connection*, afc_file_ref, const void*, size_t);
typedef unsigned(__cdecl *afc_fileref_close)(afc_connection*, afc_file_ref);

#endif
#pragma endregion Dll_Type_Definitions

#pragma region Dll_Variable_Definitions

#ifdef _WIN32
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
afc_fileref_write __AFCFileRefWrite;
afc_fileref_close __AFCFileRefClose;
#endif

#pragma endregion Dll_Variable_Definitions

#pragma region Dll_Method_Definitions

#ifdef _WIN32
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
#define AFCFileRefWrite GET_IF_EXISTS(__AFCFileRefWrite, afc_fileref_write, mobile_device_dll, "AFCFileRefWrite")
#define AFCFileRefClose GET_IF_EXISTS(__AFCFileRefClose, afc_fileref_close, mobile_device_dll, "AFCFileRefClose")
#endif

#pragma endregion Dll_Method_Definitions

struct cmp_str
{
	bool operator()(char const *a, char const *b) const
	{
		return std::strcmp(a, b) < 0;
	}
};

int __result;
std::map<const char*, DeviceData, cmp_str> devices;

LengthEncodedMessage get_message_with_encoded_length(const char* message)
{
	size_t original_message_len = strlen(message);
	unsigned long message_len = original_message_len + 4;
	char *length_encoded_message = new char[message_len];
	unsigned long packed_length = htonl(original_message_len);
    size_t packed_length_size = 4;// ssizeof(packed_length);
	memcpy(length_encoded_message, &packed_length, packed_length_size);
	memcpy(length_encoded_message + packed_length_size, message, original_message_len);
	return{ length_encoded_message, message_len };
}

void print(const char* str)
{
	LengthEncodedMessage length_encoded_message = get_message_with_encoded_length(str);
	fwrite(length_encoded_message.message, length_encoded_message.length, 1, stdout);
	fflush(stdout);
}

void print(json message)
{
	std::string str = message.dump();
	print(str.c_str());
}

std::string get_dirname(const char *path)
{
	size_t found;
	std::string str(path);
	found = str.find_last_of(kPathSeparators);
	return str.substr(0, found);
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void windows_path_to_unix(std::string& path)
{
	replace_all(path, "\\", "/");
}

std::string windows_path_to_unix(const char *path)
{
	std::string path_str(path);
	replace_all(path_str, "\\", "/");
	return path_str;
}

const char* get_cstring_from_cfstring(CFStringRef cfstring)
{
	const char* device_identifier = CFStringGetCStringPtr(cfstring, kCFStringEncodingUTF8);
	char* device_identifier_buffer = new char[200];
	if (device_identifier == NULL)
	{
		if (CFStringGetCString(cfstring, device_identifier_buffer, 200, kCFStringEncodingUTF8))
		{
			return device_identifier_buffer;
		}
	}

	return device_identifier;
}

CFStringRef create_CFString(const char* str)
{
	return CFStringCreateWithCString(NULL, str, kCFStringEncodingUTF8);
}

int start_session(const DeviceInfo* device_info)
{
	RETURN_IF_FAILED_RESULT(AMDeviceConnect(device_info));
	if (!AMDeviceIsPaired(device_info))
	{
		RETURN_IF_FAILED_RESULT(AMDevicePair(device_info));
	}

	RETURN_IF_FAILED_RESULT(AMDeviceValidatePairing(device_info));
	RETURN_IF_FAILED_RESULT(AMDeviceStartSession(device_info));

	return 0;
}

void stop_session(const DeviceInfo *device_info)
{
	AMDeviceStopSession(device_info);
	AMDeviceDisconnect(device_info);
}

const char *get_device_property_value(const DeviceInfo* device_info, char* property_name) 
{
	const char *result = "";
	if (!start_session(device_info))
	{
		CFStringRef cfstring = create_CFString(property_name);
		result = get_cstring_from_cfstring(AMDeviceCopyValue(device_info, NULL, cfstring));
		CFRelease(cfstring);
	}

	stop_session(device_info);
	return result;
}

const char *get_device_status(const DeviceInfo* device_info)
{
	const char *result;
	if (start_session(device_info))
	{
		result = kUnreachableStatus;
	}
	else
	{
		result = kConnectedStatus;
	}

	AMDeviceStopSession(device_info);
	AMDeviceDisconnect(device_info);
	return result;
}

void get_device_properties(const DeviceInfo* device_info, json &result)
{
	result["status"] = get_device_status(device_info);
    result["product_type"] = get_device_property_value(device_info, "ProductType");
	result["device_name"] = get_device_property_value(device_info, "DeviceName");
	result["product_version"] = get_device_property_value(device_info, "ProductVersion");
	result["device_color"] = get_device_property_value(device_info, "DeviceColor");
}

void device_notification_callback(const DevicePointer* device_ptr)
{
	const char *device_identifier = get_cstring_from_cfstring(AMDeviceCopyDeviceIdentifier(device_ptr->device_info));
	json result;
	result["device_identifier"] = device_identifier;
	switch (device_ptr->msg)
	{
		case kADNCIMessageConnected:
		{
			devices[device_identifier] = { device_ptr->device_info };
			result[kEventString] = kDeviceFound;
			get_device_properties(device_ptr->device_info, result);
			break;
		}
		case kADNCIMessageDisconnected:
		{
			if (devices.count(device_identifier))
			{
				devices.erase(device_identifier);
			}
			result[kEventString] = kDeviceLost;
			break;
		}
		case kADNCIMessageUnknown:
		{
			result[kEventString] = kDeviceUnknown;
			break;
		}
		case kADNCIMessageTrusted:
		{
			devices[device_identifier] = { device_ptr->device_info };
			result[kEventString] = kDeviceTrusted;
			get_device_properties(device_ptr->device_info, result);
			break;
		}
	}

	print(result);
}


void print_error(const char *message, std::string method_id, int code =
#ifdef _WIN32
                 GetLastError()
#else
                 kAMDNotFoundError
#endif

)
{
    json exception;
    exception[kError][kMessage] = message;
    exception[kError][kCode] = code;
    exception[kId] = method_id;
    //Decided it's a better idea to print everything to stdout
    //Not a good practice, but the client process wouldn't have to monitor both streams
    //fprintf(stderr, "%s", exception.dump().c_str());
    print(exception);
}

#ifdef _WIN32
void add_dll_paths_to_environment()
{
	std::string str(std::getenv("PATH"));
	str = str.append(";C:\\Program Files\\Common Files\\Apple\\Apple Application Support;C:\\Program Files\\Common Files\\Apple\\Mobile Device Support;");
	str = str.append(";C:\\Program Files (x86)\\Common Files\\Apple\\Apple Application Support;C:\\Program Files (x86)\\Common Files\\Apple\\Mobile Device Support;");

	SetEnvironmentVariable("PATH", str.c_str());
}

int load_dlls()
{
	add_dll_paths_to_environment();
	mobile_device_dll = LoadLibrary("MobileDevice.dll");
	if (!mobile_device_dll)
	{
		print_error("Could not load MobileDevice.dll", kNullMessageId);
		return 1;
	}

	core_foundation_dll = LoadLibrary("CoreFoundation.dll");
	if (!core_foundation_dll)
	{
		print_error("Could not load CoreFoundation.dll", kNullMessageId);
		return 1;
	}

	return 0;
}

#endif

int subscribe_for_notifications()
{
	HANDLE notify_function = NULL;
	int result = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0, &notify_function);
	if (result)
	{
		print_error("Could not attach notification callback", kNullMessageId);
	}

	return result;
}

void start_run_loop()
{
	int subscribe_for_notifications_result = subscribe_for_notifications();

	if (subscribe_for_notifications_result)
	{
		return;
	}
    
#ifdef _WIN32
	run_loop_ptr CFRunLoopRun = (run_loop_ptr)GetProcAddress(core_foundation_dll, "CFRunLoopRun");
#endif
	CFRunLoopRun();
}

HANDLE start_service(const char* device_identifier, const char* service_name, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return NULL;
	}

	if (devices[device_identifier].services.count(service_name))
	{
		return devices[device_identifier].services[service_name];
	}

	HANDLE socket = NULL;
	PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(start_session(devices[device_identifier].device_info), "Could start device session", method_id, NULL);
	CFStringRef cf_service_name = create_CFString(service_name);
	unsigned result = AMDeviceStartService(devices[device_identifier].device_info, cf_service_name, &socket, NULL);
	CFRelease(cf_service_name);
	if (result)
	{
		std::string message("Could not start service ");
		message += service_name;
		print_error(message.c_str(), method_id, result);
		return NULL;
	}

	devices[device_identifier].services[service_name] = socket;
	return socket;
}

HANDLE start_house_arrest(const char* device_identifier, const char* application_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return NULL;
	}

	if (devices[device_identifier].services.count(kHouseArrest))
	{
		return devices[device_identifier].services[kHouseArrest];
	}

	HANDLE houseFd = NULL;
	start_session(devices[device_identifier].device_info);
	CFStringRef cf_application_identifier = create_CFString(application_identifier);
	unsigned result = AMDeviceStartHouseArrestService(devices[device_identifier].device_info, cf_application_identifier, 0, &houseFd, 0);
	CFRelease(cf_application_identifier);
	stop_session(devices[device_identifier].device_info);
	
	if (result)
	{
		std::string message("Could not start house arrest for application ");
		message += application_identifier;
		print_error(message.c_str(), method_id, result);
		return NULL;
	}

	devices[device_identifier].services[kHouseArrest] = houseFd;
	return houseFd;
}

afc_connection *get_afc_connection(const char* device_identifier, const char* application_identifier, std::string method_id)
{
	if (devices.count(device_identifier) && devices[device_identifier].services.count(kAppleFileConnection))
	{
		return (afc_connection *)devices[device_identifier].services[kAppleFileConnection];
	}

	HANDLE houseFd = start_house_arrest(device_identifier, application_identifier, method_id);
	if (!houseFd)
	{
		return NULL;
	}

	afc_connection afc_conn;
	afc_connection* afc_conn_p = &afc_conn;
	PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(AFCConnectionOpen(houseFd, 0, &afc_conn_p), "Could not open afc connection", method_id, NULL);
	devices[device_identifier].services[kAppleFileConnection] = afc_conn_p;
	return afc_conn_p;
}

void uninstall_application(std::string application_identifier, const char* device_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return;
	}

	HANDLE socket = start_service(device_identifier, kInstallationProxy, method_id);
	if (!socket)
	{
		return;
	}

	CFStringRef appid_cfstring = create_CFString(application_identifier.c_str());
	unsigned result = AMDeviceUninstallApplication(socket, appid_cfstring, NULL, [] {}, NULL);
	CFRelease(appid_cfstring);
	stop_session(devices[device_identifier].device_info);

	if (result)
	{
		print_error("Could not uninstall application", method_id, result);
	}
	else
	{
		print(json({{ "response", "Successfully uninstalled application" }, { kId, method_id }}));
	}
}

void install_application(std::string install_path, const char* device_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return;
	}

#ifdef _WIN32
	if (install_path.compare(0, file_prefix.size(), file_prefix))
	{
		install_path = file_prefix + install_path;
	}
	windows_path_to_unix(install_path);
#endif
	start_session(devices[device_identifier].device_info);

	CFStringRef path = create_CFString(install_path.c_str());
	CFURLRef local_app_url =
#ifdef _WIN32
    CFURLCreateWithString(NULL, path, NULL)
#else
    CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, true);
#endif
	CFRelease(path);
	if (!local_app_url) {
		stop_session(devices[device_identifier].device_info);
		print_error("Could not parse application path", method_id, kAMDAPIInternalError);
		return;
	}

	CFStringRef cf_package_type = create_CFString("PackageType");
	CFStringRef cf_developer = create_CFString("Developer");
	const void *keys_arr[] = { cf_package_type };
	const void *values_arr[] = { cf_developer };
	CFDictionaryRef options = CFDictionaryCreate(NULL, keys_arr, values_arr, 1, NULL, NULL);

	unsigned transfer_result = AMDeviceSecureTransferPath(0, devices[device_identifier].device_info, local_app_url, options, NULL, 0);
	if (transfer_result) {
		stop_session(devices[device_identifier].device_info);
		print_error("Could not transfer application", method_id, transfer_result);
		return;
	}

	unsigned install_result = AMDeviceSecureInstallApplication(0, devices[device_identifier].device_info, local_app_url, options, NULL, 0);
	CFRelease(cf_package_type);
	CFRelease(cf_developer);
	stop_session(devices[device_identifier].device_info);

	if (install_result)
	{
		print_error("Could not install application", method_id, install_result);
		return;
	}

	print(json({{ "response", "Successfully installed application" }, { kId, method_id }}));
}

void perform_detached_operation(void(*operation)(const char*, std::string), std::string arg, std::string method_id)
{
	std::thread([operation, arg, method_id]() { operation(arg.c_str(), method_id);  }).detach();
}

void perform_detached_operation(void(*operation)(std::string, const char*, std::string), json method_args, std::string method_id)
{
	std::string first_arg = method_args[0].get<std::string>();
	std::vector<std::string> device_identifiers = method_args[1].get<std::vector<std::string>>();
	for (std::string device_identifier : device_identifiers)
		std::thread([operation, first_arg, device_identifier, method_id]() { operation(first_arg, device_identifier.c_str(), method_id);  }).detach();
}

void read_dir(HANDLE afcFd, afc_connection* afc_conn_p, const char* dir, json &files, std::string method_id)
{
	char *dir_ent;
	files.push_back(dir);

	afc_dictionary afc_dict;
	afc_dictionary* afc_dict_p = &afc_dict;
	unsigned afc_file_info_open_result = AFCFileInfoOpen(afc_conn_p, dir, &afc_dict_p);
	if (afc_file_info_open_result)
	{
		std::string message("Could not open file info for file: ");
		message += dir;
		print_error(message.c_str(), method_id, afc_file_info_open_result);
		return;
	}

	afc_directory afc_dir;
	afc_directory* afc_dir_p = &afc_dir;
	unsigned err = AFCDirectoryOpen(afc_conn_p, dir, &afc_dir_p);

	if (err != 0)
	{
		// Couldn't open dir - was probably a file
		return;
	}

	while (true) {
		err = AFCDirectoryRead(afc_conn_p, afc_dir_p, &dir_ent);

		if (!dir_ent)
			break;

		if (strcmp(dir_ent, ".") == 0 || strcmp(dir_ent, "..") == 0)
			continue;

		char* dir_joined = (char*)malloc(strlen(dir) + strlen(dir_ent) + 2);
		strcpy(dir_joined, dir);
		if (dir_joined[strlen(dir) - 1] != '/')
			strcat(dir_joined, "/");
		strcat(dir_joined, dir_ent);
		read_dir(afcFd, afc_conn_p, dir_joined, files, method_id);
		free(dir_joined);
	}

	unsigned afc_directory_close_result = AFCDirectoryClose(afc_conn_p, afc_dir_p);
	if (afc_directory_close_result)
	{
		print_error("Could not close directory", method_id, afc_directory_close_result);
	}
}

void list_files(const char *device_identifier, const char *application_identifier, const char *device_path, std::string method_id)
{
	HANDLE houseFd = start_house_arrest(device_identifier, application_identifier, method_id);
	if (!houseFd)
	{
		return;
	}

	afc_connection afc_conn;
	afc_connection* afc_conn_p = &afc_conn;
	unsigned afc_connection_open_result = AFCConnectionOpen(houseFd, 0, &afc_conn_p);
	if (afc_connection_open_result)
	{
		print_error("Could not establish AFC Connection", method_id, afc_connection_open_result);
		return;
	}
	json files;
	
	std::string device_path_str = windows_path_to_unix(device_path);
	read_dir(houseFd, afc_conn_p, device_path_str.c_str(), files, method_id);
	print(json({{ "response", files }, { kId, method_id }}));
}

void* read_file_to_memory(char * path, size_t* file_size)
{
	struct stat buf;
	int err = stat(path, &buf);
	if (err < 0)
	{
		return NULL;
	}

	*file_size = buf.st_size;
	FILE* fd = fopen(path, "r");
	char* content = (char*)malloc(*file_size);
	if (fread(content, *file_size, 1, fd) != 1)
	{
		fclose(fd);
		return NULL;
	}
	fclose(fd);
	return content;
}

bool ensure_device_path_exists(std::string &device_path, afc_connection *connection, std::string method_id)
{
	std::vector<std::string> directories = split(device_path, kUnixPathSeparator);
	std::string curent_device_path("");
	for (std::string directory_path : directories)
	{
		if (!directory_path.empty())
		{
			curent_device_path += kUnixPathSeparator;
			curent_device_path += directory_path;
			PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(AFCDirectoryCreate(connection, curent_device_path.c_str()), "Unable to make directory", method_id, false)
		}
	}

	return true;
}

void upload_file(const char *device_identifier, const char *application_identifier, const char *source, const char *destination, std::string method_id) {
	afc_file_ref file_ref;
	afc_connection* afc_conn_p = get_afc_connection(device_identifier, application_identifier, method_id);
	if (!afc_conn_p)
	{
		return;
	}

	std::ifstream file(source, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> file_content(size);
	if (file.read(file_content.data(), size))
	{
		std::string destination_str =  windows_path_to_unix(destination);
		destination = destination_str.c_str();
		std::string dir_name = get_dirname(destination);
		if (ensure_device_path_exists(dir_name, afc_conn_p, method_id))
		{
			std::stringstream error_message;
			AFCRemovePath(afc_conn_p, destination);
			error_message << "Could not open file " << destination << " for writing";
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefOpen(afc_conn_p, destination, kAFCFileModeWrite, &file_ref), error_message.str().c_str(), method_id);
			error_message.str(std::string());
			error_message << "Could not write to file: " << destination;
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefWrite(afc_conn_p, file_ref, &file_content[0], file_content.size()), error_message.str().c_str(), method_id);
			error_message.str(std::string());
			error_message << "Could not close file reference: " << destination;
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefClose(afc_conn_p, file_ref), error_message.str().c_str(), method_id);
			error_message.str(std::string());
			print(json({{ "response", "Successfully uploaded file" },{ kId, method_id }}));
		}
	}
	else 
	{
		std::string message("Could not open file: ");
		message += source;
		print_error(message.c_str(), method_id, kAMDAPIInternalError);
	}
}



void delete_file(const char *device_identifier, const char *application_identifier, const char *destination, std::string method_id) {
	afc_connection* afc_conn_p = get_afc_connection(device_identifier, application_identifier, method_id);
	if (!afc_conn_p)
	{
		return;
	}

	std::string destination_str = windows_path_to_unix(destination);
	destination = destination_str.c_str();
	std::stringstream error_message;
	error_message << "Could not remove file " << destination;
	PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCRemovePath(afc_conn_p, destination), error_message.str().c_str(), method_id);
	print(json({{ "response", "Successfully removed file" },{ kId, method_id }}));
}

void read_file(const char *device_identifier, const char *application_identifier, const char *destination, std::string method_id) {
	afc_file_ref file_ref;
	afc_connection* afc_conn_p = get_afc_connection(device_identifier, application_identifier, method_id);
	if (!afc_conn_p)
	{
		return;
	}

	std::string destination_str = windows_path_to_unix(destination);
	destination = destination_str.c_str();
	std::stringstream error_message;
	AFCRemovePath(afc_conn_p, destination);
	error_message << "Could not open file " << destination << " for writing";
	PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefOpen(afc_conn_p, destination, kAFCFileModeRead, &file_ref), error_message.str().c_str(), method_id);
	unsigned size_to_read = 1 << 3;
	void *buf = malloc(sizeof(size_t) * size_to_read);
	AFCFileRefRead(afc_conn_p, &file_ref, buf, size_to_read);

	print(json({{ "response", "Successfully uploaded file" },{ kId, method_id }}));
}

LiveSyncApplicationInfo* get_applications_livesync_supported_status(std::string application_identifier, const char *device_identifier, std::string method_id)
{
	HANDLE socket = start_service(device_identifier, kInstallationProxy, method_id);
	if (!socket)
	{
		return NULL;
	}

	int result_index = -1;
	int current_index = -1;
	devices[device_identifier].livesync_app_infos.clear();
	const char *xml_command = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
						"<plist version=\"1.0\">"
						"<dict>"
							"<key>Command</key>"
							"<string>Browse</string>"
							"<key>ClientOptions</key>"
							"<dict>"
								"<key>ApplicationType</key>"
								"<string>User</string>"
								"<key>ReturnAttributes</key>"
								"<array>"
									"<string>CFBundleIdentifier</string>"
									"<string>IceniumLiveSyncEnabled</string>"
									"<string>configuration</string>"
								"</array>"
							"</dict>"
						"</dict>"
						"</plist>";
	
	LengthEncodedMessage length_encoded_message = get_message_with_encoded_length(xml_command);
	int bytes_sent = send((SOCKET)socket, length_encoded_message.message, length_encoded_message.length, 0);
	while (true)
	{
		std::map<std::string, boost::any> dict;
		char *buffer = new char[4];
		int bytes_read = recv((SOCKET)socket, buffer, 4, 0);
		unsigned long res = ntohl(*((unsigned long*)buffer));
		free(buffer);
		buffer = new char[res];
		bytes_read = recv((SOCKET)socket, buffer, res, 0);
		Plist::readPlist(buffer, res, dict);
		free(buffer);
		PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(dict.count("Error"), boost::any_cast<std::string>(dict["Error"]).c_str(), method_id, NULL);
		if (dict.count("Status") && boost::any_cast<std::string>(dict["Status"]) == "Complete")
		{
			break;
		}
		std::vector<boost::any> current_list = boost::any_cast<std::vector<boost::any>>(dict["CurrentList"]);
		for (boost::any list : current_list)
		{
			std::map<std::string, boost::any> app_info = boost::any_cast<std::map<std::string, boost::any>>(list);
			LiveSyncApplicationInfo current_info = {
				app_info.count("IceniumLiveSyncEnabled") && boost::any_cast<bool>(app_info["IceniumLiveSyncEnabled"]),
				app_info.count("CFBundleIdentifier") ? boost::any_cast<std::string>(app_info["CFBundleIdentifier"]) : "",
				app_info.count("configuration") ? boost::any_cast<std::string>(app_info["configuration"]) : "",
			};
			devices[device_identifier].livesync_app_infos.push_back(current_info);
			++current_index;
			if (current_info.application_identifier == application_identifier)
			{
				result_index = current_index;
			}
		}
	}

	return result_index == -1 ? NULL : &devices[device_identifier].livesync_app_infos[result_index];
}

json to_json(LiveSyncApplicationInfo* info, const char* device_identifier)
{
	json result;
	result["device_identifier"] = device_identifier;
	result["configuration"] = info->configuration;
	result["is_livesync_supported"] = info->is_livesync_supported;
	return result;
}

void is_app_installed(std::string application_identifier, const char* device_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return;
	}

	for (LiveSyncApplicationInfo livesync_app_info : devices[device_identifier].livesync_app_infos)
	{
		if (livesync_app_info.application_identifier == application_identifier)
		{
			json livesync_app_info_json = to_json(&livesync_app_info, device_identifier);
			livesync_app_info_json[kId] = method_id;
			print(livesync_app_info_json);
			return;
		}
	}

	LiveSyncApplicationInfo *livesync_app_info = get_applications_livesync_supported_status(application_identifier, device_identifier, method_id);
	if (livesync_app_info)
	{
		json livesync_app_info_json = to_json(livesync_app_info, device_identifier);
		livesync_app_info_json[kId] = method_id;
		print(livesync_app_info_json);
	}
	else
	{
		print_error("App not installed", method_id, kAMDNotFoundError);
	}
}

void device_log(const char* device_identifier, std::string method_id)
{
	HANDLE socket = start_service(device_identifier, kSyslog, method_id);
	if (!socket)
	{
		return;
	}

	char *buffer = new char[kDeviceLogBytesToRead];
	int bytes_read; 
	while (bytes_read = recv((SOCKET)socket, buffer, kDeviceLogBytesToRead, 0))
	{
		json message;
		message["deviceId"] = device_identifier;
		message[kMessage] = buffer;
		message[kId] = method_id;
		print(message);
	}

	free(buffer);
}

void post_notification(const char* device_identifier, std::string notification_name, std::string method_id)
{
	HANDLE socket = start_service(device_identifier, kNotificationProxy, method_id);
	if (!socket)
	{
		return;
	}

	if (!devices.count(device_identifier))
	{
		print_error("Device not found", method_id, kAMDNotFoundError);
		return;
	}

	std::stringstream xml_command;
	xml_command <<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
						"<plist version=\"1.0\">"
						"<dict>"
							"<key>Command</key>"
							"<string>PostNotification</string>"
							"<key>Name</key>"
							"<string>" + notification_name + "</string>"
							"<key>ClientOptions</key>"
							"<string></string>"
						"</dict>"
						"</plist>";
	
	LengthEncodedMessage length_encoded_message = get_message_with_encoded_length(xml_command.str().c_str());
	int bytes_sent = send((SOCKET)socket, length_encoded_message.message, length_encoded_message.length, 0);
}

int main()
{
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
    
	RETURN_IF_FAILED_RESULT(load_dlls());
#endif
    
	std::thread([]() { start_run_loop(); }).detach();
	
	for (std::string line; std::getline(std::cin, line);)
	{
		json message = json::parse(line.c_str());
		for (json method : message.value("methods", json::array()))
		{
			std::string method_name = method.value("name", "");
			std::string method_id = method.value("id", "");
			std::vector<json> method_args = method.value("args", json::array()).get<std::vector<json>>();
			if (method_name == "install")
			{
				perform_detached_operation(install_application, method_args, method_id);
			}
			if (method_name == "uninstall")
			{
				perform_detached_operation(uninstall_application, method_args, method_id);
			}
			if (method_name == "list")
			{
				for (json arg : method_args)
				{
					std::string application_identifier = arg.value("appId", "");
					std::string device_identifier = arg.value("deviceId", "");
					std::string path = arg.value("path", "");
					list_files(device_identifier.c_str(), application_identifier.c_str(), path.c_str(), method_id);
				}
			}
			if (method_name == "upload")
			{
				for (json arg : method_args)
				{
					std::string application_identifier = arg.value("appId", "");
					std::string device_identifier = arg.value("deviceId", "");
					std::string source = arg.value("source", "");
					std::string destination = arg.value("destination", "");
//					std::vector<char> source_writable(source.begin(), source.end());
//					source_writable.push_back('\0');
					upload_file(device_identifier.c_str(), application_identifier.c_str(), source.c_str(), destination.c_str(), method_id);
				}
			}
			if (method_name == "delete")
			{
				for (json arg : method_args)
				{
					std::string application_identifier = arg.value("appId", "");
					std::string device_identifier = arg.value("deviceId", "");
					std::string destination = arg.value("destination", "");
					delete_file(device_identifier.c_str(), application_identifier.c_str(), destination.c_str(), method_id);
				}
			}
			///*if (method_name == "read")
			//{
			//	for (json arg : method_args)
			//	{
			//		std::string application_identifier = arg.value("appId", "");
			//		std::string device_identifier = arg.value("deviceId", "");
			//		std::string destination = arg.value("destination", "");
			//		read_file(device_identifier.c_str(), application_identifier.c_str(), destination.c_str());
			//	}
			//}*/
			if (method_name == "isInstalled")
			{
				perform_detached_operation(is_app_installed, method_args, method_id);
			}
			if (method_name == "log")
			{
				std::string device_identifier = method_args[0].get<std::string>();
				perform_detached_operation(device_log, device_identifier, method_id);
			}
			if (method_name == "notify")
			{
				for (json arg : method_args)
				{
					std::string device_identifier = arg.value("deviceId", "");
					std::string notification_name = arg.value("notificationName", "");
					post_notification(device_identifier.c_str(), notification_name.c_str(), method_id);
				}
			}
			if (method_name == "exit")
			{
				return 0;
			}
		}
	}

	return 0;
}
