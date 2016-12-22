#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <sys/stat.h>
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
#include "Declarations.h"

#ifndef _WIN32
#include <sys/socket.h>
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
const char *kDeviceId = "deviceId";
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
#define PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(expr, error_msg, device_identifier, method_id, value) ; if((__result = (expr))) { print_error(error_msg, device_identifier, method_id, __result); return value; }
#define PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(expr, error_msg, device_identifier, method_id) ; if((__result = (expr))) { print_error(error_msg, device_identifier, method_id, __result); return; }
#define GET_IF_EXISTS(variable, type, dll, method_name) (variable ? variable : variable = (type)GetProcAddress(dll, method_name))

using json = nlohmann::json;

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

void print(const json& message)
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

	stop_session(device_info);
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
	result[kDeviceId] = device_identifier;
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


void print_error(const char *message, const char* device_identifier, std::string method_id, int code =
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
	exception[kDeviceId] = device_identifier;
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
		print_error("Could not load MobileDevice.dll", kNullMessageId, kNullMessageId);
		return 1;
	}

	core_foundation_dll = LoadLibrary("CoreFoundation.dll");
	if (!core_foundation_dll)
	{
		print_error("Could not load CoreFoundation.dll", kNullMessageId, kNullMessageId);
		return 1;
	}

	return 0;
}

#endif // _WIN32

int subscribe_for_notifications()
{
	HANDLE notify_function = NULL;
	int result = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0, &notify_function);
	if (result)
	{
		print_error("Could not attach notification callback", kNullMessageId, kNullMessageId);
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
		print_error("Device not found", device_identifier, method_id, kAMDNotFoundError);
		return NULL;
	}

	if (devices[device_identifier].services.count(service_name))
	{
		return devices[device_identifier].services[service_name];
	}

	HANDLE socket = NULL;
	PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(start_session(devices[device_identifier].device_info), "Could not start device session", device_identifier, method_id, NULL);
	CFStringRef cf_service_name = create_CFString(service_name);
	unsigned result = AMDeviceStartService(devices[device_identifier].device_info, cf_service_name, &socket, NULL);
	stop_session(devices[device_identifier].device_info);
	CFRelease(cf_service_name);
	if (result)
	{
		std::string message("Could not start service ");
		message += service_name;
		print_error(message.c_str(), device_identifier, method_id, result);
		return NULL;
	}
	devices[device_identifier].services[service_name] = socket;

	return socket;
}

HANDLE start_house_arrest(const char* device_identifier, const char* application_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", device_identifier, method_id, kAMDNotFoundError);
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
		print_error(message.c_str(), device_identifier, method_id, result);
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
	PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(AFCConnectionOpen(houseFd, 0, &afc_conn_p), "Could not open afc connection", device_identifier, method_id, NULL);
	devices[device_identifier].services[kAppleFileConnection] = afc_conn_p;
	return afc_conn_p;
}

void uninstall_application(std::string application_identifier, const char* device_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", device_identifier, method_id, kAMDNotFoundError);
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

	if (result)
	{
		print_error("Could not uninstall application", device_identifier, method_id, result);
	}
	else
	{
		print(json({{ "response", "Successfully uninstalled application" }, { kId, method_id }, { kDeviceId, device_identifier }}));
		// AppleFileConnection and HouseArrest deal with the files on an application so they have to be removed when uninstalling the application
		devices[device_identifier].services.erase(kAppleFileConnection);
		devices[device_identifier].services.erase(kHouseArrest);
	}
}

void install_application(std::string install_path, const char* device_identifier, std::string method_id)
{
	if (!devices.count(device_identifier))
	{
		print_error("Device not found", device_identifier, method_id, kAMDNotFoundError);
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
		CFURLCreateWithString(NULL, path, NULL);
#else
		CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, true);
#endif
	CFRelease(path);
	if (!local_app_url) {
		stop_session(devices[device_identifier].device_info);
		print_error("Could not parse application path", device_identifier, method_id, kAMDAPIInternalError);
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
		print_error("Could not transfer application", device_identifier, method_id, transfer_result);
		return;
	}

	unsigned install_result = AMDeviceSecureInstallApplication(0, devices[device_identifier].device_info, local_app_url, options, NULL, 0);
	CFRelease(cf_package_type);
	CFRelease(cf_developer);
	stop_session(devices[device_identifier].device_info);

	if (install_result)
	{
		print_error("Could not install application", device_identifier, method_id, install_result);
		return;
	}

	print(json({ { "response", "Successfully installed application" }, { kId, method_id }, {kDeviceId, device_identifier}}));
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

void read_dir(HANDLE afcFd, afc_connection* afc_conn_p, const char* dir, json &files, std::string method_id, const char* device_identifier)
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
		print_error(message.c_str(), device_identifier, method_id, afc_file_info_open_result);
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
		read_dir(afcFd, afc_conn_p, dir_joined, files, method_id, device_identifier);
		free(dir_joined);
	}

	unsigned afc_directory_close_result = AFCDirectoryClose(afc_conn_p, afc_dir_p);
	if (afc_directory_close_result)
	{
		print_error("Could not close directory", device_identifier, method_id, afc_directory_close_result);
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
		print_error("Could not establish AFC Connection", device_identifier, method_id, afc_connection_open_result);
		return;
	}
	json files;
	
	std::string device_path_str = windows_path_to_unix(device_path);
	read_dir(houseFd, afc_conn_p, device_path_str.c_str(), files, method_id, device_identifier);
	print(json({{ "response", files }, { kId, method_id }, { kDeviceId, device_identifier }}));
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

bool ensure_device_path_exists(std::string &device_path, afc_connection *connection, std::string method_id, const char * device_identifier)
{
	std::vector<std::string> directories = split(device_path, kUnixPathSeparator);
	std::string curent_device_path("");
	for (std::string directory_path : directories)
	{
		if (!directory_path.empty())
		{
			curent_device_path += kUnixPathSeparator;
			curent_device_path += directory_path;
			PRINT_ERROR_AND_RETURN_VALUE_IF_FAILED_RESULT(AFCDirectoryCreate(connection, curent_device_path.c_str()), "Unable to make directory", device_identifier, method_id, false)
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
		if (ensure_device_path_exists(dir_name, afc_conn_p, method_id, device_identifier))
		{
			std::stringstream error_message;
			AFCRemovePath(afc_conn_p, destination);
			error_message << "Could not open file " << destination << " for writing";
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefOpen(afc_conn_p, destination, kAFCFileModeWrite, &file_ref), error_message.str().c_str(), device_identifier, method_id);
			error_message.str(std::string());
			error_message << "Could not write to file: " << destination;
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefWrite(afc_conn_p, file_ref, &file_content[0], file_content.size()), error_message.str().c_str(), device_identifier, method_id);
			error_message.str(std::string());
			error_message << "Could not close file reference: " << destination;
			PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefClose(afc_conn_p, file_ref), error_message.str().c_str(), device_identifier, method_id);
			error_message.str(std::string());
			print(json({{ "response", "Successfully uploaded file" },{ kId, method_id },{ kDeviceId, device_identifier }}));
		}
	}
	else 
	{
		std::string message("Could not open file: ");
		message += source;
		print_error(message.c_str(), device_identifier, method_id, kAMDAPIInternalError);
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
	PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCRemovePath(afc_conn_p, destination), error_message.str().c_str(), device_identifier, method_id);
	print(json({{ "response", "Successfully removed file" },{ kId, method_id },{ kDeviceId, device_identifier } }));
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
	PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(AFCFileRefOpen(afc_conn_p, destination, kAFCFileModeRead, &file_ref), error_message.str().c_str(), device_identifier, method_id);
	unsigned size_to_read = 1 << 3;
	void *buf = malloc(sizeof(size_t) * size_to_read);
	AFCFileRefRead(afc_conn_p, &file_ref, buf, size_to_read);

	print(json({{ "response", "Successfully uploaded file" },{ kId, method_id },{ kDeviceId, device_identifier } }));
}

void get_application_infos(const char *device_identifier, std::string method_id)
{
	HANDLE socket = start_service(device_identifier, kInstallationProxy, method_id);
	if (!socket)
	{
		return;
	}

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

	std::vector<json> livesync_app_infos;
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
		PRINT_ERROR_AND_RETURN_IF_FAILED_RESULT(dict.count("Error"), boost::any_cast<std::string>(dict["Error"]).c_str(), device_identifier, method_id);
		if (dict.count("Status") && boost::any_cast<std::string>(dict["Status"]) == "Complete")
		{
			break;
		}
		std::vector<boost::any> current_list = boost::any_cast<std::vector<boost::any>>(dict["CurrentList"]);
		for (boost::any list : current_list)
		{
			std::map<std::string, boost::any> app_info = boost::any_cast<std::map<std::string, boost::any>>(list);
			json current_info = {
				{ "IceniumLiveSyncEnabled", app_info.count("IceniumLiveSyncEnabled") && boost::any_cast<bool>(app_info["IceniumLiveSyncEnabled"]) },
				{ "CFBundleIdentifier", app_info.count("CFBundleIdentifier") ? boost::any_cast<std::string>(app_info["CFBundleIdentifier"]) : "" },
				{ "configuration", app_info.count("configuration") ? boost::any_cast<std::string>(app_info["configuration"]) : ""},
			};
			livesync_app_infos.push_back(current_info);
		}
	}

	print(json({ { "apps", livesync_app_infos }, { kId, method_id },{ kDeviceId, device_identifier } }));
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
		message[kDeviceId] = device_identifier;
		message[kMessage] = std::string(buffer, bytes_read);
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
		print_error("Device not found", device_identifier, method_id, kAMDNotFoundError);
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
	print(json({ { "response", "Successfully sent notification" },{ kId, method_id },{ kDeviceId, device_identifier } }));
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
					std::string device_identifier = arg.value(kDeviceId, "");
					std::string path = arg.value("path", "");
					list_files(device_identifier.c_str(), application_identifier.c_str(), path.c_str(), method_id);
				}
			}
			if (method_name == "upload")
			{
				for (json arg : method_args)
				{
					std::string application_identifier = arg.value("appId", "");
					std::string device_identifier = arg.value(kDeviceId, "");
					std::string source = arg.value("source", "");
					std::string destination = arg.value("destination", "");
					upload_file(device_identifier.c_str(), application_identifier.c_str(), source.c_str(), destination.c_str(), method_id);
				}
			}
			if (method_name == "delete")
			{
				for (json arg : method_args)
				{
					std::string application_identifier = arg.value("appId", "");
					std::string device_identifier = arg.value(kDeviceId, "");
					std::string destination = arg.value("destination", "");
					delete_file(device_identifier.c_str(), application_identifier.c_str(), destination.c_str(), method_id);
				}
			}
			///*if (method_name == "read")
			//{
			//	for (json arg : method_args)
			//	{
			//		std::string application_identifier = arg.value("appId", "");
			//		std::string device_identifier = arg.value(kDeviceId, "");
			//		std::string destination = arg.value("destination", "");
			//		read_file(device_identifier.c_str(), application_identifier.c_str(), destination.c_str());
			//	}
			//}*/
			if (method_name == "apps")
			{
				for (json device_identifier_json : method_args) {
					std::string device_identifier = device_identifier_json.get<std::string>();
					perform_detached_operation(get_application_infos, device_identifier, method_id);
				}
			}
			if (method_name == "log")
			{
				for (json device_identifier_json : method_args) {
					std::string device_identifier = device_identifier_json.get<std::string>();
					perform_detached_operation(device_log, device_identifier, method_id);
				}
			}
			if (method_name == "notify")
			{
				for (json arg : method_args)
				{
					std::string device_identifier = arg.value(kDeviceId, "");
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
