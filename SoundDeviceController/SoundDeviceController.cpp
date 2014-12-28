#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <atlstr.h>
#include <devicetopology.h>
#include "PolicyConfig.h"
#include <functiondiscoverykeys.h>

#define THROW_ON_ERROR(hres)  if (FAILED(hres)) { throw hres;  }
#define SAFE_RELEASE(obj)  if (obj != NULL) { obj->Release(); obj = NULL; }

class SoundDeviceController {
	
	IMMDeviceEnumerator *deviceEnumeration = NULL;
	IMMDeviceCollection *deviceCollection = NULL;
	UINT devicesCount = 0;
	HRESULT hr = NULL;

private:

	void SoundDeviceController::init() {
		hr = CoInitialize(NULL);
		THROW_ON_ERROR(hr);

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**) &deviceEnumeration);
		THROW_ON_ERROR(hr);

		hr = deviceEnumeration->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &deviceCollection);
		THROW_ON_ERROR(hr)

		hr = deviceCollection->GetCount(&devicesCount);
		THROW_ON_ERROR(hr)
	}

	PROPVARIANT SoundDeviceController::getName(IPropertyStore* propertyStore) {
		PROPVARIANT varName;
		PropVariantInit(&varName);
		hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &varName);
		THROW_ON_ERROR(hr);
		return varName;
	}

public:

	SoundDeviceController::SoundDeviceController() {
		init();
	}

	SoundDeviceController::~SoundDeviceController() {
		CoUninitialize();
		SAFE_RELEASE(deviceCollection);
		SAFE_RELEASE(deviceEnumeration);
	}

	bool setDefaultAudioPlaybackDevice(LPCWSTR devId) {
		IPolicyConfig *pPolicyConfig;
		ERole reserved = eConsole;

		HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfig), (LPVOID *)&pPolicyConfig);
		if (SUCCEEDED(hr)) {
			hr = pPolicyConfig->SetDefaultEndpoint(devId, reserved);
			pPolicyConfig->Release();
			return true;
		}
		else {
			return false;
		}
	}

	void SoundDeviceController::listDevices() {
		for (ULONG i = 0; i < devicesCount; i++) {
			IMMDevice *device = NULL;
			hr = deviceCollection->Item(i, &device);
			THROW_ON_ERROR(hr)

			IPropertyStore* propertyStore = NULL;
			hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
			THROW_ON_ERROR(hr);

			PROPVARIANT varName = getName(propertyStore);

			LPWSTR devId;
			device->GetId(&devId);

			printf("Endpoint ID: %S Name: %S\n", devId, varName.pwszVal);
		}
	}

	LPWSTR SoundDeviceController::getDeviceIdByName(LPWSTR name) {
		for (ULONG i = 0; i < devicesCount; i++) {
			IMMDevice *device = NULL;
			hr = deviceCollection->Item(i, &device);
			THROW_ON_ERROR(hr)

				IPropertyStore* propertyStore = NULL;
			hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
			THROW_ON_ERROR(hr);

			PROPVARIANT varName = getName(propertyStore);

			if (wcscmp(name, varName.pwszVal) == 0) {
				LPWSTR devId;
				device->GetId(&devId);
				return devId;
			}
		}
		return NULL;
	}

};

int main(int argc, char* argv[]) {
	SoundDeviceController soundDeviceController;

	if (argc > 1) {
		char* command = argv[1];
		char* parameter = argv[2];


		if (command == NULL) {
			command = "";
		}
		if (parameter == NULL) {
			parameter = "";
		}

		if (strcmp(command, "list") == 0) {
			soundDeviceController.listDevices();
		}
		else if (strcmp(command, "setDefault") == 0) {
			int paramLength = strlen(parameter);
			size_t convertedChars = 0;

			wchar_t* wtext = NULL;
			mbstowcs_s(&convertedChars, wtext, paramLength, parameter, _TRUNCATE);

			LPWSTR devId = soundDeviceController.getDeviceIdByName(wtext);
			soundDeviceController.setDefaultAudioPlaybackDevice(devId);
		}
		else {
			printf("%s", "Usage:\n'SoundDeviceController.exe list' to list all devices\n'SoundDeviceController.exe setDefault \"deviceName\"' to set default device");
		}
	} else {
		printf("%s", "Usage:\n'SoundDeviceController.exe list' to list all devices\n'SoundDeviceController.exe setDefault \"deviceName\"' to set default device");
	}
	exit(0);
}
