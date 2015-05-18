/*
 * Copyright © 1998-2012 Apple Inc.  All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#import <Foundation/Foundation.h>
#import "BusProberSharedFunctions.h"
#import "BusProbeDevice.h"

#import "DecodeDeviceDescriptor.h"
#import "DecodeConfigurationDescriptor.h"
#import "DecodeInterfaceDescriptor.h"
#import "DecodeEndpointDescriptor.h"
#import "DecodeHIDDescriptor.h"
#import "DecodeHubDescriptor.h"
#import "DecodeDeviceQualifierDescriptor.h"
#import "DecodeAudioInterfaceDescriptor.h"
#import "DecodeVideoInterfaceDescriptor.h"
#import "DecodeCommClassDescriptor.h"
#import "DecodeBOSDescriptor.h"

#define HID_DESCRIPTOR                  0x21
#define DFU_FUNCTIONAL_DESCRIPTOR       0x21
#define CCID_DESCRIPTOR					0x21

enum ClassSpecific {
    CS_INTERFACE		= 0x24,
    CS_ENDPOINT			= 0x25
};

@interface DescriptorDecoder : NSObject {

}

+(void)decodeBytes:(Byte *)p forDevice:(BusProbeDevice *)thisDevice deviceInterface:(IOUSBDeviceRef)deviceIntf userInfo:(void *)userInfo isOtherSpeedDesc:(BOOL)isOtherSpeedDesc  isinCurrentConfig:(Boolean)inCurrentConfig;
+(void)dumpRawDescriptor:(Byte *)p forDevice:(BusProbeDevice *)thisDevice atDepth:(int)depth;
+(void)dumpRawConfigDescriptor:(IOUSBConfigurationDescriptor*)cfg forDevice:(BusProbeDevice *)thisDevice atDepth:(int)depth;
+(void)dump:(int)n byte:(Byte *)p forDevice:(BusProbeDevice *)thisDevice atDepth:(int)depth;
+(void)dumpRawBOSDescriptor:(IOUSBBOSDescriptor*)bos forDevice:(BusProbeDevice *)thisDevice atDepth:(int)depth;
@end
