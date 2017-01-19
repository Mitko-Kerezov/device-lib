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

	interface IOSDeviceLib {
		install(ipaPath: string, deviceIdentifiers: string[]): Promise<any[]>;
		uninstall(ipaPath: string, deviceIdentifiers: string[]): Promise<any[]>;
		list(listArray: IReadOperationData[]): Promise<any[]>;
		upload(uploadArray: IFileOperationData[]): Promise<any[]>;
		download(downloadArray: IFileOperationData[]): Promise<any[]>;
		read(readArray: IReadOperationData[]): Promise<any[]>;
		delete(deleteArray: IDeleteFileData[]): Promise<any[]>;
		notify(notifyArray: INotifyData): Promise<any[]>;
		apps(deviceIdentifiers: string[]): Promise<any[]>;
		startDeviceLog(deviceIdentifiers: string[]): void;
		dispose(signal?: string): void;
	}
}