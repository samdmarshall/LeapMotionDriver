#ifndef LeapMotionDriver_LeapMotion_h
#define LeapMotionDriver_LeapMotion_h

#include <IOKit/IOLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOService.h>
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

#ifdef DEBUG
#define LM_IOLog(format,...) IOLog(format,## __VA_ARGS__)
#else
#define LM_IOLog(format,...) 
#endif

#define LeapMotionDeviceCount 5
#define LeapMotionConnections 2*LeapMotionDeviceCount

#define LeapMotionController com_samdmarshall_driver_LeapMotion

class LeapMotionController;

struct LMDConnection {
	IOUSBInterface *controller;
	IOUSBPipe *inController, *outController;
	bool isActive;
	LeapMotionController *service;
};

class LeapMotionController : public IOService
{
	OSDeclareDefaultStructors(LeapMotionController)
public:
    virtual bool init(OSDictionary *dictionary = 0);
    virtual void free(void);
    virtual IOService *probe(IOService *provider, SInt32 *score);
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
	IOReturn message(UInt32 type,IOService *provider,void *argument);
	bool didTerminate (IOService *provider, IOOptionBits options, bool *defer);
private:
	IOUSBDevice *device;
	
	struct LMDConnection connection[LeapMotionConnections];
	UInt32 connectionCount;
	
	void ReleaseAll(void);
};

#endif