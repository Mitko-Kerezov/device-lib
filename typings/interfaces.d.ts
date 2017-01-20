declare module IOSDeviceLib {
	interface IDeviceActionInfo {
		deviceId: string;
		event: string;
		deviceColor?: string;
		deviceName?: string;
		productType?: string;
		productVersion?: string;
		status?: string;
	}

	interface IDeviceId {
		deviceId: string;
	}

	interface IAppId {
		appId: string;
	}

	interface IAppDevice extends IDeviceId, IAppId {
	}

	interface IDestination {
		destination: string;
	}

	interface ISource {
		source: string;
	}

	interface IReadOperationData extends IAppDevice {
		path: string;
	}

	interface IFileOperationData extends IAppDevice, IDestination, ISource {
	}

	interface IDeleteFileData extends IAppDevice, IDestination {
	}

	interface INotifyData extends IDeviceId {
		notificationName: string;
	}

	interface IErrorDTO {
		code: number;
		message: string;
	}

	interface IDeviceError extends IDeviceId {
		error: IErrorDTO;
	}

	interface IDeviceResponse extends IDeviceId {
		response: string;
	}

	interface IDeviceMultipleResponse extends IDeviceId {
		response: string[];
	}

	interface IApplicationInfo {
		CFBundleIdentifier: string;
		IceniumLiveSyncEnabled: boolean;
		configuration: string;
	}

	interface IDeviceAppInfo extends IDeviceId {
		response: IApplicationInfo[];
	}

	interface IDeviceLogData extends IDeviceId {
		message: string;
	}

	interface IOSDeviceLib extends NodeJS.EventEmitter {
		new (onDeviceFound: (found: IDeviceActionInfo) => void, onDeviceLost: (found: IDeviceActionInfo) => void): IOSDeviceLib;
		install(ipaPath: string, deviceIdentifiers: string[]): Promise<IDeviceResponse | IErrorDTO>[];
		uninstall(ipaPath: string, deviceIdentifiers: string[]): Promise<IDeviceResponse | IErrorDTO>[];
		list(listArray: IReadOperationData[]): Promise<IDeviceMultipleResponse | IErrorDTO>[];
		upload(uploadArray: IFileOperationData[]): Promise<IDeviceResponse | IErrorDTO>[];
		download(downloadArray: IFileOperationData[]): Promise<IDeviceResponse | IErrorDTO>[];
		read(readArray: IReadOperationData[]): Promise<IDeviceResponse | IErrorDTO>[];
		delete(deleteArray: IDeleteFileData[]): Promise<IDeviceResponse | IErrorDTO>[];
		notify(notifyArray: INotifyData): Promise<IDeviceResponse | IErrorDTO>[];
		apps(deviceIdentifiers: string[]): Promise<IDeviceAppInfo | IErrorDTO>[];
		startDeviceLog(deviceIdentifiers: string[]): void;
		dispose(signal?: string): void;
		on(event: "deviceLogData", listener: (response: IDeviceLogData | IErrorDTO) => void): NodeJS.EventEmitter;
	}
}