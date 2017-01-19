declare module IOSDeviceLib {
	interface IDeviceActionInfo {
		deviceId: string;
		deviceColor: string;
		deviceName: string;
		event: string;
		productType: string;
		productVersion: string;
		status: string;
	}

	interface IOSDeviceLib {
		install(ipaPath: string, deviceIdentifiers: string[]): Promise<any[]>;
		startLookingForDevices(deviceFoundCallback: (deviceInfo: IDeviceActionInfo) => void, deviceLostCallback: (deviceInfo: IDeviceActionInfo) => void): Promise<void>;
		dispose(signal?: string): void;
	}
}