#include "macosutils.h"

#include <AppKit/AppKit.h>

void macos_disable_tabs()
{
#ifdef AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER
    // Remove (don't allow) the "Show Tab Bar" menu item from the "View" menu, if supported

    if ([NSWindow respondsToSelector:@selector(allowsAutomaticWindowTabbing)])
        NSWindow.allowsAutomaticWindowTabbing = NO;
#endif
}
