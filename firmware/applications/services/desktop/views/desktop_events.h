#pragma once

typedef enum {
    DesktopMainEventLock,
    // Stock lock menu: in R0N1N only reached from Up in dummy mode; Down opens
    // Control Center (DesktopMainEventOpenControlCenter) instead.
    DesktopMainEventOpenLockMenu,
    DesktopMainEventOpenFavoriteLeftShort,
    DesktopMainEventOpenFavoriteLeftLong,
    DesktopMainEventOpenFavoriteRightShort,
    DesktopMainEventOpenFavoriteRightLong,
    // R0N1N navigation law (docs/UX_DESIGN.md): Up = Quick Actions, hold OK =
    // Recent, Down = Control Center, Left/Right = sections, hold Back = Search.
    // Archive (the stock Down-short target) is in Quick Actions and the menu.
    DesktopMainEventOpenFavorites,
    DesktopMainEventOpenRecent,
    DesktopMainEventOpenControlCenter,
    DesktopMainEventOpenSectionsLeft,
    DesktopMainEventOpenSectionsRight,
    DesktopMainEventOpenSearch,
    DesktopMainEventOpenMenu,
    DesktopMainEventOpenDebug,
    DesktopMainEventOpenPowerOff,
    // R0N1N simple mode: any arrow or OK on its Home
    DesktopMainEventOpenSimpleMenu,

    DesktopDummyEventOpenLeft,
    DesktopDummyEventOpenDown,
    DesktopDummyEventOpenOk,

    DesktopLockedEventUnlocked,
    DesktopLockedEventUpdate,
    DesktopLockedEventShowPinInput,
    DesktopLockedEventDoorsClosed,

    DesktopPinInputEventResetWrongPinLabel,
    DesktopPinInputEventUnlocked,
    DesktopPinInputEventUnlockFailed,
    DesktopPinInputEventBack,

    DesktopPinTimeoutExit,

    DesktopDebugEventDeed,
    DesktopDebugEventWrongDeed,
    DesktopDebugEventSaveState,
    DesktopDebugEventExit,

    DesktopLockMenuEventLock,
    DesktopLockMenuEventDummyModeOn,
    DesktopLockMenuEventDummyModeOff,
    DesktopLockMenuEventStealthModeOn,
    DesktopLockMenuEventStealthModeOff,

    DesktopAnimationEventCheckAnimation,
    DesktopAnimationEventNewIdleAnimation,
    DesktopAnimationEventInteractAnimation,

    DesktopSlideshowCompleted,
    DesktopSlideshowPoweroff,

    DesktopHwMismatchExit,

    DesktopEnclaveExit,

    // Global events
    DesktopGlobalBeforeAppStarted,
    DesktopGlobalAfterAppFinished,
    DesktopGlobalAutoLock,
    DesktopGlobalApiUnlock,
    DesktopGlobalSaveSettings,
    DesktopGlobalReloadSettings,
    // R0N1N Recent apps: the app announced by the last BeforeAppStarted
    // actually ran and has now exited -- see desktop.c.
    DesktopGlobalAppStopped,
} DesktopEvent;
