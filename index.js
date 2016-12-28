"use strict";

const bufferpack = require("bufferpack");
const uuid = require('node-uuid');
const spawn = require("child_process").spawn;
const path = require("path");
const fs = require("fs");
const os = require("os");
const EventEmitter = require("events");

const DeviceEventEnum = {
	kDeviceFound: "deviceFound",
	kDeviceLost: "deviceLost",
	kDeviceTrusted: "deviceTrusted",
	kDeviceUnknown: "deviceUnknown"
};

const MethodNames = {
	install: "install",
	uninstall: "uninstall",
	list: "list",
	log: "log",
	upload: "upload",
	delete: "delete",
	notify: "notify",
	apps: "apps",
};

const Events = {
	deviceLogData: "deviceLogData"
};

class DeviceLib extends EventEmitter {
	constructor(onDeviceFound, onDeviceLost) {
		super();
		this._chProc = spawn(path.join(__dirname, "bin", os.platform(), os.arch(), "devicelib"));
		this._chProc.stdout.on("data", (data) => {
			const parsedMessage = this._read(data);
			switch (parsedMessage.event) {
				case DeviceEventEnum.kDeviceFound:
					onDeviceFound(parsedMessage);
					break;
				case DeviceEventEnum.kDeviceLost:
					onDeviceLost(parsedMessage);
					break;
				case DeviceEventEnum.kDeviceTrusted:
					onDeviceLost(parsedMessage);
					onDeviceFound(parsedMessage);
					break;
			}
		});
	}

	install(ipaPath, deviceIdentifiers) {
		return this._getPromise(MethodNames.install, [ipaPath, deviceIdentifiers], deviceIdentifiers.length);
	}

	uninstall(appId, deviceIdentifiers) {
		return this._getPromise(MethodNames.uninstall, [appId, deviceIdentifiers], deviceIdentifiers.length);
	}

	list(listArray) {
		return this._getPromise(MethodNames.list, listArray, listArray.length);
	}

	upload(listArray) {
		return this._getPromise(MethodNames.upload, listArray, listArray.length);
	}

	delete(listArray) {
		return this._getPromise(MethodNames.delete, listArray, listArray.length);
	}

	notify(listArray) {
		return this._getPromise(MethodNames.notify, listArray, listArray.length);
	}

	apps(deviceIdentifiers) {
		return this._getPromise(MethodNames.apps, deviceIdentifiers, deviceIdentifiers.length);
	}

	startDeviceLog(deviceIdentifiers) {
		return this._getPromise(MethodNames.log, deviceIdentifiers);
	}

	_getPromise(methodName, args, numberOrMessages) {
		return new Promise((resolve, reject) => {
			let responses = [];
			if (!args || !args.length) {
				return reject(new Error("No arguments provided"));
			}

			const id = uuid.v4();
			const eventHandler = (data) => {
				let response = this._read(data, id);
				if (response) {
					delete response.id;
					if (numberOrMessages) {
						responses.push(response);
						if (responses.length === numberOrMessages) {
							resolve(responses);
							this._chProc.stdout.removeListener("data", eventHandler);
						}
					} else {
						this.emit(Events.deviceLogData, response);
					}
				}
			};

			this._chProc.stdout.on("data", eventHandler);
			this._chProc.stdin.write(this._getMessage(id, methodName, args));
		});
	}

	_getMessage(id, name, args) {
		return JSON.stringify({ methods: [{ id: id, name: name, args: args }] }) + '\n';
	}

	_read(data, id) {
		if (data.length) {
			const messageSizeBuffer = data.slice(0, 4);
			const bufferLength = bufferpack.unpack(">i", messageSizeBuffer)[0];
			const message = data.slice(messageSizeBuffer.length, bufferLength + messageSizeBuffer.length);
			let parsedMessage = {};

			try {
				parsedMessage = JSON.parse(message.toString());
			} catch (ex) {
				console.log("An ERROR OCCURRED");
				// What to do here?
			}

			if (!id || id === parsedMessage.id) {
				return parsedMessage;
			} else {
				return this._read(data.slice(bufferLength + messageSizeBuffer.length), id);
			}
		}
	}
}

module.exports = DeviceLib;
