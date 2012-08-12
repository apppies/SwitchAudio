// EndPointController.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include "windows.h"
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"
using namespace std;

HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{	
	IPolicyConfigVista *pPolicyConfig;
	ERole reserved = eConsole;

    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
		pPolicyConfig->Release();
	}
	return hr;
}

// EndPointController.exe [NewDefaultDeviceID]
int _tmain(int argc, _TCHAR* argv[])
{
	bool setup = false;
	vector<wstring> devicesToToggle;
	LPWSTR wstrDefaultID = NULL;
	// read the command line option, -1 indicates list devices.
	// setup: create file with device ids to toggle between
	if (argc == 2)
	{
		wcout << argv[1];
		if (wcscmp(argv[1], L"-setup") == 0)
		{
			setup = true;
			wcout << L"Setup..\n";
		}
	}

	//Load devices to toggle from config file
	if (setup == false)
	{
		wifstream fin("config.txt");
		if (fin.fail())
		{
			wcout << L"No or unreadable config file, running setup" << endl;
			setup = true;
		}
		else
		{
			while (fin.good())
			{
				WCHAR line[256];
				fin.getline(line, 256);
				if (wcslen(line) != 0)
					devicesToToggle.push_back(line);
			}
			fin.close();

			if (devicesToToggle.size() == 0)
			{
				wcout << L"Empty or unreadable config file, running setup";
				devicesToToggle.clear();
				setup = true;
			}
		}
	}

	int option = -1;
	if (argc == 2) option = atoi((char*)argv[1]);

	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IMMDeviceEnumerator *pEnum = NULL;
		// Create a multimedia device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
		if (SUCCEEDED(hr))
		{

			// Get the default output device
			IMMDevice *pDeviceOut;
			
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDeviceOut);
			if (SUCCEEDED(hr))
			{				
				hr = pDeviceOut->GetId(&wstrDefaultID);
				if (SUCCEEDED(hr))
				{

				}
				pDeviceOut->Release();
			}
			//wcout << L"Current default device: " << wstrDefaultID << endl;

			// Get the next wanted audio device
			wstring wantedDevice;
			if (!setup)
			{
				wantedDevice = *devicesToToggle.begin();
				for (vector<wstring>::iterator n = devicesToToggle.begin(); n != devicesToToggle.end(); ++n)
				{
					if (n->compare(wstrDefaultID) == 0)
					{
						//default id found, get next 
						n++;
						if (n == devicesToToggle.end()) n = devicesToToggle.begin();
						wantedDevice = *n;
						break;
					}
				}					
				//wcout << L"Switching to " << wantedDevice << endl;
			}

			IMMDeviceCollection *pDevices;
			// Enumerate the output devices.
			hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
			if (SUCCEEDED(hr))
			{
				UINT count;
				pDevices->GetCount(&count);
				if (SUCCEEDED(hr))
				{
					bool deviceFound = false;

					for (int i = 0; i < count; i++)
					{
						IMMDevice *pDevice;
						hr = pDevices->Item(i, &pDevice);
						if (SUCCEEDED(hr))
						{
							LPWSTR wstrID = NULL;
							hr = pDevice->GetId(&wstrID);
							if (SUCCEEDED(hr))
							{
								IPropertyStore *pStore;
								hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
								if (SUCCEEDED(hr))
								{
									PROPVARIANT friendlyName;
									PropVariantInit(&friendlyName);
									hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
									/*PROPVARIANT deviceDesc;									
									PropVariantInit(&deviceDesc);
									hr = pStore->GetValue(PKEY_Device_DeviceDesc, &deviceDesc);*/
									if (SUCCEEDED(hr))
									{
										// if setup, ask to include in file
										if (setup)
										{
											wcout << L"Audio device: " << friendlyName.pwszVal << endl;
											//wcout << L"Description: " << deviceDesc.pwszVal << endl;
											wcout << L"Include this device? [y/n]";
											char type;
											cin >> type;
											if (!std::cin.fail() && type == 'y')
											{
												wcout << L"Testing device.."  << endl;
												hr = SetDefaultAudioPlaybackDevice(wstrID);
												if (SUCCEEDED(hr))
												{
													//switch back to the default device
													Sleep(500); // without delay the default playback device does not get back up
													SetDefaultAudioPlaybackDevice(wstrDefaultID);

													wcout << L"Succesfully tested " << friendlyName.pwszVal  << endl << endl;
													devicesToToggle.push_back(wstrID);
													
												}
												else
													wcout << L"Can not switch to " << friendlyName.pwszVal << L"\nThis device will not be added to the configuration" << endl << endl;
											}
										}

										// not setup switch to the requested device
										else
										{
											if (wantedDevice.compare(wstrID) == 0)
											{
												deviceFound = true;
												hr = SetDefaultAudioPlaybackDevice(wstrID);
												if (SUCCEEDED(hr))
													wcout << L"Succesfully switched to " << friendlyName.pwszVal;
												else
													wcout << L"Failed to switch to " << friendlyName.pwszVal;
											}
										}

										PropVariantClear(&friendlyName);
									}
									pStore->Release();
								}
							}
							pDevice->Release();
						}
					} //end for

					//check if device was found
					if (!deviceFound && !setup)
						wcout << L"The requested device was not found, you might want to rerun with the 'setup' argument";

				}
				pDevices->Release();
			}

			pEnum->Release();
		}

	}

	//Write config file
	if (setup)
	{
		wofstream fout("config.txt");
		for (vector<wstring>::iterator n = devicesToToggle.begin(); n != devicesToToggle.end(); ++n)
		{
			fout << *n << endl;		
		}
		
	}

	return hr;
}

