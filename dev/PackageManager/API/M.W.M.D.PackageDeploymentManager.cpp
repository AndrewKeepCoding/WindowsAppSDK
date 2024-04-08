// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "M.W.M.D.PackageDeploymentManager.h"
#include "Microsoft.Windows.Management.Deployment.PackageDeploymentManager.g.cpp"

#include "M.W.M.D.PackageDeploymentResult.h"
#include "MsixPackageManager.h"
#include "PackageDeploymentResolver.h"

#include "PackageManagerTelemetry.h"

#include <windows.foundation.h>
#include <windows.management.deployment.h>
#if !defined(TODO_UpdatedWindowsSDK_with_Windows_Management_Deployment_IRemovePackageOptions)
// Until we have access to the updated Windows SDK we can't use RemovePackageOptions
// but we need *something* to satisfy PackageManagement_RemovePackageByUriAsync() in the FrameworkUDK
// so we'll make a placeholder definition to satisfy the compiler enough to let us call
// PackageManagement_RemovePackageByUriAsync(uri, nullptr). Remove this once the updated
// Windows SDK makes this unnecessary.
namespace ABI::Windows::Management::Deployment
{
    typedef void * IRemovePackageOptions;
}
#endif
//
#include <FrameworkUdk/PackageManagement.h>
#include <FrameworkUdk/UupStateRepository.h>

static_assert(static_cast<int>(winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::Default) == static_cast<int>(winrt::Windows::Management::Deployment::StubPackageOption::Default),
              "winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::Default != winrt::Windows::Management::Deployment::StubPackageOption::Default");
static_assert(static_cast<int>(winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::InstallFull) == static_cast<int>(winrt::Windows::Management::Deployment::StubPackageOption::InstallFull),
              "winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::InstallFull != winrt::Windows::Management::Deployment::StubPackageOption::InstallFull");
static_assert(static_cast<int>(winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::InstallStub) == static_cast<int>(winrt::Windows::Management::Deployment::StubPackageOption::InstallStub),
              "winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::InstallStub != winrt::Windows::Management::Deployment::StubPackageOption::InstallStub");
static_assert(static_cast<int>(winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::UsePreference) == static_cast<int>(winrt::Windows::Management::Deployment::StubPackageOption::UsePreference),
              "winrt::Microsoft::Windows::Management::Deployment::StubPackageOption::UsePreference != winrt::Windows::Management::Deployment::StubPackageOption::UsePreference");

static PackageManagement_ArchitectureType ToArchitectureType(const winrt::Windows::System::ProcessorArchitecture architecture)
{
    switch (architecture)
    {
        case winrt::Windows::System::ProcessorArchitecture::X86:        return PackageManagement_ArchitectureType_X86;
        case winrt::Windows::System::ProcessorArchitecture::Arm:        return PackageManagement_ArchitectureType_Arm;
        case winrt::Windows::System::ProcessorArchitecture::X64:        return PackageManagement_ArchitectureType_X64;
        case winrt::Windows::System::ProcessorArchitecture::Neutral:    return PackageManagement_ArchitectureType_Neutral;
        case winrt::Windows::System::ProcessorArchitecture::Arm64:      return PackageManagement_ArchitectureType_Arm64;
        case winrt::Windows::System::ProcessorArchitecture::X86OnArm64: return PackageManagement_ArchitectureType_X86A64;
        case winrt::Windows::System::ProcessorArchitecture::Unknown:    return PackageManagement_ArchitectureType_Unknown;
        default: THROW_HR_MSG(E_UNEXPECTED, "%d", static_cast<int>(architecture));
    }
}

namespace winrt::Microsoft::Windows::ApplicationModel::DynamicDependency
{
    DEFINE_ENUM_FLAG_OPERATORS(PackageDependencyProcessorArchitectures)
}
static PackageManagement_ArchitectureType ToArchitectureType(const winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures processorArchitectureFilter)
{
    auto architectureType{ PackageManagement_ArchitectureType_None };
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Neutral))
    {
        architectureType |= PackageManagement_ArchitectureType_Neutral;
    }
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X86))
    {
        architectureType |= PackageManagement_ArchitectureType_X86;
    }
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X64))
    {
        architectureType |= PackageManagement_ArchitectureType_X64;
    }
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Arm))
    {
        architectureType |= PackageManagement_ArchitectureType_Arm;
    }
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Arm64))
    {
        architectureType |= PackageManagement_ArchitectureType_Arm64;
    }
    if (WI_IsFlagSet(processorArchitectureFilter, winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X86OnArm64))
    {
        architectureType |= PackageManagement_ArchitectureType_X86A64;
    }
    return architectureType;
}

static winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures ToPackageDependencyProcessorArchitectures(const winrt::Windows::System::ProcessorArchitecture architecture)
{
    switch (architecture)
    {
        case winrt::Windows::System::ProcessorArchitecture::X86:        return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X86;
        case winrt::Windows::System::ProcessorArchitecture::X64:        return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X64;
        case winrt::Windows::System::ProcessorArchitecture::Arm:        return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Arm;
        case winrt::Windows::System::ProcessorArchitecture::Arm64:      return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Arm64;
        case winrt::Windows::System::ProcessorArchitecture::X86OnArm64: return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::X86OnArm64;
        case winrt::Windows::System::ProcessorArchitecture::Neutral:    return winrt::Microsoft::Windows::ApplicationModel::DynamicDependency::PackageDependencyProcessorArchitectures::Neutral;
        case winrt::Windows::System::ProcessorArchitecture::Unknown:    THROW_HR_MSG(E_UNEXPECTED, "Unsupported architecture 0x%X", architecture);
        default: THROW_HR_MSG(E_UNEXPECTED, "Unknown architecture 0x%X", architecture);
    }
}

namespace winrt::Microsoft::Windows::Management::Deployment::implementation
{
    winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager PackageDeploymentManager::GetDefault()
    {
        return winrt::make<winrt::Microsoft::Windows::Management::Deployment::implementation::PackageDeploymentManager>();
    }
    bool PackageDeploymentManager::IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature feature)
    {
        switch (feature)
        {
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageReadyOrNewerAvailable:
            {
                // Relies on PackageManagement_IsRegisteredOrNewerAvailable() exist in Microsoft.FrameworkUdk and enabled
#define WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE   48914090    //TODO Move to FrameworkUdk
                return ::WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE>();
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageReadyOrNewerAvailableByUri:
            {
                // Relies on PackageManagement_IsRegisteredOrNewerAvailable() exist in Microsoft.FrameworkUdk and enabled
#define WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE   48914090    //TODO Move to FrameworkUdk
                return ::WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE>();
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageSetReadyOrNewerAvailable:
            {
                // Relies on PackageManagement_IsRegisteredOrNewerAvailable() exist in Microsoft.FrameworkUdk and enabled
#define WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE   48914090    //TODO Move to FrameworkUdk
                return ::WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_ISPACKAGEREADYORNEWERAVAILABLE>();
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::EnsurePackageReadyByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RemovePackageByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RemovePackageSetAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageSetAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageSetAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageSetAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageByUriAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            case winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageSetAsync:
            {
                //TODO Feature id lookup
                return false;
            }
            default:
            {
                (void)LOG_HR_MSG(E_UNEXPECTED, "Feature:%d", static_cast<int>(feature));
                return false;
            }
        }
    }
    bool PackageDeploymentManager::IsPackageReady(hstring const& package)
    {
        if (VerifyPackageFullName(package.c_str()) == ERROR_SUCCESS)
        {
            return IsReadyByPackageFullName(package);
        }

        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return IsPackageReadyByUri(packageUri);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    bool PackageDeploymentManager::IsPackageReadyByUri(winrt::Windows::Foundation::Uri const& packageUri)
    {
        // Currently supported URI schemes: ms-uup
        const auto packageFullNames{ GetPackageFullNamesFromUupProductUriIfMsUup(packageUri) };
        if (packageFullNames)
        {
            for (PCWSTR packageFullName : packageFullNames)
            {
                if (!IsReadyByPackageFullName(packageFullName))
                {
                    return false;
                }
            }
            return true;
        }

        THROW_HR_MSG(E_INVALIDARG, "%ls", packageUri.ToString().c_str());
    }
    bool PackageDeploymentManager::IsPackageSetReady(winrt::Microsoft::Windows::Management::Deployment::PackageSet const& packageSet)
    {
        Validate(packageSet);

        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSet.Items())
        {
            if (!IsReady(packageSetItem))
            {
                (void)LOG_HR_MSG(MSIXPACKAGEMANAGER_E_PACKAGE_SCAN_FAILED,
                                 "Id=%ls PackageFamilyName=%ls MinVersion=%hu.%hu.%hu.%hu ArchitectureFilter:0x%X",
                                 packageSetItem.Id().c_str(),
                                 packageSetItem.PackageFamilyName().c_str(),
                                 packageSetItem.MinVersion().Major,
                                 packageSetItem.MinVersion().Minor,
                                 packageSetItem.MinVersion().Build,
                                 packageSetItem.MinVersion().Revision,
                                 packageSetItem.ProcessorArchitectureFilter());
                return false;
            }
        }
        return true;
    }
    winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus PackageDeploymentManager::IsPackageReadyOrNewerAvailable(hstring const& package)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageReadyOrNewerAvailable));

        if (VerifyPackageFullName(package.c_str()) == ERROR_SUCCESS)
        {
            return IsReadyOrNewerAvailableByPackageFullName(package.c_str());
        }

        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return IsPackageReadyOrNewerAvailableByUri(packageUri);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus PackageDeploymentManager::IsPackageReadyOrNewerAvailableByUri(winrt::Windows::Foundation::Uri const& packageUri)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageReadyOrNewerAvailableByUri));

        // Currently supported URI schemes: ms-uup
        const auto schemeName{ packageUri.SchemeName() };
        const auto uupProductId{ GetUupProductIdIfMsUup(packageUri) };
        if (!uupProductId.empty())
        {
            wil::unique_cotaskmem_array_ptr<wil::unique_cotaskmem_string> packageFullNames;
            THROW_IF_FAILED_MSG(Uup_SRFindPackageFullNamesByUupProductId(uupProductId.c_str(), packageFullNames.size_address<UINT32>(), packageFullNames.addressof()), "%s", uupProductId.c_str());

            bool anyNewerAvailable{};
            const auto packageUriAsString{ packageUri.ToString() };
            for (PCWSTR packageFullName : packageFullNames)
            {
                const auto packageIdentity{ ::AppModel::Identity::PackageIdentity::FromPackageFullName(packageFullName) };
                const auto packageFamilyName{ packageIdentity.PackageFamilyName() };
                const auto minVersion{ packageIdentity.Version().Version };
                const auto architectureType{ ToArchitectureType(packageIdentity.Architecture()) };
                BOOL isRegistered{};
                BOOL isNewerAvailable{};
                THROW_IF_FAILED_MSG(PackageManagement_IsRegisteredOrNewerAvailable(nullptr, packageFamilyName.c_str(), minVersion, architectureType, &isRegistered, &isNewerAvailable), "%s", packageUriAsString.c_str());
                if (!isRegistered)
                {
                    // At least one package isn't ready so we have our answer
                    return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NotReady;
                }
                if (!anyNewerAvailable & !!isNewerAvailable)
                {
                    anyNewerAvailable = true;
                }
            }
            if (anyNewerAvailable)
            {
                // All packages are registered but at least 1 has a newer version available
                return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NewerAvailable;
            }

            // All packages are registered and none have anything newer avaiable
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::Ready;
        }

        THROW_HR_MSG(E_INVALIDARG, "%ls", packageUri.ToString().c_str());
    }
    winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus PackageDeploymentManager::IsPackageSetReadyOrNewerAvailable(winrt::Microsoft::Windows::Management::Deployment::PackageSet const& packageSet)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageSetReadyOrNewerAvailable));

        Validate(packageSet);

        bool newerAvailable{};
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSet.Items())
        {
            const auto status{ IsReadyOrNewerAvailable(packageSetItem) };
            if (status == winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NotReady)
            {
                (void)LOG_HR_MSG(MSIXPACKAGEMANAGER_E_PACKAGE_SCAN_FAILED,
                                 "Id=%ls PackageFamilyName=%ls MinVersion=%hu.%hu.%hu.%hu ArchitectureFilter:0x%X",
                                 packageSetItem.Id().c_str(),
                                 packageSetItem.PackageFamilyName().c_str(),
                                 packageSetItem.MinVersion().Major,
                                 packageSetItem.MinVersion().Minor,
                                 packageSetItem.MinVersion().Build,
                                 packageSetItem.MinVersion().Revision,
                                 packageSetItem.ProcessorArchitectureFilter());
                return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NotReady;
            }
            if (!newerAvailable && (status == winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NewerAvailable))
            {
                (void)LOG_HR_MSG(MSIXPACKAGEMANAGER_E_PACKAGE_SCAN_NEWER_AVAILABLE,
                                 "Id=%ls PackageFamilyName=%ls MinVersion=%hu.%hu.%hu.%hu ArchitectureFilter:0x%X",
                                 packageSetItem.Id().c_str(),
                                 packageSetItem.PackageFamilyName().c_str(),
                                 packageSetItem.MinVersion().Major,
                                 packageSetItem.MinVersion().Minor,
                                 packageSetItem.MinVersion().Build,
                                 packageSetItem.MinVersion().Revision,
                                 packageSetItem.ProcessorArchitectureFilter());
                newerAvailable = true;
            }
        }
        return newerAvailable ?
            winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NewerAvailable :
            winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::Ready;
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::EnsurePackageReadyAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::EnsureReadyOptions options)
    {
        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return EnsurePackageReadyByUriAsync(packageUri, options);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::EnsurePackageReadyByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::EnsureReadyOptions options)
    {
        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::EnsurePackageSetReadyAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::EnsureReadyOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::EnsurePackageSetReadyAsync::Start(packageSet.Id()) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        Validate(packageSet);

        // Is there any work to do?
        packageDeploymentProgress.Status = PackageDeploymentProgressStatus::InProgress;
        const double c_progressPercentageStartOfIsReady{ 0.01 };
        packageDeploymentProgress.Progress = c_progressPercentageStartOfIsReady;
        progress(packageDeploymentProgress);
        if (IsPackageSetReady(packageSet))
        {
            co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, winrt::guid{});
        }

        const double c_progressPercentageStartOfInstalls{ 0.10 };
        packageDeploymentProgress.Progress = c_progressPercentageStartOfInstalls;
        progress(packageDeploymentProgress);
        auto packageSetItems{ packageSet.Items() };
        const double progressIncrementPerPackageSetItem{ (1.0 - c_progressPercentageStartOfInstalls) / packageSetItems.Size() };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSetItems)
        {
            const auto packageFamilyName{ packageSetItem.PackageFamilyName() };
            const auto packageUri{ GetEffectivePackageUri(packageSet, packageSetItem) };
            try
            {
                error = LOG_IF_FAILED_MSG(EnsureReadyAsync(packageUri, packageSetItem, options, packageDeploymentProgress, progress, progressIncrementPerPackageSetItem, extendedError, errorText, activityId),
                                          "Error:0x%08X (0x%08X) PackageFamilyName:%ls PackageUri:%ls : %ls",
                                          error, extendedError, packageFamilyName.c_str(), packageUri.ToString().c_str(), errorText.c_str());
            }
            catch (...)
            {
                auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
                error = LOG_HR_MSG(exception.code(), "Error:0x%08X (0x%08X) PackageFamilyName:%ls PackageUri:%ls : %ls", error, extendedError, packageFamilyName.c_str(), packageUri.ToString().c_str(), errorText.c_str());
            }
            if (FAILED(error))
            {
                co_return winrt::make<PackageDeploymentResult>(
                    PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
            }
            packageDeploymentProgress.Progress = packageDeploymentProgress.Progress + progressIncrementPerPackageSetItem;
            progress(packageDeploymentProgress);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageSet.Id());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::AddPackageAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions options)
    {
        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return AddPackageByUriAsync(packageUri, options);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::AddPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions options)
    {
        const winrt::hstring packageUriAsString{ packageUri.ToString() };
#if defined(TODO_SUPPORT_APPINSTALLER)
        if (IsUriEndsWith(packageUri, L".appinstaller"))
        {
            return AddPackageByAppInstallerFileAsync(packageUri, options);
        }
#endif
        auto logTelemetry{ PackageManagementTelemetry::AddPackageByUriAsync::Start(packageUriAsString) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::Windows::Management::Deployment::AddPackageOptions addOptions{ ToOptions(options) };
        const double progressMaxPerPackage{ 1.0 };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(AddPackage(packageUri, addOptions, packageDeploymentProgress, progress, progressMaxPerPackage, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageUri:%ls",
                                      extendedError, packageUri.ToString().c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageUri:%ls", extendedError, packageUri.ToString().c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageUriAsString);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::AddPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::AddPackageSetAsync::Start(packageSet.Id()) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        Validate(packageSet);

        packageDeploymentProgress.Status = PackageDeploymentProgressStatus::InProgress;
        const double c_progressPercentageStartOfInstalls{ 0.10 };
        packageDeploymentProgress.Progress = c_progressPercentageStartOfInstalls;
        progress(packageDeploymentProgress);
        auto packageSetItems{ packageSet.Items() };
        const double progressIncrementPerPackageSetItem{ (1.0 - c_progressPercentageStartOfInstalls) / packageSetItems.Size() };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSetItems)
        {
            const auto packageUri{ GetEffectivePackageUri(packageSet, packageSetItem) };
            try
            {
                const auto progressBeforePackage{ packageDeploymentProgress.Progress };
                error = LOG_IF_FAILED_MSG(AddPackage(packageUri, options, packageDeploymentProgress, progress, progressIncrementPerPackageSetItem, extendedError, errorText, activityId),
                                          "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls",
                                          extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
                const auto progressAfterPackage{ progressBeforePackage + progressIncrementPerPackageSetItem };
                if (packageDeploymentProgress.Progress < progressAfterPackage)
                {
                    packageDeploymentProgress.Progress = progressAfterPackage;
                    progress(packageDeploymentProgress);
                }
            }
            catch (...)
            {
                const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
                error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls", extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
            }
            if (FAILED(error))
            {
                co_return winrt::make<PackageDeploymentResult>(
                    PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
            }
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageSet.Id());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::StagePackageAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::StagePackageOptions options)
    {
        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return StagePackageByUriAsync(packageUri, options);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::StagePackageByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::StagePackageOptions options)
    {
        const winrt::hstring packageUriAsString{ packageUri.ToString() };
        auto logTelemetry{ PackageManagementTelemetry::StagePackageByUriAsync::Start(packageUriAsString) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::Windows::Management::Deployment::StagePackageOptions stageOptions{ ToOptions(options) };
        const double progressMaxPerPackage{ 1.0 };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(StagePackage(packageUri, stageOptions, packageDeploymentProgress, progress, progressMaxPerPackage, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageUri:%ls",
                                      extendedError, packageUri.ToString().c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageUri:%ls", extendedError, packageUri.ToString().c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageUriAsString);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::StagePackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::StagePackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::StagePackageSetAsync::Start(packageSet.Id()) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        Validate(packageSet);

        packageDeploymentProgress.Status = PackageDeploymentProgressStatus::InProgress;
        const double c_progressPercentageStartOfInstalls{ 0.10 };
        packageDeploymentProgress.Progress = c_progressPercentageStartOfInstalls;
        progress(packageDeploymentProgress);
        auto packageSetItems{ packageSet.Items() };
        const double progressIncrementPerPackageSetItem{ (1.0 - c_progressPercentageStartOfInstalls) / packageSetItems.Size() };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSetItems)
        {
            const auto packageUri{ GetEffectivePackageUri(packageSet, packageSetItem) };
            try
            {
                const auto progressBeforePackage{ packageDeploymentProgress.Progress };
                error = LOG_IF_FAILED_MSG(StagePackage(packageUri, options, packageDeploymentProgress, progress, progressIncrementPerPackageSetItem, extendedError, errorText, activityId),
                                          "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls",
                                          extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
                const auto progressAfterPackage{ progressBeforePackage + progressIncrementPerPackageSetItem };
                if (packageDeploymentProgress.Progress < progressAfterPackage)
                {
                    packageDeploymentProgress.Progress = progressAfterPackage;
                    progress(packageDeploymentProgress);
                }
            }
            catch (...)
            {
                const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
                error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls", extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
            }
            if (FAILED(error))
            {
                co_return winrt::make<PackageDeploymentResult>(
                    PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
            }
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageSet.Id());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RegisterPackageAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions options)
    {
        if (VerifyPackageFamilyName(package.c_str()) == ERROR_SUCCESS)
        {
            return RegisterPackageByPackageFullNameAsync(package, options);
        }
        else if (VerifyPackageFullName(package.c_str()) == ERROR_SUCCESS)
        {
            return RegisterPackageByPackageFamilyNameAsync(package, options);
        }

        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return RegisterPackageByUriAsync(packageUri, options);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RegisterPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions options)
    {
        const winrt::hstring packageUriAsString{ packageUri.ToString() };
        auto logTelemetry{ PackageManagementTelemetry::RegisterPackageByUriAsync::Start(packageUriAsString) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::Windows::Management::Deployment::RegisterPackageOptions registerOptions{ ToOptions(options) };
        const double progressMaxPerPackage{ 1.0 };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(RegisterPackage(packageUri, registerOptions, packageDeploymentProgress, progress, progressMaxPerPackage, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageUri:%ls",
                                      extendedError, packageUri.ToString().c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageUri:%ls", extendedError, packageUri.ToString().c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageUriAsString);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RegisterPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::RegisterPackageSetAsync::Start(packageSet.Id()) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        Validate(packageSet);

        packageDeploymentProgress.Status = PackageDeploymentProgressStatus::InProgress;
        const double c_progressPercentageStartOfInstalls{ 0.10 };
        packageDeploymentProgress.Progress = c_progressPercentageStartOfInstalls;
        progress(packageDeploymentProgress);
        auto packageSetItems{ packageSet.Items() };
        const double progressIncrementPerPackageSetItem{ (1.0 - c_progressPercentageStartOfInstalls) / packageSetItems.Size() };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSetItems)
        {
            const auto packageUri{ GetEffectivePackageUri(packageSet, packageSetItem) };
            try
            {
                const auto progressBeforePackage{ packageDeploymentProgress.Progress };
                error = LOG_IF_FAILED_MSG(RegisterPackage(packageUri, options, packageDeploymentProgress, progress, progressIncrementPerPackageSetItem, extendedError, errorText, activityId),
                                          "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls",
                                          extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
                const auto progressAfterPackage{ progressBeforePackage + progressIncrementPerPackageSetItem };
                if (packageDeploymentProgress.Progress < progressAfterPackage)
                {
                    packageDeploymentProgress.Progress = progressAfterPackage;
                    progress(packageDeploymentProgress);
                }
            }
            catch (...)
            {
                const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
                error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls", extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str());
            }
            if (FAILED(error))
            {
                co_return winrt::make<PackageDeploymentResult>(
                    PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
            }
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageSet.Id());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RemovePackageAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options)
    {
        if (VerifyPackageFamilyName(package.c_str()) == ERROR_SUCCESS)
        {
            return RemovePackageByFamilyNameAsync(package, options);
        }
        else if (VerifyPackageFullName(package.c_str()) == ERROR_SUCCESS)
        {
            return RemovePackageByFullNameAsync(package, options);
        }

        const winrt::Windows::Foundation::Uri packageUri{ package };
        const auto packageAbsoluteUri{ packageUri.AbsoluteUri() };
        if (!packageAbsoluteUri.empty())
        {
            return RemovePackageByUriAsync(packageUri, options);
        }
        THROW_HR_MSG(E_INVALIDARG, "%ls", package.c_str());
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RemovePackageByFullNameAsync(hstring packageFullName, winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::RemovePackageAsync::Start(packageFullName) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::Windows::Management::Deployment::RemovalOptions removeOptions{ ToOptions(options) };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(RemovePackageByFullName(packageFullName, options, removeOptions, packageDeploymentProgress, progress, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageUri:%ls",
                                      extendedError, packageFullName.c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageUri:%ls", extendedError, packageFullName.c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageFullName);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RemovePackageByFamilyNameAsync(hstring packageFamilyName, winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::RemovePackageAsync::Start(packageFamilyName) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::hstring packageFullName{ packageFamilyName };
        winrt::Windows::Management::Deployment::RemovalOptions removeOptions{ ToOptions(options) };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            // Resource packages are automagically removed when their associated Main|Optional package is removed
            const auto packageTypes{ winrt::Windows::Management::Deployment::PackageTypes::Framework |
                                     winrt::Windows::Management::Deployment::PackageTypes::Main |
                                     winrt::Windows::Management::Deployment::PackageTypes::Optional };
            auto packages{ m_packageManager.FindPackagesForUserWithPackageTypes(winrt::hstring(), packageFamilyName, packageTypes) };
            if (packages)
            {
                for (const winrt::Windows::ApplicationModel::Package& package : packages)
                {
                    packageFullName = package.Id().FullName();
                    error = LOG_IF_FAILED_MSG(RemovePackageByFullName(packageFullName, options, removeOptions, packageDeploymentProgress, progress, extendedError, errorText, activityId),
                                              "ExtendedError:0x%08X PackageUri:%ls",
                                              extendedError, packageFullName.c_str());
                }
            }
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageUri:%ls", extendedError, packageFullName.c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageFamilyName);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RemovePackageByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RemovePackageByUriAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_RemovePackageByUriAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RemovePackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RemovePackageSetAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_RemovePackageByUriAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ResetPackageAsync(hstring package)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ResetPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageByUriAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_ResetPackageAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ResetPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackageSetAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_ResetPackageAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RepairPackageAsync(hstring package)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RepairPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageByUriAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_RepairPackageAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RepairPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackageSetAsync));

        //TODO Awaiting FrameworkUdk update with PackageManagement_RepairPackageAsync()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ProvisionPackageAsync(hstring package, winrt::Microsoft::Windows::Management::Deployment::ProvisionPackageOptions options)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ProvisionPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::ProvisionPackageOptions options)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageByUriAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::ProvisionPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet, winrt::Microsoft::Windows::Management::Deployment::ProvisionPackageOptions options)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ProvisionPackageSetAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::DeprovisionPackageAsync(hstring package)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::DeprovisionPackageByUriAsync(winrt::Windows::Foundation::Uri packageUri)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageByUriAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::DeprovisionPackageSetAsync(winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet)
    {
        THROW_HR_IF(E_NOTIMPL, IsPackageDeploymentFeatureSupported(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::DeprovisionPackageSetAsync));

        //TODO Awaiting FrameworkUdk update with Uup_SRFindPackageFullNamesByUupProductId()
        throw hresult_not_implemented();
    }
    bool PackageDeploymentManager::IsPackageRegistrationPending(hstring const& packageFullName)
    {
        return IsPackageRegistrationPendingForUser(nullptr, packageFullName.c_str());
    }
    bool PackageDeploymentManager::IsPackageRegistrationPendingForUser(hstring const& userSecurityId, hstring const& packageFullName)
    {
        wil::unique_any_psid userSid{ ::Security::User::StringToSid(userSecurityId.c_str()) };
        return IsPackageRegistrationPendingForUser(userSid.get(), packageFullName.c_str());
    }

    bool PackageDeploymentManager::IsPackageRegistrationPendingForUser(PSID userSid, PCWSTR packageFullName)
    {
        BOOL isPending{};
        THROW_IF_FAILED(::PackageManagement_IsPackageRegistrationPending(userSid, packageFullName, &isPending));
        return !!isPending;
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::AddPackageByAppInstallerFileAsync(winrt::Windows::Foundation::Uri packageUri, winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions options)
    {
        //TODO add via AddPackageByAppInstallerFileAsync()
        throw hresult_not_implemented();
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RegisterPackageByPackageFamilyNameAsync(winrt::hstring const& packageFamilyName, winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::RegisterPackageByPackageFamilyNameAsync::Start(packageFamilyName) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(RegisterPackageByPackageFamilyName(packageFamilyName, options, packageDeploymentProgress, progress, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageFamilyName:%ls",
                                      extendedError, packageFamilyName.c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageFamilyName:%ls", extendedError, packageFamilyName.c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageFamilyName);
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>
    PackageDeploymentManager::RegisterPackageByPackageFullNameAsync(winrt::hstring const& packageFullName, winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions options)
    {
        auto logTelemetry{ PackageManagementTelemetry::RegisterPackageByPackageFullNameAsync::Start(packageFullName) };

        auto strong{ get_strong() };

        auto cancellation{ co_await winrt::get_cancellation_token() };
        cancellation.enable_propagation(true);

        // Allow to register the progress and complete handler
        logTelemetry.IgnoreCurrentThread();
        co_await resume_background();   // Allow to register the progress and complete handler
        auto logTelemetryContinuation{ logTelemetry.ContinueOnCurrentThread() };

        auto progress{ co_await winrt::get_progress_token() };
        auto packageDeploymentProgress{
            winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress{
                PackageDeploymentProgressStatus::Queued, 0} };
        progress(packageDeploymentProgress);

        // Check parameter(s)
        //TODO Validate(packageSet);

        winrt::Windows::Management::Deployment::RegisterPackageOptions registerOptions{ ToOptions(options) };
        HRESULT error{};
        HRESULT extendedError{};
        winrt::hstring errorText;
        winrt::guid activityId{};
        try
        {
            error = LOG_IF_FAILED_MSG(RegisterPackageByPackageFullName(packageFullName, registerOptions, packageDeploymentProgress, progress, extendedError, errorText, activityId),
                                      "ExtendedError:0x%08X PackageFullName:%ls",
                                      extendedError, packageFullName.c_str());
        }
        catch (...)
        {
            const auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            error = LOG_HR_MSG(exception.code(), "ExtendedError:0x%08X PackageFullName:%ls", extendedError, packageFullName.c_str());
        }
        if (FAILED(error))
        {
            co_return winrt::make<PackageDeploymentResult>(
                PackageDeploymentStatus::CompletedFailure, activityId, error, extendedError, errorText);
        }

        co_return winrt::make<PackageDeploymentResult>(PackageDeploymentStatus::CompletedSuccess, activityId);

        logTelemetry.Stop(packageFullName);
    }

    winrt::hstring PackageDeploymentManager::GetUupProductIdIfMsUup(winrt::Windows::Foundation::Uri const& uri) const
    {
        const auto schemeName{ uri.SchemeName() };
        if (CompareStringOrdinal(schemeName.c_str(), -1, L"ms-uup", -1, TRUE) == CSTR_EQUAL)
        {
            // Support legacy behavior (for now)
            //
            // ms-uup://Product/id
            const auto uriAsString{ uri.ToString() };
            const std::wstring_view legacyPrefix{ L"ms-uup://Product/" };
            if ((uriAsString.size() > legacyPrefix.length()) &&
                StringEqualsNoCase(uriAsString.c_str(), legacyPrefix.length(), legacyPrefix.data(), legacyPrefix.length()))
            {
                return uriAsString.begin() + legacyPrefix.length();
            }

            // ms-uup:///Product/id
            if (uri.Host().empty())
            {
                const auto path{ uri.Path() };
                const std::wstring_view c_product{ L"/Product/" };
                if ((path.size() > c_product.length()) &&
                    StringEqualsNoCase(path.c_str(), c_product.length(), c_product.data(), c_product.length()))
                {
                    return path.begin() + c_product.length();
                }
            }
        }
        return winrt::hstring{};
    }

    wil::unique_cotaskmem_array_ptr<wil::unique_cotaskmem_string> PackageDeploymentManager::GetPackageFullNamesFromUupProductUriIfMsUup(winrt::Windows::Foundation::Uri const& uri) const
    {
        wil::unique_cotaskmem_array_ptr<wil::unique_cotaskmem_string> packageFullNames;

        const auto schemeName{ uri.SchemeName() };
        const auto uupProductId{ GetUupProductIdIfMsUup(uri) };
        if (!uupProductId.empty())
        {
            THROW_IF_FAILED_MSG(Uup_SRFindPackageFullNamesByUupProductId(uupProductId.c_str(), packageFullNames.size_address<UINT32>(), packageFullNames.addressof()), "%s", uupProductId.c_str());
        }
        return packageFullNames;
    }

    bool PackageDeploymentManager::IsReadyByPackageFullName(hstring const& packageFullName)
    {
        const auto packageIdentity{ ::AppModel::Identity::PackageIdentity::FromPackageFullName(packageFullName.c_str()) };
        const winrt::hstring packageFamilyName{ packageIdentity.PackageFamilyName().c_str()};
        const AppModel::Identity::PackageVersion minVersion{ packageIdentity.Version() };
        const auto processorArchitectureFilter{ ToPackageDependencyProcessorArchitectures(packageIdentity.Architecture()) };
        return ::Microsoft::Windows::ApplicationModel::PackageDeploymentResolver::FindAny(m_packageManager, packageFamilyName, minVersion, processorArchitectureFilter);
    }

    bool PackageDeploymentManager::IsReady(winrt::Microsoft::Windows::Management::Deployment::PackageSetItem const& packageSetItem)
    {
        const AppModel::Identity::PackageVersion minVersion{ packageSetItem.MinVersion() };
        const auto processorArchitectureFilter{ packageSetItem.ProcessorArchitectureFilter() };
        return ::Microsoft::Windows::ApplicationModel::PackageDeploymentResolver::FindAny(m_packageManager, packageSetItem.PackageFamilyName(), minVersion, processorArchitectureFilter);
    }

    winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus PackageDeploymentManager::IsReadyOrNewerAvailableByPackageFullName(hstring const& packageFullName)
    {
        const auto packageIdentity{ ::AppModel::Identity::PackageIdentity::FromPackageFullName(packageFullName.c_str()) };
        const auto packageFamilyName{ packageIdentity.PackageFamilyName() };
        const auto minVersion{ packageIdentity.Version().Version };
        const auto architectureType{ ToArchitectureType(packageIdentity.Architecture()) };
        BOOL isRegistered{};
        BOOL isNewerAvailable{};
        THROW_IF_FAILED_MSG(PackageManagement_IsRegisteredOrNewerAvailable(nullptr, packageFamilyName.c_str(), minVersion, architectureType, &isRegistered, &isNewerAvailable), "%s", packageFullName.c_str());
        if (!isRegistered)
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NotReady;
        }
        else if (!isNewerAvailable)
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::Ready;
        }
        else
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NewerAvailable;
        }
    }

    winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus PackageDeploymentManager::IsReadyOrNewerAvailable(winrt::Microsoft::Windows::Management::Deployment::PackageSetItem const& packageSetItem)
    {
        const auto packageFamilyName{ packageSetItem.PackageFamilyName().c_str() };
        const AppModel::Identity::PackageVersion minVersion{ packageSetItem.MinVersion() };
        const auto architectureType{ ToArchitectureType(packageSetItem.ProcessorArchitectureFilter()) };
        BOOL isRegistered{};
        BOOL isNewerAvailable{};
        THROW_IF_FAILED_MSG(PackageManagement_IsRegisteredOrNewerAvailable(nullptr, packageFamilyName, minVersion.Version, architectureType, &isRegistered, &isNewerAvailable), "%ls", packageFamilyName);
        if (!isRegistered)
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NotReady;
        }
        else if (!isNewerAvailable)
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::Ready;
        }
        else
        {
            return winrt::Microsoft::Windows::Management::Deployment::PackageReadyOrNewerAvailableStatus::NewerAvailable;
        }
    }

    void PackageDeploymentManager::Validate(
        winrt::Microsoft::Windows::Management::Deployment::PackageSet const& packageSet) const
    {
        THROW_HR_IF(E_INVALIDARG, packageSet.Id().empty());
        const auto& packageSetItems{ packageSet.Items() };
        THROW_HR_IF(E_INVALIDARG, packageSetItems.Size() == 0);
        for (const winrt::Microsoft::Windows::Management::Deployment::PackageSetItem& packageSetItem : packageSetItems)
        {
            Validate(packageSet, packageSetItem);
        }
    }

    void PackageDeploymentManager::Validate(
        winrt::Microsoft::Windows::Management::Deployment::PackageSet const& packageSet,
        winrt::Microsoft::Windows::Management::Deployment::PackageSetItem const& packageSetItem) const
    {
        const auto packageFamilyName{ packageSetItem.PackageFamilyName() };
        THROW_IF_WIN32_ERROR_MSG(VerifyPackageFamilyName(packageFamilyName.c_str()), "PackageFamilyName:%ls", packageFamilyName.c_str());

        const auto packageUri{ GetEffectivePackageUri(packageSet, packageSetItem) };
        THROW_HR_IF_NULL_MSG(E_INVALIDARG, packageUri, "PackageUri:<null>");
    }

    HRESULT PackageDeploymentManager::EnsureReadyAsync(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Microsoft::Windows::Management::Deployment::PackageSetItem const& packageSetItem,
        winrt::Microsoft::Windows::Management::Deployment::EnsureReadyOptions const& options,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackageSetItem,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        if (IsReady(packageSetItem))
        {
            return S_OK;
        }

        winrt::Windows::Management::Deployment::AddPackageOptions addOptions{ ToOptions(options) };
        const auto progressBefore{ packageDeploymentProgress.Progress };
        auto deploymentOperation{ m_packageManager.AddPackageByUriAsync(packageUri, addOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const auto progressAfter{ progressBefore + PercentageToProgress(progressInfo.percentage, progressMaxPerPackageSetItem) };
            if (packageDeploymentProgress.Progress < progressAfter)
            {
                packageDeploymentProgress.Progress = progressAfter;
                progress(packageDeploymentProgress);
            }
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls : %ls",
                                     extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls : %ls",
                                 extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageUri.ToString().c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d %ls", static_cast<int32_t>(status), packageUri.ToString().c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageFamilyName:%ls PackageUri:%ls : %ls",
                         extendedError, packageSetItem.PackageFamilyName().c_str(), packageUri.ToString().c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::AddPackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions const& options,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackageSetItem,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        winrt::Windows::Management::Deployment::AddPackageOptions addOptions{ ToOptions(options) };
        RETURN_IF_FAILED(AddPackage(packageUri, addOptions, packageDeploymentProgress, progress, progressMaxPerPackageSetItem, extendedError, errorText, activityId));
        return S_OK;
    }

    HRESULT PackageDeploymentManager::AddPackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Windows::Management::Deployment::AddPackageOptions const& addOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackage,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        const auto progressBefore{ packageDeploymentProgress.Progress };
        auto deploymentOperation{ m_packageManager.AddPackageByUriAsync(packageUri, addOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const auto progressAfter{ progressBefore + PercentageToProgress(progressInfo.percentage, progressMaxPerPackage) };
            if (packageDeploymentProgress.Progress < progressAfter)
            {
                packageDeploymentProgress.Progress = progressAfter;
                progress(packageDeploymentProgress);
            }
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE))
                {
                    // Newer version already installed. Success!
                    (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE),
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());
                    extendedError = error;
                    errorText.clear();
                    return S_OK;
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                 extendedError, packageUri.ToString().c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageUri.ToString().c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageUri:%ls : %ls", static_cast<int32_t>(status), packageUri.ToString().c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageUri:%ls : %ls",
                         extendedError, packageUri.ToString().c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::StagePackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Microsoft::Windows::Management::Deployment::StagePackageOptions const& options,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackageSetItem,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        winrt::Windows::Management::Deployment::StagePackageOptions stageOptions{ ToOptions(options) };
        RETURN_IF_FAILED(StagePackage(packageUri, stageOptions, packageDeploymentProgress, progress, progressMaxPerPackageSetItem, extendedError, errorText, activityId));
        return S_OK;
    }

    HRESULT PackageDeploymentManager::StagePackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Windows::Management::Deployment::StagePackageOptions const& stageOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackage,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        const auto progressBefore{ packageDeploymentProgress.Progress };
        auto deploymentOperation{ m_packageManager.StagePackageByUriAsync(packageUri, stageOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const auto progressAfter{ progressBefore + PercentageToProgress(progressInfo.percentage, progressMaxPerPackage) };
            if (packageDeploymentProgress.Progress < progressAfter)
            {
                packageDeploymentProgress.Progress = progressAfter;
                progress(packageDeploymentProgress);
            }
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE))
                {
                    // Newer version already installed. Success!
                    (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE),
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());
                    extendedError = error;
                    errorText.clear();
                    return S_OK;
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                 extendedError, packageUri.ToString().c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageUri.ToString().c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageUri:%ls : %ls", static_cast<int32_t>(status), packageUri.ToString().c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageUri:%ls : %ls",
                         extendedError, packageUri.ToString().c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::RegisterPackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions const& options,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackageSetItem,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        winrt::Windows::Management::Deployment::RegisterPackageOptions registerOptions{ ToOptions(options) };
        RETURN_IF_FAILED(RegisterPackage(packageUri, registerOptions, packageDeploymentProgress, progress, progressMaxPerPackageSetItem, extendedError, errorText, activityId));
        return S_OK;
    }

    HRESULT PackageDeploymentManager::RegisterPackage(
        winrt::Windows::Foundation::Uri const& packageUri,
        winrt::Windows::Management::Deployment::RegisterPackageOptions const& registerOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        const double progressMaxPerPackage,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        const auto progressBefore{ packageDeploymentProgress.Progress };
        auto deploymentOperation{ m_packageManager.RegisterPackageByUriAsync(packageUri, registerOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const auto progressAfter{ progressBefore + PercentageToProgress(progressInfo.percentage, progressMaxPerPackage) };
            if (packageDeploymentProgress.Progress < progressAfter)
            {
                packageDeploymentProgress.Progress = progressAfter;
                progress(packageDeploymentProgress);
            }
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE))
                {
                    // Newer version already installed. Success!
                    (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE),
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());
                    extendedError = error;
                    errorText.clear();
                    return S_OK;
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                     extendedError, packageUri.ToString().c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageUri:%ls : %ls",
                                 extendedError, packageUri.ToString().c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageUri.ToString().c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageUri:%ls : %ls", static_cast<int32_t>(status), packageUri.ToString().c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageUri:%ls : %ls",
                         extendedError, packageUri.ToString().c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::RegisterPackageByPackageFamilyName(
        winrt::hstring const& packageFamilyName,
        winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions const& registerOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        const auto deploymentOptions{ ToDeploymentOptions(registerOptions) };
        auto deploymentOperation{ m_packageManager.RegisterPackageByFamilyNameAsync(packageFamilyName,
            registerOptions.DependencyPackageFamilyNames(), deploymentOptions,
            ToPackageVolume(registerOptions.AppDataVolume()), registerOptions.OptionalPackageFamilyNames()) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const double progressMaxPerPackageFamily{ 1.0 };
            packageDeploymentProgress.Progress = PercentageToProgress(progressInfo.percentage, progressMaxPerPackageFamily);
            progress(packageDeploymentProgress);
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE))
                {
                    // Newer version already installed. Success!
                    (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE),
                                     "ExtendedError:0x%08X PackageFamilyName:%ls : %ls",
                                     extendedError, packageFamilyName.c_str(), errorText.c_str());
                    extendedError = error;
                    errorText.clear();
                    return S_OK;
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageFamilyName:%ls : %ls",
                                     extendedError, packageFamilyName.c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageFamilyName:%ls : %ls",
                                 extendedError, packageFamilyName.c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageFamilyName.c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageFamilyName:%ls : %ls", static_cast<int32_t>(status), packageFamilyName.c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageFamilyName:%ls : %ls",
                         extendedError, packageFamilyName.c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::RegisterPackageByPackageFullName(
        winrt::hstring const& packageFullName,
        winrt::Windows::Management::Deployment::RegisterPackageOptions const& registerOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        winrt::Windows::Foundation::Collections::IVector<winrt::hstring> packageFullNames{ winrt::single_threaded_vector<winrt::hstring>() };
        packageFullNames.Append(packageFullName);
        auto deploymentOperation{ m_packageManager.RegisterPackagesByFullNameAsync(packageFullNames, registerOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const double progressMaxPerPackage{ 1.0 };
            packageDeploymentProgress.Progress = PercentageToProgress(progressInfo.percentage, progressMaxPerPackage);
            progress(packageDeploymentProgress);
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE))
                {
                    // Newer version already installed. Success!
                    (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE),
                                     "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                     extendedError, packageFullName.c_str(), errorText.c_str());
                    extendedError = error;
                    errorText.clear();
                    return S_OK;
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                     extendedError, packageFullName.c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                 extendedError, packageFullName.c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageFullName.c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageFullName:%ls : %ls", static_cast<int32_t>(status), packageFullName.c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                         extendedError, packageFullName.c_str(), errorText.c_str());
        }
        return S_OK;
    }

    HRESULT PackageDeploymentManager::RemovePackageByFullName(
        winrt::hstring const& packageFullName,
        winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions const& options,
        winrt::Windows::Management::Deployment::RemovalOptions const& removeOptions,
        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress& packageDeploymentProgress,
        wistd::function<void(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress)> progress,
        HRESULT& extendedError,
        winrt::hstring& errorText,
        winrt::guid& activityId)
    {
        extendedError = S_OK;
        errorText.clear();
        activityId = winrt::guid{};

        auto deploymentOperation{ m_packageManager.RemovePackageAsync(packageFullName, removeOptions) };
        deploymentOperation.Progress([&](winrt::Windows::Foundation::IAsyncOperationWithProgress<
                                            winrt::Windows::Management::Deployment::DeploymentResult,
                                            winrt::Windows::Management::Deployment::DeploymentProgress> const& /*sender*/,
                                         winrt::Windows::Management::Deployment::DeploymentProgress const& progressInfo)
        {
            const double progressMaxPerPackage{ 1.0 };
            packageDeploymentProgress.Progress = PercentageToProgress(progressInfo.percentage, progressMaxPerPackage);
            progress(packageDeploymentProgress);
        });
        deploymentOperation.get();
        try
        {
            const auto deploymentResult{ deploymentOperation.GetResults() };
            const HRESULT error{ static_cast<HRESULT>(deploymentOperation.ErrorCode()) };
            extendedError = deploymentResult.ExtendedErrorCode();
            errorText = deploymentResult.ErrorText();
            activityId = deploymentResult.ActivityId();
            const auto status{ deploymentOperation.Status() };
            if (status == winrt::Windows::Foundation::AsyncStatus::Error)
            {
                if (error == HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND))
                {
                    if (!options.FailIfNotFound())
                    {
                        // Package not installed. Success!
                        (void)LOG_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND),
                                         "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                         extendedError, packageFullName.c_str(), errorText.c_str());
                        extendedError = error;
                        errorText.clear();
                        return S_OK;
                    }
                }
                RETURN_IF_FAILED_MSG(error,
                                     "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                     extendedError, packageFullName.c_str(), errorText.c_str());

                // Status=Error but SUCCEEDED(error) == This.Should.Never.Happen.
                FAIL_FAST_HR_MSG(E_UNEXPECTED,
                                 "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                                 extendedError, packageFullName.c_str(), errorText.c_str());
            }
            else if (status == winrt::Windows::Foundation::AsyncStatus::Canceled)
            {
                RETURN_WIN32_MSG(ERROR_CANCELLED, "%ls", packageFullName.c_str());
            }
            FAIL_FAST_HR_IF_MSG(E_UNEXPECTED, status != winrt::Windows::Foundation::AsyncStatus::Completed,
                                "Status:%d PackageFullName:%ls : %ls", static_cast<int32_t>(status), packageFullName.c_str(), errorText.c_str());
        }
        catch (...)
        {
            auto exception{ hresult_error(to_hresult(), take_ownership_from_abi) };
            THROW_HR_MSG(exception.code(),
                         "ExtendedError:0x%08X PackageFullName:%ls : %ls",
                         extendedError, packageFullName.c_str(), errorText.c_str());
        }
        return S_OK;
    }

    winrt::Windows::Management::Deployment::PackageVolume PackageDeploymentManager::ToPackageVolume(winrt::Microsoft::Windows::Management::Deployment::PackageVolume const& packageVolume) const
    {
        if (packageVolume)
        {
            const auto name{ packageVolume.Name() };
            if (!name.empty())
            {
                auto toPackageVolume{ m_packageManager.FindPackageVolume(name) };
                return toPackageVolume;
            }
        }
        return nullptr;
    }

    winrt::Windows::Management::Deployment::AddPackageOptions PackageDeploymentManager::ToOptions(winrt::Microsoft::Windows::Management::Deployment::AddPackageOptions const& options) const
    {
        if (!options)
        {
            return winrt::Windows::Management::Deployment::AddPackageOptions();
        }

        winrt::Windows::Management::Deployment::AddPackageOptions toOptions;
        const auto targetVolume{ options.TargetVolume() };
        if (targetVolume)
        {
            const auto toPackageVolume{ ToPackageVolume(targetVolume) };
            if (toPackageVolume)
            {
                toOptions.TargetVolume(toPackageVolume);
            }
        }
        for (const auto uri : options.DependencyPackageUris())
        {
            toOptions.DependencyPackageUris().Append(uri);
        }
        for (const auto packageFamilyName : options.OptionalPackageFamilyNames())
        {
            toOptions.OptionalPackageFamilyNames().Append(packageFamilyName);
        }
        for (const auto uri : options.OptionalPackageUris())
        {
            toOptions.OptionalPackageUris().Append(uri);
        }
        for (const auto uri : options.RelatedPackageUris())
        {
            toOptions.RelatedPackageUris().Append(uri);
        }
        toOptions.ExternalLocationUri(options.ExternalLocationUri());
        toOptions.StubPackageOption(static_cast<winrt::Windows::Management::Deployment::StubPackageOption>(options.StubPackageOption()));
        toOptions.AllowUnsigned(options.AllowUnsigned());
        toOptions.DeveloperMode(options.DeveloperMode());
        toOptions.ForceAppShutdown(options.ForceAppShutdown());
        toOptions.ForceTargetAppShutdown(options.ForceTargetAppShutdown());
        toOptions.ForceUpdateFromAnyVersion(options.ForceUpdateFromAnyVersion());
        toOptions.InstallAllResources(options.InstallAllResources());
        toOptions.RequiredContentGroupOnly(options.RequiredContentGroupOnly());
        toOptions.RetainFilesOnFailure(options.RetainFilesOnFailure());
        toOptions.StageInPlace(options.StageInPlace());
        toOptions.DeferRegistrationWhenPackagesAreInUse(options.DeferRegistrationWhenPackagesAreInUse());
        if (options.IsExpectedDigestsSupported())
        {
            const auto expectedDigests{ options.ExpectedDigests() };
            if (expectedDigests)
            {
                auto toExpectedDigests{ toOptions.ExpectedDigests() };
                for (const auto expectedDigest : expectedDigests)
                {
                    toExpectedDigests.Insert(expectedDigest.Key(), expectedDigest.Value());
                }
            }
        }
        if (options.IsLimitToExistingPackagesSupported())
        {
            toOptions.LimitToExistingPackages(options.LimitToExistingPackages());
        }
        return toOptions;
    }

    winrt::Windows::Management::Deployment::StagePackageOptions PackageDeploymentManager::ToOptions(winrt::Microsoft::Windows::Management::Deployment::StagePackageOptions const& options) const
    {
        winrt::Windows::Management::Deployment::StagePackageOptions toOptions;
        const auto targetVolume{ options.TargetVolume() };
        if (targetVolume)
        {
            const auto toPackageVolume{ ToPackageVolume(targetVolume) };
            if (toPackageVolume)
            {
                toOptions.TargetVolume(toPackageVolume);
            }
        }
        for (const auto uri : options.DependencyPackageUris())
        {
            toOptions.DependencyPackageUris().Append(uri);
        }
        for (const auto packageFamilyName : options.OptionalPackageFamilyNames())
        {
            toOptions.OptionalPackageFamilyNames().Append(packageFamilyName);
        }
        for (const auto uri : options.OptionalPackageUris())
        {
            toOptions.OptionalPackageUris().Append(uri);
        }
        for (const auto uri : options.RelatedPackageUris())
        {
            toOptions.RelatedPackageUris().Append(uri);
        }
        toOptions.ExternalLocationUri(options.ExternalLocationUri());
        toOptions.StubPackageOption(static_cast<winrt::Windows::Management::Deployment::StubPackageOption>(options.StubPackageOption()));
        toOptions.DeveloperMode(options.DeveloperMode());
        toOptions.ForceUpdateFromAnyVersion(options.ForceUpdateFromAnyVersion());
        toOptions.InstallAllResources(options.InstallAllResources());
        toOptions.RequiredContentGroupOnly(options.RequiredContentGroupOnly());
        toOptions.StageInPlace(options.StageInPlace());
        toOptions.AllowUnsigned(options.AllowUnsigned());
        if (options.IsExpectedDigestsSupported())
        {
            const auto expectedDigests{ options.ExpectedDigests() };
            if (expectedDigests)
            {
                auto toExpectedDigests{ toOptions.ExpectedDigests() };
                for (const auto expectedDigest : expectedDigests)
                {
                    toExpectedDigests.Insert(expectedDigest.Key(), expectedDigest.Value());
                }
            }
        }
        return toOptions;
    }

    winrt::Windows::Management::Deployment::RegisterPackageOptions PackageDeploymentManager::ToOptions(winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions const& options) const
    {
        winrt::Windows::Management::Deployment::RegisterPackageOptions toOptions;
        const auto appDataVolume{ options.AppDataVolume() };
        if (appDataVolume)
        {
            const auto toPackageVolume{ ToPackageVolume(appDataVolume) };
            if (toPackageVolume)
            {
                toOptions.AppDataVolume(toPackageVolume);
            }
        }
        for (const auto uri : options.DependencyPackageUris())
        {
            toOptions.DependencyPackageUris().Append(uri);
        }
        for (const auto packageFamilyName : options.OptionalPackageFamilyNames())
        {
            toOptions.OptionalPackageFamilyNames().Append(packageFamilyName);
        }
        toOptions.ExternalLocationUri(options.ExternalLocationUri());
        toOptions.DeveloperMode(options.DeveloperMode());
        toOptions.ForceAppShutdown(options.ForceAppShutdown());
        toOptions.ForceTargetAppShutdown(options.ForceTargetAppShutdown());
        toOptions.ForceUpdateFromAnyVersion(options.ForceUpdateFromAnyVersion());
        toOptions.InstallAllResources(options.InstallAllResources());
        toOptions.StageInPlace(options.StageInPlace());
        toOptions.AllowUnsigned(options.AllowUnsigned());
        toOptions.DeferRegistrationWhenPackagesAreInUse(options.DeferRegistrationWhenPackagesAreInUse());
        if (options.IsExpectedDigestsSupported())
        {
            const auto expectedDigests{ options.ExpectedDigests() };
            if (expectedDigests)
            {
                auto toExpectedDigests{ toOptions.ExpectedDigests() };
                for (const auto expectedDigest : expectedDigests)
                {
                    toExpectedDigests.Insert(expectedDigest.Key(), expectedDigest.Value());
                }
            }
        }
        return toOptions;
    }

    winrt::Windows::Management::Deployment::RemovalOptions PackageDeploymentManager::ToOptions(winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions const& options) const
    {
        auto toOptions{ winrt::Windows::Management::Deployment::RemovalOptions::None };
        if (options.PreserveApplicationData())
        {
            toOptions |= winrt::Windows::Management::Deployment::RemovalOptions::PreserveApplicationData;
        }
        if (options.PreserveRoamableApplicationData())
        {
            toOptions |= winrt::Windows::Management::Deployment::RemovalOptions::PreserveRoamableApplicationData;
        }
        if (options.RemoveForAllUsers())
        {
            toOptions |= winrt::Windows::Management::Deployment::RemovalOptions::RemoveForAllUsers;
        }
        return toOptions;
    }

    winrt::Windows::Management::Deployment::AddPackageOptions PackageDeploymentManager::ToOptions(winrt::Microsoft::Windows::Management::Deployment::EnsureReadyOptions const& /*options*/) const
    {
        return winrt::Windows::Management::Deployment::AddPackageOptions();
    }

    winrt::Windows::Management::Deployment::DeploymentOptions PackageDeploymentManager::ToDeploymentOptions(winrt::Microsoft::Windows::Management::Deployment::RegisterPackageOptions const& options) const
    {
        auto deploymentOptions{ winrt::Windows::Management::Deployment::DeploymentOptions::None };
        if (options.DeveloperMode())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::DevelopmentMode;
        }
        if (options.ForceAppShutdown())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::ForceApplicationShutdown;
        }
        if (options.ForceTargetAppShutdown())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::ForceTargetApplicationShutdown;
        }
        if (options.ForceUpdateFromAnyVersion())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::ForceUpdateFromAnyVersion;
        }
        if (options.InstallAllResources())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::InstallAllResources;
        }
        if (options.StageInPlace())
        {
            deploymentOptions |= winrt::Windows::Management::Deployment::DeploymentOptions::StageInPlace;
        }
        return deploymentOptions;
    }

    double PackageDeploymentManager::PercentageToProgress(uint32_t percentage, const double progressMaxPerItem)
    {
        return (static_cast<double>(percentage) / 100.0) * progressMaxPerItem;
    }

    bool PackageDeploymentManager::IsUriEndsWith(winrt::Windows::Foundation::Uri const& packageUri, PCWSTR target)
    {
        FAIL_FAST_HR_IF(E_UNEXPECTED, !target);
        FAIL_FAST_HR_IF(E_UNEXPECTED, *target == L'\0');

        if (!packageUri)
        {
            return false;
        }
        const auto targetLength{ wcslen(target) };
        const auto path{ packageUri.Path() };
        if (path.size() < targetLength)
        {
            return false;
        }
        PCWSTR pathToCompare{ path.c_str() + (path.size() - targetLength) };
        return CompareStringOrdinal(pathToCompare, -1, target, -1, TRUE) == CSTR_EQUAL;
    }

    winrt::Windows::Foundation::Uri PackageDeploymentManager::GetEffectivePackageUri(
        winrt::Microsoft::Windows::Management::Deployment::PackageSet const& packageSet,
        winrt::Microsoft::Windows::Management::Deployment::PackageSetItem const& packageSetItem)
    {
        auto packageUri{ packageSetItem.PackageUri() };
        if (packageUri)
        {
            return packageUri;
        }
        else
        {
            return packageSet.PackageUri();
        }
    }
}
