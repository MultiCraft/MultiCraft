@import AppKit;
@import AVFoundation;
@import Foundation;

#import "wrapper.h"

/* Initialization Apple Specific Things */
void init_apple()
{
	NSLog(@"[INIT] initialized");
}

/* Get Scale Factor */
float get_screen_scale()
{
	return (float) [NSScreen mainScreen].backingScaleFactor;
}

void get_upgrade(const char *key)
{
	NSLog(@"[EVENT] got upgrade event: %s", key);
}

const char *get_secret_key(const char *key)
{
	return "dummy";
}
