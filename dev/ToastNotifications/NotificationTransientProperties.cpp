﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ToastNotification.h"

#include "ToastNotificationUtility.h"
#include "NotificationTransientProperties.h"

namespace winrt
{
    using namespace winrt::Microsoft::Windows::ToastNotifications;
}

NotificationTransientProperties::NotificationTransientProperties(winrt::ToastNotification const& toastNotification)
{
    m_suppressPopup = toastNotification.SuppressDisplay();

    auto priority = toastNotification.Priority();

    if (priority == winrt::Microsoft::Windows::ToastNotifications::ToastPriority::High)
    {
        m_toastNotificationPriority = ::ABI::Microsoft::Internal::ToastNotifications::ToastNotificationPriority::ToastNotificationPriority_High;
    }
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_OfflineCacheCount(_Out_ unsigned long long* offlineCacheCount) noexcept
{
    *offlineCacheCount = 0;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_OfflineBundleId(_Out_ HSTRING* offlineBundleId) noexcept try
{
    *offlineBundleId = nullptr;
    return S_OK;
}
CATCH_RETURN()

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_ServerCacheRollover(_Out_ boolean* serverCacheRollover) noexcept
{
    *serverCacheRollover = false;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_SuppressPopup(_Out_ boolean* suppressPopup) noexcept
{
    auto lock = m_lock.lock_shared();

    *suppressPopup = m_suppressPopup;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_RecurrenceId(_Out_ GUID* recurrenceId) noexcept
{
    *recurrenceId = GUID_NULL;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_MessageId(_Out_ GUID* messageId) noexcept
{
    *messageId = GUID_NULL;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_ToastNotificationPriority(_Out_ ::ABI::Microsoft::Internal::ToastNotifications::ToastNotificationPriority* priority) noexcept
{
    auto lock = m_lock.lock_shared();

    *priority = m_toastNotificationPriority;
    return S_OK;
}

STDMETHODIMP_(HRESULT __stdcall) NotificationTransientProperties::get_CorrelationVector(_Out_ HSTRING* correlationVector) noexcept
{
    *correlationVector = nullptr;
    return S_OK;
}
