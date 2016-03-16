/*!*****************************************************************************************************************
\file         PVRShell\EntryPoint\UIApplicationMain\AppController.mm
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief        Implementation of the AppController for the UIKit based (iOS) implementation of PVRShell.
********************************************************************************************************************/
#import "AppController.h"

//CONSTANTS:
const int kFPS = 60.0;

// CLASS IMPLEMENTATION
@implementation AppController

- (void) mainLoop
{
	if(stateMachine->executeOnce() != pvr::Result::Success)
	{		
        [mainLoopTimer invalidate];
        mainLoopTimer = nil;
	}
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
	// Parse the command-line
    NSMutableString *cl = [[NSMutableString alloc] init];
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    
    for(NSUInteger i = 1;i < [args count]; ++i)
    {
        [cl appendString:[args objectAtIndex:i]];
        [cl appendString:@" "];
    }
	
	commandLine.set([cl UTF8String]);
	//[cl release];
	
	stateMachine = new pvr::system::StateMachine((__bridge pvr::system::OSApplication)self, commandLine,NULL);

	if(!stateMachine)
	{
		NSLog(@"Failed to allocate stateMachine.\n");
        return;
	}
	
	if(stateMachine->init() != pvr::Result::Success)
	{
		NSLog(@"Failed to initialize stateMachine.\n");
        delete stateMachine;
        stateMachine = NULL;
		return;
	}
	
	mainLoopTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / kFPS) target:self selector:@selector(mainLoop) userInfo:nil repeats:YES];	
}

- (void) applicationWillTerminate:(NSNotification *)notification
{
    [mainLoopTimer invalidate];
    mainLoopTimer = nil;
    
    if(stateMachine->getCurrentState() == pvr::system::StateMachine::StateRenderScene)
    {
        stateMachine->executeOnce(pvr::system::StateMachine::StateReleaseView);
    }
    stateMachine->execute();
    delete stateMachine;
    stateMachine = NULL;
}

@end

