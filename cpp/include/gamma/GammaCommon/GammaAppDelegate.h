//
//  GammaAppDelegate.h
//  GammaCommon
//
//  Created by daphnis on 15/9/30.
//  Copyright © 2015年 daphnis. All rights reserved.
//


#import <Availability.h>

#ifndef __IPHONE_5_0
#warning "This project uses features only available in iOS SDK 5.0 and later."
#endif

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>
#endif

#ifndef GammaAppDelegate_h
#define GammaAppDelegate_h

//==============================================================
// GammaAppDelegate
//==============================================================
@interface GammaAppDelegate : UIResponder
<UIApplicationDelegate,CLLocationManagerDelegate>
@property (strong, nonatomic) UIWindow *window;
+ (void)setGammaAppDelegateClass:(Class)delegateClass;
- (void)setRotateMask:(UIInterfaceOrientationMask)mask;
- (long)prepareGammaThreadStack;
- (NSThread*)gammaThread;
@end

#endif /* GammaAppDelegate_h */
