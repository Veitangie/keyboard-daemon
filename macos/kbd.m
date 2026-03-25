// Keyboard Daemon - A cross-platform layout synchronization tool
// Copyright (C) 2026 Veitangie
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#import <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDManager.h>

NSMutableSet *connectedDevices;
NSArray<NSString *> *layoutMap;

void performHandshake(IOHIDDeviceRef device) {
  uint8_t payload[32] = {0};

  payload[0] = 0x00;
  IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, payload,
                       sizeof(payload));
  [NSThread sleepForTimeInterval:0.05];

  payload[0] = 0x01;
  IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, payload,
                       sizeof(payload));
  [NSThread sleepForTimeInterval:0.05];

  payload[0] = 0xFE;
  IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, payload,
                       sizeof(payload));

  NSLog(@"[INFO] Handshake fired for device.");
}

void sendLayoutCommand(uint8_t layer) {
  if (connectedDevices.count == 0)
    return;

  uint8_t payload[32] = {0};
  payload[0] = 0x04;
  payload[1] = 0x01;
  payload[2] = layer;

  for (NSValue *deviceVal in connectedDevices) {
    IOHIDDeviceRef device = (IOHIDDeviceRef)[deviceVal pointerValue];
    IOReturn res = IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0,
                                        payload, sizeof(payload));

    if (res != kIOReturnSuccess) {
      NSLog(@"[WARN] Failed to send payload to a device. Code: %x", res);
    }
  }
  NSLog(@"[INFO] Synced Layer %d -> ON across %lu device(s)", layer,
        (unsigned long)connectedDevices.count);
}

void layoutChangedCallback(CFNotificationCenterRef center, void *observer,
                           CFStringRef name, const void *object,
                           CFDictionaryRef userInfo) {
  TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
  NSString *langID = (__bridge NSString *)TISGetInputSourceProperty(
      source, kTISPropertyInputSourceID);

  for (NSUInteger i = 0; i < layoutMap.count; i++) {
    if ([langID localizedCaseInsensitiveContainsString:layoutMap[i]]) {
      NSLog(@"[DEBUG] Matched OS langID '%@' to argument '%@' (Layer %lu)",
            langID, layoutMap[i], (unsigned long)i);
      sendLayoutCommand((uint8_t)i);
      break;
    }
  }

  CFRelease(source);
}

void deviceConnected(void *context, IOReturn result, void *sender,
                     IOHIDDeviceRef device) {
  NSValue *deviceVal = [NSValue valueWithPointer:device];

  if ([connectedDevices containsObject:deviceVal])
    return;

  NSLog(@"[INFO] Target keyboard Raw HID endpoint connected.");
  [connectedDevices addObject:deviceVal];

  performHandshake(device);

  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)),
      dispatch_get_main_queue(), ^{
        if ([connectedDevices containsObject:deviceVal]) {
          NSLog(@"[INFO] Firing 1.5s delayed handshake...");
          performHandshake(device);
          layoutChangedCallback(NULL, NULL, NULL, NULL, NULL);
        }
      });
}

void deviceDisconnected(void *context, IOReturn result, void *sender,
                        IOHIDDeviceRef device) {
  NSValue *deviceVal = [NSValue valueWithPointer:device];
  if ([connectedDevices containsObject:deviceVal]) {
    NSLog(@"[INFO] Target keyboard disconnected.");
    [connectedDevices removeObject:deviceVal];
  }
}

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    NSLog(@"[INFO] Starting macOS Layout Sync Daemon...");

    if (argc > 1) {
      NSMutableArray *args = [NSMutableArray arrayWithCapacity:argc - 1];
      for (int i = 1; i < argc; i++) {
        [args addObject:[NSString stringWithUTF8String:argv[i]]];
      }
      layoutMap = [args copy];
    } else {
      layoutMap = @[ @"ABC", @"Russian" ];
    }

    NSLog(@"[INFO] Layout mapping initialized: %@", layoutMap);

    connectedDevices = [[NSMutableSet alloc] init];

    IOHIDManagerRef hidManager =
        IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    NSDictionary *matchDict =
        @{@kIOHIDVendorIDKey : @0x3297,
          @kIOHIDPrimaryUsagePageKey : @0xFF60};

    IOHIDManagerSetDeviceMatching(hidManager,
                                  (__bridge CFDictionaryRef)matchDict);
    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, deviceConnected,
                                               NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, deviceDisconnected,
                                              NULL);

    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetMain(),
                                    kCFRunLoopDefaultMode);
    IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);

    CFNotificationCenterAddObserver(
        CFNotificationCenterGetDistributedCenter(), NULL, layoutChangedCallback,
        kTISNotifySelectedKeyboardInputSourceChanged, NULL,
        CFNotificationSuspensionBehaviorDeliverImmediately);

    NSLog(@"[INFO] Daemon running. Listening for layout changes...");
    CFRunLoopRun();
  }
  return 0;
}
