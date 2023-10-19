@import AppKit;
@import Foundation;

#import "wrapper.h"

const char *get_secret_key(const char *key)
{
	return "dummy";
}

float get_screen_scale()
{
	return [NSScreen mainScreen].backingScaleFactor;
}
