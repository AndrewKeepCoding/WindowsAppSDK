﻿// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

// THIS FILE IS AUTOMATICALLY GENERATED; DO NOT EDIT IT

// INPUT FILE: TerminalVelocityFeatures-Test.xml

// Feature constants
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTAL_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEW_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_STABLE_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_WINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ID_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTAL_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEW_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_STABLE_ENABLED 1
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_WINDOWSINBOX_ENABLED 0
#define WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ID_ENABLED 1

#if defined(__cplusplus)

namespace Test::TerminalVelocity::Features
{

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTAL_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_Experimental
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTAL_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalPreview
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalPreviewStable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalPreviewWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalStable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalStableWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_ExperimentalWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEW_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_Preview
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEW_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_PreviewStableWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_STABLE_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_Stable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_STABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_WINDOWSINBOX_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_AlwaysEnabledChannels_WindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ALWAYSENABLEDCHANNELS_WINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ID_ENABLED_mismatch", "AlwaysDisabled"))
struct Feature_AlwaysDisabled_Id
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSDISABLED_ID_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTAL_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_Experimental
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTAL_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalPreview
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEW_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalPreviewStable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWSTABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalPreviewWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALPREVIEWWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalStable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalStableWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALSTABLEWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_ExperimentalWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_EXPERIMENTALWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEW_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_Preview
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEW_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_PreviewStableWindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_PREVIEWSTABLEWINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_STABLE_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_Stable
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_STABLE_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_WINDOWSINBOX_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_AlwaysDisabledChannels_WindowsInbox
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ALWAYSDISABLEDCHANNELS_WINDOWSINBOX_ENABLED == 1; }
};

__pragma(detect_mismatch("ODR_violation_WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ID_ENABLED_mismatch", "AlwaysEnabled"))
struct Feature_AlwaysEnabled_Id
{
    static constexpr bool IsEnabled() { return WINDOWSAPPRUNTIME_FEATURE_ALWAYSENABLED_ID_ENABLED == 1; }
};

} // namespace Test.TerminalVelocity.Features

#endif // defined(__cplusplus)
