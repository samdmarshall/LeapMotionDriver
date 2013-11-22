#ifndef LeapMotionDriver_LeapMotion_c
#define LeapMotionDriver_LeapMotion_c

#include "LeapMotion.h"

static UInt32 messageCount = 0x0;

OSDefineMetaClassAndStructors(LeapMotionController, IOService)

bool LeapMotionController::init(OSDictionary *dict) {
    bool result = IOService::init(dict);
    LM_IOLog("LeapMotionDriver - Initializing\n");
	for (UInt32 i = 0x0; i < LeapMotionConnections; i++) {
        connection[i].controller = NULL;
        connection[i].inController = NULL;
        connection[i].outController = NULL;
        connection[i].service = NULL;
        connection[i].isActive = false;
    }
    return result;
}

bool LeapMotionController::didTerminate (IOService *provider, IOOptionBits options, bool *defer) {
	LM_IOLog("LeapMotionDriver - didTerminate()\n");
	ReleaseAll();
	return IOService::didTerminate(provider, options, defer);
}

void LeapMotionController::ReleaseAll(void) {
	for (UInt32 i = 0x0; i < LeapMotionConnections; i++) {
		
        if (connection[i].service != NULL) {
            connection[i].service->terminate(kIOServiceRequired);
            connection[i].service->detachAll(gIOServicePlane);
            connection[i].service->release();
            connection[i].service = NULL;
        }
		
        if (connection[i].inController != NULL) {
            connection[i].inController->Abort();
            connection[i].inController->release();
            connection[i].inController = NULL;
        }
		
        if (connection[i].outController != NULL) {
            connection[i].outController->Abort();
            connection[i].outController->release();
            connection[i].outController = NULL;
        }
		
        if (connection[i].controller != NULL) {
            connection[i].controller->close(this);
            connection[i].controller = NULL;
			connectionCount--;
        }
		
        connection[i].isActive = false;
    }
	LM_IOLog("LeapMotionDriver - Remaining connection count: %d\n",connectionCount);
	
	if (device != NULL) {
		device->close(this);
		device = NULL;
	}
}

void LeapMotionController::free(void) {
    LM_IOLog("LeapMotionDriver - Freeing\n");
    IOService::free();
}

IOService* LeapMotionController::probe(IOService *provider, SInt32 *score) {
    IOService *result = IOService::probe(provider, score);
    LM_IOLog("LeapMotionDriver - Probing\n");
	device = OSDynamicCast(IOUSBDevice, provider);
	if (device != NULL) {
		UInt8 configurationCount = device->GetNumConfigurations();
		if (configurationCount < 0x1) {
			LM_IOLog("LeapMotionDriver - Cannot find configurations\n");
		} else {
			LM_IOLog("LeapMotionDriver - Found %d configurations!\n",configurationCount);
			device->retain();
			if (device->open(this)) {
				LM_IOLog("LeapMotionDriver - Open device\n");
			}
		}
	}
    return result;
}

bool LeapMotionController::start(IOService *provider) {
	messageCount = 0x0;
    bool result = IOService::start(provider);
    LM_IOLog("LeapMotionDriver - Starting\n");
	if (device != NULL) {
		LM_IOLog("LeapMotionDriver - Device - Active\n");
		const IOUSBConfigurationDescriptor *configDescription = device->GetFullConfigurationDescriptor(0x0);
		if (!configDescription) {
			result = false;
		} else {
			IOReturn setConfigResult = device->SetConfiguration(this, configDescription->bConfigurationValue, true);
			if (setConfigResult != kIOReturnSuccess) {
				result = false;
			} else {
				LM_IOLog("LeapMotionDriver - Device - Successfully set configuration\n");
				IOUSBFindInterfaceRequest interfaceRequest;
				interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
				interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
				interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
				interfaceRequest.bAlternateSetting = 0x0;
				
				IOUSBFindEndpointRequest pipeRequest;
				pipeRequest.interval = 0x0;
				pipeRequest.maxPacketSize = 0x0;
				pipeRequest.type = kUSBInterrupt;
				
				UInt32 connections = 0x0 + connectionCount;
				UInt32 interfaceCount = 0x0;
				IOUSBInterface *interface = NULL;
				while ((interface = device->FindNextInterface(interface, &interfaceRequest)) != NULL) {
					LM_IOLog("LeapMotionDriver - Device - Interface %d\n",interfaceCount);
					
					LM_IOLog("LeapMotionDriver - Interface %d - Protocol %d\n",interfaceCount,interface->GetInterfaceProtocol());
					if (!interface->open(this)) {
						result = false;
						break;
					}
					UInt8 subclass = interface->GetInterfaceSubClass();
					LM_IOLog("LeapMotionDriver - Interface %d - Subclass %d\n",interfaceCount,subclass);
					connection[connections].controller = interface;
					if (subclass == 0x1) {
						pipeRequest.direction = kUSBIn;
						connection[connections].inController = interface->FindNextPipe(NULL, &pipeRequest);
						if (connection[connections].inController) {
							LM_IOLog("LeapMotionDriver - Interface %d - Found USBIn\n",interfaceCount);
							connection[connections].inController->retain();
						} else {
							LM_IOLog("LeapMotionDriver - Interface %d - Cannot find pipe!\n",interfaceCount);
							result = false;
						}
						connections++;
					}
					if (subclass == 0x2) {
						pipeRequest.direction = kUSBAnyDirn;
						connection[connections].outController = interface->FindNextPipe(NULL, &pipeRequest);
						if (connection[connections].outController) {
							LM_IOLog("LeapMotionDriver - Interface %d - Found USBAny\n",interfaceCount);
							connection[connections].outController->retain();
						} else {
							LM_IOLog("LeapMotionDriver - Interface %d - Cannot find pipe!\n",interfaceCount);
							result = false;
						}
						connections++;
					}
					interfaceCount++;
					if (!result) {
						break;
					}
				}
				connectionCount = connections;
			}
		}
	} else {
		result = false;
	}
	
	if (!result) {
		ReleaseAll();
	}
    return result;
}

IOReturn LeapMotionController::message(UInt32 type,IOService *provider,void *argument) {
	LM_IOLog("LeapMotionDriver - Message #%03d - %08x - ",messageCount,type);
	switch (type) {
		case kIOMessageServiceIsTerminated: {
			LM_IOLog("kIOMessageServiceIsTerminated\n");
			break;
		};
		case kIOMessageServiceIsSuspended: {
			LM_IOLog("kIOMessageServiceIsSuspended\n");
			break;
		};
		case kIOMessageServiceIsResumed: {
			LM_IOLog("kIOMessageServiceIsResumed\n");
			break;
		};
		case kIOMessageServiceIsRequestingClose: {
			LM_IOLog("kIOMessageServiceIsRequestingClose\n");
			break;
		};
		case kIOMessageServiceIsAttemptingOpen: {
			LM_IOLog("kIOMessageServiceIsAttemptingOpen\n");
			break;
		};
		case kIOMessageServiceWasClosed: {
			LM_IOLog("kIOMessageServiceWasClosed\n");
			break;
		};
		case kIOMessageServiceBusyStateChange: {
			LM_IOLog("kIOMessageServiceBusyStateChange\n");
			break;
		};
		case kIOMessageConsoleSecurityChange: {
			LM_IOLog("kIOMessageConsoleSecurityChange\n");
			break;
		};
		case kIOMessageServicePropertyChange: {
			LM_IOLog("kIOMessageServicePropertyChange\n");
			break;
		};
		case kIOMessageCopyClientID: {
			LM_IOLog("kIOMessageCopyClientID\n");
			break;
		};
		case kIOMessageSystemCapabilityChange: {
			LM_IOLog("kIOMessageSystemCapabilityChange\n");
			break;
		};
		case kIOMessageDeviceSignaledWakeup: {
			LM_IOLog("kIOMessageDeviceSignaledWakeup\n");
			break;
		};
		case kIOMessageDeviceWillPowerOff: {
			LM_IOLog("kIOMessageDeviceWillPowerOff\n");
			break;
		};
		case kIOMessageDeviceHasPoweredOn: {
			LM_IOLog("kIOMessageDeviceHasPoweredOn\n");
			break;
		};
		case kIOMessageSystemWillPowerOff: {
			LM_IOLog("kIOMessageSystemWillPowerOff\n");
			break;
		};
		case kIOMessageSystemWillRestart: {
			LM_IOLog("kIOMessageSystemWillRestart\n");
			break;
		};
		case kIOMessageSystemPagingOff: {
			LM_IOLog("kIOMessageSystemPagingOff\n");
			break;
		};
		case kIOMessageCanSystemSleep: {
			LM_IOLog("kIOMessageCanSystemSleep\n");
			break;
		};
		case kIOMessageSystemWillNotSleep: {
			LM_IOLog("kIOMessageSystemWillNotSleep\n");
			break;
		};
		case kIOMessageSystemWillSleep: {
			LM_IOLog("kIOMessageSystemWillSleep\n");
			break;
		};
		case kIOMessageSystemWillPowerOn: {
			LM_IOLog("kIOMessageSystemWillPowerOn\n");
			break;
		};
		case kIOMessageSystemHasPoweredOn: {
			LM_IOLog("kIOMessageSystemHasPoweredOn\n");
			break;
		};
		case kIOMessageCanDevicePowerOff: {
			LM_IOLog("kIOMessageCanDevicePowerOff\n");
			break;
		};
		case kIOMessageDeviceWillNotPowerOff: {
			LM_IOLog("kIOMessageDeviceWillNotPowerOff\n");
			break;
		};
		case kIOMessageSystemWillNotPowerOff: {
			LM_IOLog("kIOMessageSystemWillNotPowerOff\n");
			break;
		};
		case kIOMessageCanSystemPowerOff: {
			LM_IOLog("kIOMessageCanSystemPowerOff\n");
			break;
		};
		case kIOMessageDeviceWillPowerOn: {
			LM_IOLog("kIOMessageDeviceWillPowerOn\n");
			break;
		};
		case kIOMessageDeviceHasPoweredOff: {
			LM_IOLog("kIOMessageDeviceHasPoweredOff\n");
			break;
		};
		default: {
			LM_IOLog("kIOMessageUnknown\n");
			break;
		};
	}
	messageCount++;
	return IOService::message(type,provider,argument);
}

void LeapMotionController::stop(IOService *provider) {
    LM_IOLog("LeapMotionDriver - Stopping\n");
    IOService::stop(provider);
}

#endif