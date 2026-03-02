//
//  IOS.mm
//  TestOpenGL
//
//  Created by daphnis on 15-1-8.
//  Copyright (c) 2015年 daphnis. All rights reserved.
//

#import <Availability.h>

#ifndef __IPHONE_5_0
#warning "This project uses features only available in iOS SDK 5.0 and later."
#endif

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <Photos/Photos.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MPMediaItem.h>
#import <MediaPlayer/MediaPlayer.h>
#import <Foundation/NSProcessInfo.h>
#import <CoreLocation/CoreLocation.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>
#endif

#import "IOS.h"
#import <UIKit/UIKit.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <sys/resource.h>
#import <sys/utsname.h>

#include "GammaCommon/CThread.h"
#include "GammaCommon/CVersion.h"
#include "GammaCommon/GammaMath.h"
#include "GammaCommon/TGammaList.h"
#include "GammaCommon/GammaTime.h"
#include "GammaCommon/TGammaStrStream.h"
#include "GammaCommon/GammaAppDelegate.h"
#include "CReadFileThread.h"
#include <arpa/inet.h>
#include <sys/sysctl.h>
#include <list>

//==============================================================
// IOSMessage 传递的消息结构定义
//==============================================================
@interface IOSMessage : NSObject
@property uint32 nWindowID;
@property uint32 nInputID;
@property uint32 nType;
@property uint32 nParam1;
@property uint32 nParam2;
@property (strong, nonatomic) id param1;
@property (strong, nonatomic) id param2;
@end

@implementation IOSMessage
@end

//==============================================================
// FileDelegate
//==============================================================
@interface FileDelegete : NSObject
<UIImagePickerControllerDelegate,
UINavigationControllerDelegate,
MPMediaPickerControllerDelegate>
@property int32 nType;
@property void* pContext;
@property Gamma::SystemFileCallback funFileCallback;
@property Gamma::SystemFileListCallback funListCallback;
@property (strong, nonatomic) NSArray<NSString*>* fileList;
-(void)onSelectFinished:(NSArray<NSString*>*)fileList;
@end

//==============================================================
// GammaAppDelegate
//==============================================================
@interface GammaAppDelegate ()
@property (strong, nonatomic) NSThread* m_gammaThread;
@property (strong, nonatomic) CLLocationManager* m_locationMgr;
@property UIInterfaceOrientationMask m_mask;
-(void)sendMessage:(IOSMessage*)msg waitUntilDone:(BOOL)wait;
-(void)onSystemFile:(FileDelegete*)file;
@end

static Class s_gammaAppDelegate = nil;

//==============================================================
// OpenGLView
//==============================================================
@interface OpenGLView : UIView<UITextFieldDelegate>
{
	#define MAX_TOUCH_POINT 10
	
    uint32                  	m_nWindowID;
	std::map<UITouch*,uint32>	m_mapTouchs;
	UITouch*					m_aryTounchs[MAX_TOUCH_POINT];
    UITextField*            	m_pTextField;
	NSString*					m_sPreText;
	bool						m_bFullScreen;
}
@end

@implementation OpenGLView
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

+(float)getScale
{
	return [[UIScreen mainScreen] scale];
}

- (id)initGLView:(uint32)windowID Size:(CGRect)rtSize
{
	GammaLog << "GLViewSize:"
		<< rtSize.size.width << ","
		<< rtSize.size.height << std::endl;
	
	self = [super initWithFrame:rtSize];
	self.multipleTouchEnabled = true;
	
    if( !self )
		return nil;
	
	float scale = [OpenGLView getScale];
	self.contentScaleFactor = scale;
	( (CAEAGLLayer*)self.layer ).contentsScale = scale;
	( (CAEAGLLayer*)self.layer ).opaque = YES;
	
	m_nWindowID = windowID;
	
    rtSize.size.height = rtSize.size.height/2.0;
    m_pTextField = [[UITextField alloc] initWithFrame:rtSize];
    [m_pTextField setBorderStyle:UITextBorderStyleRoundedRect]; //外框类型
	m_pTextField.delegate = (id)self;
	m_pTextField.secureTextEntry = NO; //密码
	m_pTextField.autocorrectionType = UITextAutocorrectionTypeNo;
	m_pTextField.autocapitalizationType = UITextAutocapitalizationTypeNone;
	m_pTextField.returnKeyType = UIReturnKeyDone;
	m_pTextField.clearButtonMode = UITextFieldViewModeWhileEditing; //编辑时会出现个修改
	
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(deviceOrientationDidChange:)
		name:UIDeviceOrientationDidChangeNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(keyboardWillShow:)
		name:UIKeyboardWillShowNotification object:nil];
	[[NSNotificationCenter defaultCenter]addObserver:self
		selector:@selector(textFiledEditChanged:)
		name:UITextFieldTextDidChangeNotification object:m_pTextField];
    return self;
}

-(void)enableInput:(IOSMessage*)msg
{
    if( msg.nType == 0 )
    {
        [m_pTextField resignFirstResponder];
        [m_pTextField removeFromSuperview];
        return;
    }
	
	m_bFullScreen = msg.nParam1 != 0 ? true : false;
	m_pTextField.placeholder = msg.param2;
	[m_pTextField setText:msg.param2];
	
	uint32 nLen = msg.param2 ? (uint32)[(NSString*)msg.param2 length] : 0;
	UITextPosition* beginning = m_pTextField.beginningOfDocument;
	UITextPosition* startPosition = [m_pTextField positionFromPosition:beginning offset:nLen];
	UITextPosition* endPosition = [m_pTextField positionFromPosition:beginning offset:nLen];
	UITextRange* selectionRange = [m_pTextField textRangeFromPosition:startPosition toPosition:endPosition];
	[m_pTextField setSelectedTextRange:selectionRange];
	
	CGRect rtSize = [self bounds];
	rtSize.origin.y = m_bFullScreen ? 0 : 100000;
	rtSize.size.height = rtSize.size.height/2.0;
	m_pTextField.frame = rtSize;
	m_sPreText = @"";
	
	[self addSubview:m_pTextField];
    [m_pTextField becomeFirstResponder];
    [msg.param2 release];
    [msg release];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	NSString* str = [m_pTextField.text copy];
    [m_pTextField resignFirstResponder];
    [m_pTextField removeFromSuperview];
	
	if( m_bFullScreen )
	{
		UIApplication* app = [UIApplication sharedApplication];
		GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
    
		IOSMessage* msg = [[IOSMessage alloc]init];
		msg.nWindowID = m_nWindowID;
		msg.nInputID = 0;
		msg.nType = WM_CHAR;
		msg.nParam1 = 1;
		msg.nParam2 = 0;
		msg.param1 = self;
		msg.param2 = str;
		[pAppDelegate sendMessage:msg waitUntilDone:NO];
	}
	
	return YES;
}

-(void)textFiledEditChanged:(NSNotification *)obj
{
	UITextField* textField = (UITextField *)obj.object;
    NSString* str = [textField.text copy];
	if( !m_bFullScreen && m_pTextField == textField && [m_pTextField subviews] )
	{
        UIApplication* app = [UIApplication sharedApplication];
        GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
        
        IOSMessage* msg = [[IOSMessage alloc]init];
        msg.nWindowID = m_nWindowID;
        msg.nInputID = 0;
        msg.nType = WM_CHAR;
        msg.nParam1 = 1;
        msg.nParam2 = 0;
        msg.param1 = self;
        msg.param2 = str;
        [pAppDelegate sendMessage:msg waitUntilDone:NO];
	}
}

-(uint32)addTouchID:(UITouch*)touch
{
	std::map<UITouch*,uint32>::iterator it = m_mapTouchs.find( touch );
	if( it != m_mapTouchs.end() )
		return it->second;
	
    for( uint32 i = 0; i < MAX_TOUCH_POINT; i++ )
    {
        if( m_aryTounchs[i] )
            continue;
        m_mapTouchs[touch] = i;
		m_aryTounchs[i] = touch;
        return i;
    }
	
	return INVALID_32BITID;
}

-(uint32)delTouchID:(UITouch*)touch
{
	std::map<UITouch*,uint32>::iterator it = m_mapTouchs.find( touch );
	if( it == m_mapTouchs.end() )
		return INVALID_32BITID;
	uint32 nID = it->second;
	m_aryTounchs[nID] = NULL;
	m_mapTouchs.erase( it );
	return nID;
}

-(void)postTouchEvent:(NSSet *)touches withMsgID:(uint32)msgID withParam:(uint32)param
{
    UIApplication* app = [UIApplication sharedApplication];
    GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
    
    for( UITouch *touch in touches )
	{
		uint32 nID;
		if( msgID == WM_LBUTTONUP )
			nID = [self delTouchID:touch];
		else
			nID = [self addTouchID:touch];
		
        if( nID == INVALID_32BITID )
            continue;
		
		float scale = [OpenGLView getScale];
        CGPoint curPos = [touch locationInView:self];
		uint32 x = (uint32)( curPos.x*scale );
		uint32 y = (uint32)( curPos.y*scale );
		
        IOSMessage* msg = [[IOSMessage alloc]init];
        msg.nWindowID = m_nWindowID;
        msg.nInputID = nID;
        msg.nType = msgID;
        msg.nParam1 = param;
        msg.nParam2 = MAKE_UINT32( (int16)x, (int16)y );
        msg.param1 = self;
        [pAppDelegate sendMessage:msg waitUntilDone:NO];
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [touches count] == 0 )
        return;
	[self postTouchEvent:touches withMsgID:WM_LBUTTONDOWN withParam:MK_LBUTTON];}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [touches count] == 0 )
        return;
	[self postTouchEvent:touches withMsgID:WM_MOUSEMOVE withParam:MK_LBUTTON];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [touches count] == 0 )
        return;
    [self postTouchEvent:touches withMsgID:WM_LBUTTONUP withParam:0];
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if( [touches count] == 0 )
        return;
    [self postTouchEvent:touches withMsgID:WM_LBUTTONUP withParam:0];
}

-(void)deviceOrientationDidChange:(NSObject*)sender
{
	bool bLandscape;
	UIApplication* app = [UIApplication sharedApplication];
	GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
	UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
	
	GammaLog << "pAppDelegate.m_mask:" << (uint32)pAppDelegate.m_mask << std::endl;
	GammaLog << "orientation:" << (uint32)( 1 << orientation ) << std::endl;
	
	if( ( pAppDelegate.m_mask&( 1 << orientation ) ) == 0 )
	{
		GammaLog << "===NOT MATCH prevalue===" << std::endl;
		if( pAppDelegate.m_mask&UIInterfaceOrientationMaskLandscapeLeft ||
		   pAppDelegate.m_mask&UIInterfaceOrientationMaskLandscapeRight )
			bLandscape = true;
		else if( pAppDelegate.m_mask&UIInterfaceOrientationMaskPortrait ||
			pAppDelegate.m_mask&UIInterfaceOrientationMaskAllButUpsideDown )
			bLandscape = false;
		else
			return;
	}
	else
	{
		GammaLog << "===MATCH prevalue===" << std::endl;
		if( orientation == UIInterfaceOrientationLandscapeLeft ||
		   orientation == UIInterfaceOrientationLandscapeRight )
			bLandscape = true;
		else if( orientation == UIInterfaceOrientationPortrait ||
			orientation == UIInterfaceOrientationPortraitUpsideDown )
			bLandscape = false;
		else
			return;
	}
	
	GammaLog << "curOrientation:" << ( bLandscape ? "Landscape" : "Portrait" ) << std::endl;
	
	CGRect rtBounds = [[UIScreen mainScreen]bounds];
	CGPoint posCenter = { rtBounds.size.width*0.5f, rtBounds.size.height*0.5f };
	
	if( Gamma::GammaAbs( posCenter.x ) > Gamma::GammaAbs( posCenter.y ) != bLandscape )
	{
		std::swap( posCenter.x, posCenter.y );
		std::swap( rtBounds.origin.x, rtBounds.origin.y );
		std::swap( rtBounds.size.width, rtBounds.size.height );
	}
	
	[self setBounds:rtBounds];
	[self setCenter:posCenter];
	
	float scale = [OpenGLView getScale];
	uint32 w = (uint32)( rtBounds.size.width*scale );
	uint32 h = (uint32)( rtBounds.size.height*scale );

	IOSMessage* msg = [[IOSMessage alloc]init];
	msg.nWindowID = m_nWindowID;
	msg.nInputID = 0;
	msg.nType = WM_SIZE;
	msg.nParam1 = 0;
	msg.nParam2 = MAKE_UINT32( (int16)w, (int16)h );
	msg.param1 = self;
	[pAppDelegate sendMessage:msg waitUntilDone:NO];
}

- (void)keyboardWillShow:(NSNotification *)aNotification
{
	//获取键盘的高度
	NSDictionary *userInfo = [aNotification userInfo];
	NSValue *aValue = [userInfo objectForKey:UIKeyboardFrameEndUserInfoKey];
	CGRect keyboardRect = [aValue CGRectValue];
	int height = keyboardRect.size.height;
	
	CGRect rtSize = [self bounds];
	rtSize.origin.y = m_bFullScreen ? 0 : rtSize.size.height - height - 40;
	rtSize.size.height = m_bFullScreen ? rtSize.size.height - height : 40;
	m_pTextField.frame = rtSize;
}
@end


//==============================================================
// RootController
//==============================================================
@interface RootController : UIViewController
@end

@implementation RootController
-(BOOL)shouldAutorotate
{
	return YES;
}

- (NSUInteger)supportedInterfaceOrientations
{
	UIApplication* app = [UIApplication sharedApplication];
	GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
	return pAppDelegate.m_mask;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
	UIInterfaceOrientation orientation = [super preferredInterfaceOrientationForPresentation];
	UIApplication* app = [UIApplication sharedApplication];
	GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
	if( pAppDelegate.m_mask&( 1 << orientation ) )
		return orientation;
	if( pAppDelegate.m_mask&UIInterfaceOrientationMaskPortrait )
		return UIInterfaceOrientationPortrait;
	if( pAppDelegate.m_mask&UIInterfaceOrientationMaskLandscapeLeft )
		return UIInterfaceOrientationLandscapeLeft;
	if( pAppDelegate.m_mask&UIInterfaceOrientationMaskLandscapeRight )
		return UIInterfaceOrientationLandscapeRight;
	if( pAppDelegate.m_mask&UIInterfaceOrientationMaskPortraitUpsideDown )
		return UIInterfaceOrientationPortraitUpsideDown;
	return orientation;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
	UIApplication* app = [UIApplication sharedApplication];
	GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
	return pAppDelegate.m_mask&( 1 << toInterfaceOrientation );
}

@end

//==============================================================
// FileDelegete
//==============================================================
@implementation FileDelegete
-(void)onSelectFinished:(NSArray<NSString*>*)fileList
{
	self.fileList = fileList;
	UIApplication* app = [UIApplication sharedApplication];
	GammaAppDelegate* pAppDelegate = (GammaAppDelegate *)app.delegate;
	RootController* viewControl = (RootController*)pAppDelegate.window.rootViewController;
	[viewControl dismissViewControllerAnimated:YES completion:nil];
	[pAppDelegate performSelector:@selector(onSystemFile:)
		onThread:[pAppDelegate gammaThread] withObject:self waitUntilDone:NO];
}

-(void)imagePickerController:(UIImagePickerController*)picker
didFinishPickingMediaWithInfo:(NSDictionary*)info
{
	//通过key值获取到图片
	NSString* url = [info[UIImagePickerControllerReferenceURL] absoluteString];
	[self onSelectFinished:[NSArray arrayWithObject:url]];
	[picker release];
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	[self onSelectFinished:nil];
	[picker release];
}

- (void)mediaPicker:(MPMediaPickerController *)picker
  didPickMediaItems:(MPMediaItemCollection *)mediaItemCollection
{
	NSArray* items = [mediaItemCollection items];
	if( items == nil || [items objectAtIndex:0] == nil )
		return [self onSelectFinished:nil];
	MPMediaItem* item  = [items objectAtIndex:0];
	NSURL* assetURL = [item valueForProperty:MPMediaItemPropertyAssetURL];
	NSString* fileName = assetURL ? [assetURL absoluteString] : nil;
	[self onSelectFinished:[NSArray arrayWithObject:fileName]];
	[picker release];
}

-(void)mediaPickerDidCancel:(MPMediaPickerController *)picker
{
	[self onSelectFinished:nil];
	[picker release];
}
@end

//==============================================================
// GammaAppDelegate
//==============================================================
@implementation GammaAppDelegate

+ (void)setGammaAppDelegateClass:(Class)delegateClass
{
	s_gammaAppDelegate = delegateClass;
}

- (void)setRotateMask:(UIInterfaceOrientationMask)mask
{
	self.m_mask = mask;
}

- (long)prepareGammaThreadStack
{
	// 创建游戏主线程
#if defined _DEBUG || defined DEBUG
	return sizeof(uintptr_t)*1024*1024;
#else
	return sizeof(uintptr_t)*1024*1024/2;
#endif
}

- (NSThread*)gammaThread
{
	return self.m_gammaThread;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.
	self.m_mask = UIInterfaceOrientationMaskLandscape;
	
	CGRect rt = [[UIScreen mainScreen]bounds];
    self.window = [[UIWindow alloc] initWithFrame:rt];
    self.window.backgroundColor = [UIColor blackColor];
    [self.window setRootViewController:[[RootController alloc] init]];
    [self.window makeKeyAndVisible];
	
    // 创建游戏主线程
	long nStackSize = [self prepareGammaThreadStack];
	self.m_gammaThread = [NSThread alloc];
    [self.m_gammaThread initWithTarget:self selector:@selector(GammaMainThread:) object:nil];
	[self.m_gammaThread setStackSize:nStackSize];
    [self.m_gammaThread start];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
	GammaLog << "=========applicationWillResignActive begin=========" << std::endl;
	
	IOSMessage* msg1 = [[IOSMessage alloc]init];
	msg1.nType = WM_KILLFOCUS;
	[self sendMessage:msg1 waitUntilDone:YES];
	
	GammaLog << "=========applicationWillResignActive end=========" << std::endl;
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
	GammaLog << "=========applicationDidEnterBackground begin=========" << std::endl;
	
	IOSMessage* msg2 = [[IOSMessage alloc]init];
	msg2.nType = WM_SHOWWINDOW;
	msg2.nParam1 = 0;
	[self sendMessage:msg2 waitUntilDone:YES];
	
	GammaLog << "=========applicationDidEnterBackground end=========" << std::endl;
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
	GammaLog << "=========applicationWillEnterForeground begin=========" << std::endl;
	
	IOSMessage* msg1 = [[IOSMessage alloc]init];
	msg1.nType = WM_SHOWWINDOW;
	msg1.nParam1 = 1;
	[self sendMessage:msg1 waitUntilDone:YES];
	
	GammaLog << "=========applicationWillEnterForeground end=========" << std::endl;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	GammaLog << "=========applicationDidBecomeActive begin=========" << std::endl;
	
	IOSMessage* msg2 = [[IOSMessage alloc]init];
	msg2.nType = WM_SETFOCUS;
	[self sendMessage:msg2 waitUntilDone:YES];
	
	GammaLog << "=========applicationDidBecomeActive end=========" << std::endl;
}

-(void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

-(void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	IOSMessage* msg2 = [[IOSMessage alloc]init];
	msg2.nType = WM_LOW_MEMORY;
	[self sendMessage:msg2 waitUntilDone:YES];
}
	
// 退出应用程序
-(void)exitApplication:(NSNumber*)nExitCode
{
    //[todo:需要加入动画过程，暂时不会，以上注释掉的代码（网上找的）好像是的，但编译有问题]
    exit( nExitCode.intValue );
}

-(void)sendMessage:(IOSMessage*)msg waitUntilDone:(BOOL)wait
{
    [self performSelector:@selector(postMessage:)
        onThread:self.m_gammaThread withObject:msg waitUntilDone:wait];
}

// 创建IOS窗口
-(void)createView:(NSNumber*)windowID
{
    CGRect screenBounds = [self.window.rootViewController.view bounds];
    uint32 nWindowID = (uint32)[windowID unsignedIntegerValue];
	OpenGLView* pGLView = [[OpenGLView alloc] initGLView:nWindowID Size:screenBounds];
	[self.window.rootViewController.view addSubview:pGLView];
	
	float scale = [OpenGLView getScale];
    IOSMessage* msg = [[IOSMessage alloc]init];
    msg.nWindowID = nWindowID;
    msg.nInputID = 0;
    msg.nType = WM_CREATE;
    msg.nParam1 = (uint32)( screenBounds.size.width*scale );
    msg.nParam2 = (uint32)( screenBounds.size.height*scale );
    msg.param1 = pGLView;
	[self sendMessage:msg waitUntilDone:NO];
	
	GammaLog << "createView:" << screenBounds.size.width << "," << screenBounds.size.height << std::endl;
}

// 打开输入法
-(void)enableInput:(IOSMessage*)msg
{
    [msg.param1 enableInput:msg];
}

// 销毁IOS窗口
-(void)destroyView:(OpenGLView*)view
{
    //[todo]
}

-(void)getSystemFile:(FileDelegete*)delegate
{
	if( delegate.nType == CONTENT_TYPE_IMAGE || delegate.nType == CONTENT_TYPE_VIDEO )
	{
		UIImagePickerController * picker = [[UIImagePickerController alloc]init];
		if( ![UIImagePickerController isSourceTypeAvailable:
			 UIImagePickerControllerSourceTypePhotoLibrary] )
			return [delegate onSelectFinished:nil];
		//图片选择是相册（图片来源自相册）
		picker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
		bool bIsMovie = delegate.nType == CONTENT_TYPE_VIDEO;
		CFStringRef type = bIsMovie ? kUTTypeMovie : kUTTypeImage;
		picker.mediaTypes = @[(__bridge NSString *)type];
		RootController* viewControl = (RootController*)self.window.rootViewController;
		picker.delegate = delegate;
		[viewControl presentViewController:picker animated:YES completion:nil];
	}
	else if( delegate.nType == CONTENT_TYPE_MUSIC )
	{
		MPMediaPickerController* mpc = [MPMediaPickerController alloc];
		mpc = [mpc initWithMediaTypes:MPMediaTypeAnyAudio];
		mpc.delegate = delegate;//委托
		mpc.allowsPickingMultipleItems = NO;//是否允许一次选择多个
		RootController* viewControl = (RootController*)self.window.rootViewController;
		[viewControl presentViewController:mpc animated:YES completion:nil];
	}
}

-(void)startLocation
{
	// 初始化并开始更新
	if( self.m_locationMgr )
		return;
	self.m_locationMgr = [[CLLocationManager alloc] init];
	self.m_locationMgr.delegate = self;
	self.m_locationMgr.desiredAccuracy = kCLLocationAccuracyBest;
	self.m_locationMgr.distanceFilter = 5.0;
	[self.m_locationMgr requestAlwaysAuthorization];
	[self.m_locationMgr startUpdatingLocation];
}

- (void)locationManager:(CLLocationManager *)manager
	didUpdateToLocation:(CLLocation *)newLocation
		   fromLocation:(CLLocation *)oldLocation
{
	[self performSelector:@selector(onLocationFinished:)
		onThread:self.m_gammaThread withObject:newLocation waitUntilDone:NO];
	[manager stopUpdatingLocation];
	[self.m_locationMgr release];
	self.m_locationMgr = nil;
}

- (void)locationManager:(CLLocationManager *)manager
	   didFailWithError:(NSError *)error
{
	[manager stopUpdatingLocation];
	[self.m_locationMgr release];
	self.m_locationMgr = nil;
}

//==================================================================
// 游戏线程
//==================================================================
-(void)GammaMainThread:(id) param
{
	NSNumber* nRet = [NSNumber numberWithUnsignedInt:Gamma::CIOSApp::GetInstance().MainThread()];
    [self performSelectorOnMainThread:@selector(exitApplication:) withObject:nRet waitUntilDone:NO];
}
     
-(void)postMessage:(IOSMessage*)msg
{
	Gamma::CIOSApp::GetInstance().OnIOSMessage( (__bridge void*)msg );
}

-(void)onSystemFile:(FileDelegete*)file
{
	if( file.funFileCallback )
	{
		const char* szFileName = NULL;
		if( file.fileList )
			szFileName = [file.fileList objectAtIndex:0].UTF8String;
		file.funFileCallback( file.pContext, szFileName );
	}
	else if( file.funListCallback )
	{
		std::vector<const char*> vecPath;
		uint32 nCount = file.fileList ? file.fileList.count : 0;
		for( uint32 i = 0; i < nCount; i++ )
			vecPath.push_back( [file.fileList objectAtIndex:i].UTF8String );
		const char** aryPath = nCount ? &vecPath[0] : NULL;
		( file.funListCallback )( file.pContext, aryPath, nCount );
	}
}

-(void)onLocationFinished:(CLLocation *)location
{
	Gamma::CIOSApp::GetInstance().SetLocation( location.coordinate.longitude,
		location.coordinate.latitude, 54.0/*这是广州的海拔，纪念代码产生的地方*/ );
}
@end

//==================================================================
// C++
//==================================================================
namespace Gamma
{
    //==================================================
    // IOS 窗口类，用于连接GammaWindow和ios窗口的中间层
    //==================================================
    struct SIOSWnd
    {
        uint32              m_nWindowID;
        OpenGLView*         m_pIOSView;
        InputHandler        m_funHandler;
        void*               m_pContext;
    };
    
    //==============================================================
    // CIOSApp
    //==============================================================
    CIOSApp::CIOSApp()
        : m_funEntry( NULL )
        , m_nArg( 0 )
        , m_szArg( NULL )
        , m_nWindowID( 1 )
		, m_nVersion(0)
		, m_nPreLocationTime( 0 )
		, m_nLocationInterval( 0 )
    {
		memset( &m_HardwareDesc, 0, sizeof(m_HardwareDesc) );
    }
    
    CIOSApp::~CIOSApp()
    {
        
    }

    CIOSApp& CIOSApp::GetInstance()
    {
        static CIOSApp _instance;
        return _instance;
    }
    
    //==============================================================
    // CIOSApp 方法
	//==============================================================
	uint64 CIOSApp::GetVersion()
	{
		return m_nVersion;
	}
	
	void* CIOSApp::GetApplicationHandle()
	{
		return (GammaAppDelegate*)[UIApplication sharedApplication].delegate;
	}
	
	void CIOSApp::GetHardwareDesc( SHardwareDesc& HardwareDesc )
	{
		if( !m_HardwareDesc.m_szOSDesc[0] )
			FetchHardwareInfo();
		HardwareDesc = m_HardwareDesc;
	}
	
	void CIOSApp::OpenURL( const char* szUrl )
	{
		NSString* strUrl = [NSString stringWithCString:szUrl encoding:NSUTF8StringEncoding];
		NSURL* url = [NSURL URLWithString:strUrl];
		[[UIApplication sharedApplication]openURL:url];
	}
	
	bool CIOSApp::IsWifiConnect()
	{
		//创建零地址，0.0.0.0的地址表示查询本机的网络连接状态
		struct sockaddr_in zeroAddress;
		bzero(&zeroAddress, sizeof(zeroAddress));
		zeroAddress.sin_len = sizeof(zeroAddress);
		zeroAddress.sin_family = AF_INET;
		
		// Recover reachability flags
		SCNetworkReachabilityRef defaultRouteReachability =
			SCNetworkReachabilityCreateWithAddress(NULL, (struct sockaddr *)&zeroAddress);
		SCNetworkReachabilityFlags flags;
		//获得连接的标志
		BOOL didRetrieveFlags = SCNetworkReachabilityGetFlags(defaultRouteReachability, &flags);
		CFRelease(defaultRouteReachability);
		//如果不能获取连接标志，则不能连接网络，直接返回
		if( !didRetrieveFlags )
			return false;
		
		//根据获得的连接标志进行判断
		if( ( flags & kSCNetworkReachabilityFlagsReachable ) == 0 )
			return false;
		if( ( flags & kSCNetworkReachabilityFlagsIsWWAN ) )
			return false;
		if( ( flags & kSCNetworkReachabilityFlagsConnectionRequired ) == 0 )
			return true;
		if( ( ( flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) ||
		    ( flags & kSCNetworkReachabilityFlagsConnectionOnTraffic ) ) &&
			( ( flags & kSCNetworkReachabilityFlagsInterventionRequired ) == 0 ) )
			return true;
		return false;
	}
	
	void CIOSApp::SetClipboardContent( int32 nType, const void* pContent, uint32 nSize )
	{
		if( nType == CONTENT_TYPE_TEXT )
		{
			std::string strCpp( (const char*)pContent, nSize );
			NSString* str = [NSString stringWithUTF8String:strCpp.c_str()];
			[[UIPasteboard generalPasteboard] setString:str];
		}
		else if( nType == CONTENT_TYPE_IMAGE )
		{
			NSData* data = [NSData dataWithBytes:pContent length:nSize];
			UIImage *image = [UIImage imageWithData:data];
			[[UIPasteboard generalPasteboard] setImage:image];
		}
	}
	
	void CIOSApp::GetClipboardContent( int32 nType, const void*& pContent, uint32& nSize )
	{
		if( nType == CONTENT_TYPE_TEXT )
		{
			pContent = [[UIPasteboard generalPasteboard].string UTF8String];
			nSize = (uint32)( strlen( (const char*)pContent ) );
		}
		else if( nType == CONTENT_TYPE_IMAGE )
		{
			UIImage* image = [UIPasteboard generalPasteboard].image;
			NSData* imageData = UIImageJPEGRepresentation( image, 1 );
			pContent = imageData.bytes;
			nSize = (uint32)imageData.length;
		}
	}
	
	bool CIOSApp::GetSystemFile( int32 nType, void* pContext, SystemFileCallback funCallback )
	{
		FileDelegete* delegate = [[FileDelegete alloc]init];
		delegate.nType = nType;
		delegate.pContext = pContext;
		delegate.funFileCallback = funCallback;
		
		GammaAppDelegate* pAppDelegate = (GammaAppDelegate*)GetApplicationHandle();
		[pAppDelegate performSelectorOnMainThread:
		 @selector(getSystemFile:) withObject:delegate waitUntilDone:NO];
		return true;
	}
	
	bool CIOSApp::GetSystemFileList( int32 nType, void* pContext, SystemFileListCallback funCallback )
	{
		FileDelegete* delegate = [[FileDelegete alloc]init];
		delegate.nType = nType;
		delegate.pContext = pContext;
		delegate.funListCallback = funCallback;
		
		NSMutableArray<NSString*>* aryFileName = [[NSMutableArray<NSString*> alloc] init];
		if( nType == CONTENT_TYPE_MUSIC )
		{
			MPMediaQuery* allMusic = [[MPMediaQuery alloc] init];
			NSNumber* nMediaType = [NSNumber numberWithInt:MPMediaTypeMusic];
			MPMediaPropertyPredicate* predicate = [MPMediaPropertyPredicate
				predicateWithValue:nMediaType  forProperty:MPMediaItemPropertyMediaType];
			[allMusic addFilterPredicate:predicate];
			NSArray<MPMediaItem*>* aryItems = allMusic.items;
			for( uint32 i = 0; i < aryItems.count; i++ )
			{
				MPMediaItem* item = [aryItems objectAtIndex:i];
				NSURL* assetURL = [item valueForProperty:MPMediaItemPropertyAssetURL];
				NSString* szFileName = assetURL ? [assetURL absoluteString] : nil;
				[aryFileName addObject:szFileName];
			}
			[delegate onSelectFinished:aryFileName];
			return true;
		}
		else if( nType == CONTENT_TYPE_IMAGE )
		{
			PHFetchOptions* allOptions = [[PHFetchOptions alloc] init];
			allOptions.sortDescriptors = @[[NSSortDescriptor
				sortDescriptorWithKey:@"creationDate" ascending:YES]];
			PHAssetMediaType eType = PHAssetMediaTypeImage;
			PHFetchResult* fetchResult = [PHAsset fetchAssetsWithMediaType:eType options:allOptions];
			PHImageRequestOptions* options = [[PHImageRequestOptions alloc] init];
			options.version = PHImageRequestOptionsVersionCurrent;
			options.deliveryMode = PHImageRequestOptionsDeliveryModeHighQualityFormat;
			options.synchronous = YES;
			for( uint32 i = 0; i < fetchResult.count; i++ )
			{
				PHAsset* item = [fetchResult objectAtIndex:i];
				[[PHImageManager defaultManager] requestImageDataForAsset:item
					options:options resultHandler:^(NSData *imageData, NSString* dataUTI,
					UIImageOrientation orientation, NSDictionary*info )
				 {
					 NSString* szFileName = @"assets-library://asset/asset.";
					 NSString* strUrl = [info[@"PHImageFileURLKey"] absoluteString];
					 NSRange range = [strUrl rangeOfString:@"." options:NSBackwardsSearch];
					 NSString* strExt = [strUrl substringFromIndex:( range.location + 1 )];
					 NSString* strUUID = [item.localIdentifier substringToIndex:36];
					 szFileName = [szFileName stringByAppendingString:strExt];
					 szFileName = [szFileName stringByAppendingString:@"?id=" ];
					 szFileName = [szFileName stringByAppendingString:strUUID];
					 szFileName = [szFileName stringByAppendingString:@"&ext=" ];
					 szFileName = [szFileName stringByAppendingString:strExt];
					 [aryFileName addObject:szFileName];
				 }];
			}
			[delegate onSelectFinished:aryFileName];
			return true;
		}
		else if( nType == CONTENT_TYPE_VIDEO )
		{
			PHFetchOptions* allOptions = [[PHFetchOptions alloc] init];
			allOptions.sortDescriptors = @[[NSSortDescriptor
				sortDescriptorWithKey:@"creationDate" ascending:YES]];
			PHAssetMediaType eType = PHAssetMediaTypeVideo;
			PHFetchResult* fetchResult = [PHAsset fetchAssetsWithMediaType:eType options:allOptions];
			PHVideoRequestOptions *options = [[PHVideoRequestOptions alloc] init];
			options.version = PHVideoRequestOptionsVersionCurrent;
			options.deliveryMode = PHVideoRequestOptionsDeliveryModeAutomatic;
			for( uint32 i = 0; i < fetchResult.count; i++ )
			{
				PHAsset* item = [fetchResult objectAtIndex:i];
				[[PHImageManager defaultManager] requestAVAssetForVideo:item
					options:options resultHandler:^(AVAsset* _Nullable asset, AVAudioMix *
					_Nullable audioMix, NSDictionary * _Nullable info)
				 {
					 AVURLAsset* urlAsset = (AVURLAsset*)asset;
					 NSString* strUrl = [urlAsset.URL absoluteString];
					 NSString* szFileName = @"assets-library://asset/asset.";
					 NSRange range = [strUrl rangeOfString:@"." options:NSBackwardsSearch];
					 NSString* strExt = [strUrl substringFromIndex:( range.location + 1 )];
					 NSString* strUUID = [item.localIdentifier substringToIndex:36];
					 szFileName = [szFileName stringByAppendingString:strExt];
					 szFileName = [szFileName stringByAppendingString:@"?id=" ];
					 szFileName = [szFileName stringByAppendingString:strUUID];
					 szFileName = [szFileName stringByAppendingString:@"&ext=" ];
					 szFileName = [szFileName stringByAppendingString:strExt];
					 [aryFileName addObject:szFileName];
					 
					 if( aryFileName.count != fetchResult.count )
						 return;
					 [delegate onSelectFinished:aryFileName];
				 }];
			}
			return true;
		}
		
		return false;
	}
	
	bool CIOSApp::ReadSystemFile( CFileReader* pReader, HLOCK hLock, CAsynReadList* listFinished )
	{
		const char* szFileName = pReader->GetFileName().c_str();
		if( !memcmp( szFileName, "assets-library://", 17 ) )
		{
			NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:szFileName]];
			PHFetchResult *fetchResult = [PHAsset fetchAssetsWithALAssetURLs:@[url] options:nil];
			PHAsset *asset = fetchResult.firstObject;
			
			if( asset.mediaType == PHAssetMediaTypeImage )
			{
				PHImageRequestOptions* options = [[PHImageRequestOptions alloc] init];
				options.version = PHImageRequestOptionsVersionCurrent;
				options.deliveryMode = PHImageRequestOptionsDeliveryModeHighQualityFormat;
				options.synchronous = NO;
				[[PHImageManager defaultManager] requestImageDataForAsset:asset
					options:options resultHandler:^(NSData *imageData, NSString* dataUTI,
					UIImageOrientation orientation, NSDictionary*info )
				{
					 CRefStringPtr& Buffer = pReader->GetFileBuffer();
					 const void* szBuffer = [imageData bytes];
					 Buffer->assign( (const char*)szBuffer, imageData.length );
					 GammaLock( hLock );
					 listFinished->PushBack( *pReader );
					 pReader->SetLoadState( imageData.length ? eLoadState_Succeeded : eLoadState_Failed );
					 GammaUnlock( hLock );
				}];
				return true;
			}
			else if( asset.mediaType == PHAssetMediaTypeVideo )
			{
				PHVideoRequestOptions *options = [[PHVideoRequestOptions alloc] init];
				options.version = PHVideoRequestOptionsVersionCurrent;
				options.deliveryMode = PHVideoRequestOptionsDeliveryModeAutomatic;
				[[PHImageManager defaultManager] requestAVAssetForVideo:asset
					options:options resultHandler:^(AVAsset* _Nullable asset, AVAudioMix *
					_Nullable audioMix, NSDictionary * _Nullable info)
				{
					AVURLAsset *urlAsset = (AVURLAsset *)asset;
					NSURL *url = urlAsset.URL;
					NSData *data = [NSData dataWithContentsOfURL:url];
					CRefStringPtr& Buffer = pReader->GetFileBuffer();
					const void* szBuffer = [data bytes];
					Buffer->assign( (const char*)szBuffer, data.length );
					GammaLock( hLock );
					listFinished->PushBack( *pReader );
					pReader->SetLoadState( data.length ? eLoadState_Succeeded : eLoadState_Failed );
					GammaUnlock( hLock );
				}];
				return true;
			}
			return false;
		}
		
		if( !memcmp( szFileName, "ipod-library://", 15 ) )
		{
			NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:szFileName]];
			AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];
			AVAssetExportSession* exporter = [[AVAssetExportSession alloc]
				initWithAsset:asset presetName:AVAssetExportPresetAppleM4A];
			exporter.outputFileType = AVFileTypeAppleM4A;
			NSString* tempPath = [NSTemporaryDirectory() stringByAppendingString:@"aaa.m4a"];
			exporter.outputURL = [NSURL fileURLWithPath:tempPath];
			[exporter exportAsynchronouslyWithCompletionHandler:^
			{
				NSData* data = [NSData dataWithContentsOfFile:tempPath];
				remove( tempPath.UTF8String );
				CRefStringPtr& Buffer = pReader->GetFileBuffer();
				const void* szBuffer = [data bytes];
				Buffer->assign( (const char*)szBuffer, data.length );
				GammaLock( hLock );
				listFinished->PushBack( *pReader );
				pReader->SetLoadState( data.length ? eLoadState_Succeeded : eLoadState_Failed );
				GammaUnlock( hLock );
			}];
			return true;
		}
		
		return false;
	}
	
	void CIOSApp::StartLocation( uint32 nLocationInterval )
	{
		m_nLocationInterval = Gamma::Max<uint32>( nLocationInterval, 1000 );
	}
	
	void CIOSApp::SetLocation( double fLongitude, double fLatitude, double fAltitude )
	{
		m_fLongitude = fLongitude;
		m_fLatitude = fLongitude;
		m_fAltitude = fAltitude;
	}
	
	bool CIOSApp::GetLocation( double& fLongitude, double& fLatitude, double& fAltitude )
	{
		if( m_nLocationInterval == 0 )
			return false;
		fLongitude = m_fLongitude;
		fLatitude = m_fLatitude;
		fAltitude = m_fAltitude;
		return true;
	}
	
	void* CIOSApp::Init( AppEntryFunction funEntry, int nArg, const char* szArg[] )
	{
        NSArray* paths = NSSearchPathForDirectoriesInDomains( NSCachesDirectory, NSUserDomainMask, YES );
        m_strCache = [[paths objectAtIndex:0] UTF8String];
        m_strPackage = [[[NSBundle mainBundle] resourcePath] UTF8String];
        if( *m_strCache.rbegin() != '/' )
            m_strCache.push_back( '/' );
        if( *m_strPackage.rbegin() != '/' )
            m_strPackage.push_back( '/' );
		
		NSDictionary* infos = [[NSBundle mainBundle] infoDictionary];
		NSString* version = [infos objectForKey:@"CFBundleShortVersionString"];
		m_nVersion = CVersion( [version cStringUsingEncoding:NSUTF8StringEncoding] );
		
        m_funEntry = funEntry;
        m_nArg = nArg;
        m_szArg = szArg;
		
		Class targetAppDelegate = [GammaAppDelegate class];
		Class gammaAppDelegate = s_gammaAppDelegate;
		if( gammaAppDelegate == nil )
			gammaAppDelegate = targetAppDelegate;
		Class superClass = gammaAppDelegate;
		while ( superClass != targetAppDelegate && [superClass superclass] )
			superClass = [superClass superclass];
		if( superClass != targetAppDelegate )
			throw "invalid AppDelegate";

		return gammaAppDelegate;
	}
	
    // 主线程入口
    int32 CIOSApp::StartApp( AppEntryFunction funEntry, int nArg, const char* szArg[] )
    {
		NSLog( @"StartApp" );
		
        @autoreleasepool
        {
			NSString* appDelegateClass = NSStringFromClass( (Class)Init( funEntry, nArg, szArg ) );
            return UIApplicationMain( nArg, (char**)szArg, nil, appDelegateClass );
        }
    }
    
    // 游戏线程入口	
    int32 CIOSApp::MainThread()
	{
		return m_funEntry( m_nArg, m_szArg );
	}
	
	const char* GetDeviceDesc()
	{
		struct utsname systemInfo;
		uname( &systemInfo );
		const char* szDevice = systemInfo.machine;
		
		if( !strcmp( szDevice, "iPhone1,1" ) ) return "iPhone 2G (A1203)";
		if( !strcmp( szDevice, "iPhone1,2" ) ) return "iPhone 3G (A1241/A1324)";
		if( !strcmp( szDevice, "iPhone2,1" ) ) return "iPhone 3GS (A1303/A1325)";
		if( !strcmp( szDevice, "iPhone3,1" ) ) return "iPhone 4 (A1332)";
		if( !strcmp( szDevice, "iPhone3,2" ) ) return "iPhone 4 (A1332)";
		if( !strcmp( szDevice, "iPhone3,3" ) ) return "iPhone 4 (A1349)";
		if( !strcmp( szDevice, "iPhone4,1" ) ) return "iPhone 4S (A1387/A1431)";
		if( !strcmp( szDevice, "iPhone5,1" ) ) return "iPhone 5 (A1428)";
		if( !strcmp( szDevice, "iPhone5,2" ) ) return "iPhone 5 (A1429/A1442)";
		if( !strcmp( szDevice, "iPhone5,3" ) ) return "iPhone 5c (A1456/A1532)";
		if( !strcmp( szDevice, "iPhone5,4" ) ) return "iPhone 5c (A1507/A1516/A1526/A1529)";
		if( !strcmp( szDevice, "iPhone6,1" ) ) return "iPhone 5s (A1453/A1533)";
		if( !strcmp( szDevice, "iPhone6,2" ) ) return "iPhone 5s (A1457/A1518/A1528/A1530)";
		if( !strcmp( szDevice, "iPhone7,1" ) ) return "iPhone 6 Plus (A1522/A1524)";
		if( !strcmp( szDevice, "iPhone7,2" ) ) return "iPhone 6 (A1549/A1586)";
		if( !strcmp( szDevice, "iPhone8,1" ) ) return "iPhone 6s";
		if( !strcmp( szDevice, "iPhone8,2" ) ) return "iPhone 6s Plus";
		if( !strcmp( szDevice, "iPhone8,4" ) ) return "iPhone SE";
		if( !strcmp( szDevice, "iPhone9,1" ) ) return "iPhone 7";
		if( !strcmp( szDevice, "iPhone9,2" ) ) return "iPhone 7 Plus";
		
		if( !strcmp( szDevice, "iPod1,1" ) )   return "iPod Touch 1G (A1213)";
		if( !strcmp( szDevice, "iPod2,1" ) )   return "iPod Touch 2G (A1288)";
		if( !strcmp( szDevice, "iPod3,1" ) )   return "iPod Touch 3G (A1318)";
		if( !strcmp( szDevice, "iPod4,1" ) )   return "iPod Touch 4G (A1367)";
		if( !strcmp( szDevice, "iPod5,1" ) )   return "iPod Touch 5G (A1421/A1509)";
		
		if( !strcmp( szDevice, "iPad1,1" ) )   return "iPad 1G (A1219/A1337)";
		
		if( !strcmp( szDevice, "iPad2,1" ) )   return "iPad 2 (A1395)";
		if( !strcmp( szDevice, "iPad2,2" ) )   return "iPad 2 (A1396)";
		if( !strcmp( szDevice, "iPad2,3" ) )   return "iPad 2 (A1397)";
		if( !strcmp( szDevice, "iPad2,4" ) )   return "iPad 2 (A1395+New Chip)";
		if( !strcmp( szDevice, "iPad2,5" ) )   return "iPad Mini 1G (A1432)";
		if( !strcmp( szDevice, "iPad2,6" ) )   return "iPad Mini 1G (A1454)";
		if( !strcmp( szDevice, "iPad2,7" ) )   return "iPad Mini 1G (A1455)";
		
		if( !strcmp( szDevice, "iPad3,1" ) )   return "iPad 3 (A1416)";
		if( !strcmp( szDevice, "iPad3,2" ) )   return "iPad 3 (A1403)";
		if( !strcmp( szDevice, "iPad3,3" ) )   return "iPad 3 (A1430)";
		if( !strcmp( szDevice, "iPad3,4" ) )   return "iPad 4 (A1458)";
		if( !strcmp( szDevice, "iPad3,5" ) )   return "iPad 4 (A1459)";
		if( !strcmp( szDevice, "iPad3,6" ) )   return "iPad 4 (A1460)";
		
		if( !strcmp( szDevice, "iPad4,1" ) )   return "iPad Air (A1474)";
		if( !strcmp( szDevice, "iPad4,2" ) )   return "iPad Air (A1475)";
		if( !strcmp( szDevice, "iPad4,3" ) )   return "iPad Air (A1476)";
		if( !strcmp( szDevice, "iPad4,4" ) )   return "iPad Mini 2G (A1489)";
		if( !strcmp( szDevice, "iPad4,5" ) )   return "iPad Mini 2G (A1490)";
		if( !strcmp( szDevice, "iPad4,6" ) )   return "iPad Mini 2G (A1491)";
		if( !strcmp( szDevice, "iPad4,6" ) )   return "iPad Mini 2G (A1491)";
		if( !strcmp( szDevice, "i386"    ) )   return "iPhone Simulator";
		if( !strcmp( szDevice, "x86_64"  ) )   return "iPhone Simulator";
		return "Unknow Device";
	}
	
	void CIOSApp::FetchHardwareInfo()
	{
		
		/* 收集设备信息
		 char	m_szDeviceDesc[DEVICEDESC_LEN]*;
		 char	m_szCpuType[CPUTYPE_LEN]*;
		 char	m_szOSDesc[OSDESC_LEN]*;
		 char	m_szLanguage[LANGUAGE_LEN]*;
		 uint64	m_nMac*;
		 uint32	m_nCpuFrequery*;
		 uint32	m_nCpuCount*;
		 uint32	m_nMemSize;
		 uint16  m_nScreen_X;
		 uint16	m_nScreen_Y;
		 uint32	m_nVideoMemSize*;
		 */
		
		strcpy2array_safe( m_HardwareDesc.m_szDeviceDesc, GetDeviceDesc() );
		
		size_t nLen[2] = { 4, 4 };
		int aryFlagCpu[2][2] = { { CTL_HW, HW_NCPU }, { CTL_HW, HW_CPU_FREQ } };
		sysctl( aryFlagCpu[0], 2, &m_HardwareDesc.m_nCpuCount, &nLen[0], NULL, 0 );
		sysctl( aryFlagCpu[1], 2, &m_HardwareDesc.m_nCpuFrequery, &nLen[1], NULL, 0 );
		gammasstream( m_HardwareDesc.m_szCpuType ) << "CPU "
			<< m_HardwareDesc.m_nCpuFrequery/1000000000.0f
			<< "GHz " << m_HardwareDesc.m_nCpuCount << "Core";
		
		UIDevice *device = [UIDevice currentDevice];
		gammasstream( m_HardwareDesc.m_szOSDesc ) << "IOS " << [device.systemVersion UTF8String];
		
		NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
		NSString* preferredLang = [[defaults objectForKey:@"AppleLanguages"] objectAtIndex:0];
		strcpy2array_safe( m_HardwareDesc.m_szLanguage, [preferredLang UTF8String] );
		
		NSProcessInfo* processInfo = [NSProcessInfo processInfo];
		m_HardwareDesc.m_nMac = 0x02;
		m_HardwareDesc.m_nMemSize = (uint32)processInfo.physicalMemory;
		
		CGRect rect = [[UIScreen mainScreen] bounds];
		CGFloat scale = [[UIScreen mainScreen] scale];
		m_HardwareDesc.m_nScreen_X = (uint16)( rect.size.width * scale + 0.5f );
		m_HardwareDesc.m_nScreen_Y = (uint16)( rect.size.height * scale + 0.5f );
		
		//获取设备唯一id
		NSString* KEY = @"com.joyegame.engine";
		NSMutableDictionary *keychainQuery =
			[NSMutableDictionary dictionaryWithObjectsAndKeys:
			(__bridge id)kSecClassGenericPassword,(__bridge id)kSecClass,
			KEY, (__bridge id)kSecAttrService,
			KEY, (__bridge id)kSecAttrAccount,
			(__bridge id)kSecAttrAccessibleAfterFirstUnlock,(__bridge id)kSecAttrAccessible,
			nil];
		
		NSString* uuid = nil;
		[keychainQuery setObject:(__bridge id)kCFBooleanTrue forKey:(__bridge id)kSecReturnData];
		[keychainQuery setObject:(__bridge id)kSecMatchLimitOne forKey:(__bridge id)kSecMatchLimit];
		CFDataRef keyData = NULL;
		if( SecItemCopyMatching((__bridge CFDictionaryRef)keychainQuery, (CFTypeRef*)&keyData) == noErr )
		{
			@try{ uuid = [NSKeyedUnarchiver unarchiveObjectWithData:(__bridge NSData *)keyData]; }
			@catch (NSException *e){}
			@finally{}
		}
		
		if(keyData)
			CFRelease(keyData);
		
		if( !uuid || uuid.length == 0 )
		{
			NSMutableDictionary *keychainQuery =
				[NSMutableDictionary dictionaryWithObjectsAndKeys:
				 (__bridge id)kSecClassGenericPassword,(__bridge id)kSecClass,
				 KEY, (__bridge id)kSecAttrService,
				 KEY, (__bridge id)kSecAttrAccount,
				 (__bridge id)kSecAttrAccessibleAfterFirstUnlock,(__bridge id)kSecAttrAccessible,
				 nil];
			
			uuid = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
			//Delete old item before add new item
			SecItemDelete((__bridge CFDictionaryRef)keychainQuery);
			//Add new object to search dictionary(Attention:the data format)
			NSData* data = [NSKeyedArchiver archivedDataWithRootObject:uuid];
			[keychainQuery setObject:data forKey:(__bridge id)kSecValueData];
			//Add item to keychain with the search dictionary
			SecItemAdd((__bridge CFDictionaryRef)keychainQuery, NULL);
		}
		[uuid getCString:m_HardwareDesc.m_szUUID maxLength:UUID_LEN encoding:NSUTF8StringEncoding];
	}
	
    // 创建窗口中间层
    SIOSWnd* CIOSApp::CreateIOSGLView( InputHandler funHandler, void* pContext )
    {
        uint32 nWindowID = m_nWindowID++;
        SIOSWnd& Wnd = m_mapWindows[nWindowID];
        Wnd.m_nWindowID = nWindowID;
        Wnd.m_pIOSView = NULL;
        Wnd.m_funHandler = funHandler;
        Wnd.m_pContext = pContext;
		
		GammaAppDelegate* pAppDelegate = (GammaAppDelegate*)GetApplicationHandle();
		NSNumber* nID = [NSNumber numberWithUnsignedInt:nWindowID];
		[pAppDelegate performSelectorOnMainThread:
		 @selector(createView:) withObject:nID waitUntilDone:NO];
        return &Wnd;
    }

    // 销毁窗口
    void CIOSApp::DestroyIOSGLView( SIOSWnd* pWnd )
    {
		GammaAppDelegate* pAppDelegate = (GammaAppDelegate*)GetApplicationHandle();
		[pAppDelegate performSelectorOnMainThread:
		 @selector(destroyView:) withObject:pWnd->m_pIOSView waitUntilDone:NO];
    }
    
    // 打开关闭输入法
    void CIOSApp::EnableInput( SIOSWnd* pWnd, bool bEnable, bool bFullScreen, const wchar_t* szInitText )
    {
        IOSMessage* msg = [[IOSMessage alloc]init];
        msg.nWindowID = pWnd->m_nWindowID;
        msg.nInputID = 0;
        msg.nParam1 = bFullScreen ? 1 : 0;
        msg.nParam2 = 0;
        msg.nType = bEnable;
        msg.param1 = pWnd->m_pIOSView;
        msg.param2 = nil;
        
        if( bEnable && szInitText )
        {
			std::vector<unichar> vecChar;
            while( *szInitText )
                vecChar.push_back( *szInitText++ );
            msg.param2 = [NSString stringWithCharacters:&vecChar[0] length:vecChar.size() ];
        }
		
		GammaAppDelegate* pAppDelegate = (GammaAppDelegate*)GetApplicationHandle();
		[pAppDelegate performSelectorOnMainThread:
		 @selector(enableInput:) withObject:msg waitUntilDone:NO];
    }
    
    // 获取Native窗口
    void* CIOSApp::GetNativeHandler( SIOSWnd* pWnd )
    {
        return pWnd->m_pIOSView ? (__bridge void*)( pWnd->m_pIOSView ) : NULL;
    }
    
    // 消息泵，调用NSRunLoop完成消息接收
    uint32 CIOSApp::IOSMessagePump()
    {
        uint32 nMsgCount = 0;
        NSRunLoop* runloop = [NSRunLoop currentRunLoop];
        [runloop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
		
		// 更新定位
		if( m_nLocationInterval && m_nLocationInterval != INVALID_32BITID )
		{
			int64 nCurTime = Gamma::GetGammaTime();
			if( nCurTime > m_nPreLocationTime + m_nLocationInterval )
			{
				m_nPreLocationTime = nCurTime;
				GammaAppDelegate* pAppDelegate = (GammaAppDelegate*)GetApplicationHandle();
				[pAppDelegate performSelectorOnMainThread:
				 @selector(startLocation) withObject:nil waitUntilDone:NO];
			}
		}
        return nMsgCount;
    }
    
    // 消息处理
    void CIOSApp::OnIOSMessage( void* msg )
    {
        IOSMessage* msgIOS = (__bridge IOSMessage*)msg;
		if( msgIOS.nWindowID == 0 )
		{
			for( CWindowsMap::iterator it = m_mapWindows.begin();
				it != m_mapWindows.end(); ++it )
			{
				msgIOS.nWindowID = it->first;
				msgIOS.param1 = it->second.m_pIOSView;
				OnIOSMessage( msgIOS );
			}
			return;
		}
		
        CWindowsMap::iterator it = m_mapWindows.find( msgIOS.nWindowID );
        if( it == m_mapWindows.end() )
        {
            [msgIOS release];
            return;
        }
        
        SIOSWnd& Wnd = it->second;
        if( msgIOS.nType == WM_CREATE )
        {
            Wnd.m_pIOSView = msgIOS.param1;
        }
        else if( msgIOS.nType == WM_CHAR )
        {
            NSString* strChar = (NSString*)msgIOS.param2;
			if( msgIOS.nParam1 != 0 )
				Wnd.m_funHandler( Wnd.m_pContext, 0, WM_CHAR, 0, 0 );
			else
			{
				for( uint32 i = 0; i < msgIOS.nParam2; i++ )
				{
					Wnd.m_funHandler( Wnd.m_pContext, 0, WM_KEYDOWN, VK_BACK, 0 );
					Wnd.m_funHandler( Wnd.m_pContext, 0, WM_KEYUP, VK_BACK, 0 );
				}
			}
			
			uint32 length = (uint32)strChar.length;
			for( uint32 i = 0; i < length; i++ )
				Wnd.m_funHandler( Wnd.m_pContext, 0, WM_CHAR, [strChar characterAtIndex:i], 0 );
			
            [strChar release];
            [msgIOS release];
            return;
        }
		
        Wnd.m_funHandler( Wnd.m_pContext, msgIOS.nInputID, msgIOS.nType, msgIOS.nParam1, msgIOS.nParam2 );
        [msgIOS release];
    }

}

