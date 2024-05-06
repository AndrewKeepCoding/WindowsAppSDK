// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

namespace TD = ::Test::Diagnostics;
namespace TB = ::Test::Bootstrap;
namespace TP = ::Test::Packages;
namespace TPF = ::Test::Packages::Framework;
namespace TPM = ::Test::Packages::Main;
namespace TPMT = ::Test::PackageManager::Tests;

namespace Test::PackageManager::Tests
{
    inline bool SkipIfFeatureNotSupported(
        const winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature feature,
        PCWSTR message)
    {
        const bool isSupported{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::IsPackageDeploymentFeatureSupported(feature) };
        if (!isSupported)
        {
            WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, message);
        }
        return !isSupported;
    }

    inline bool SkipIfFeatureNotSupported_PackageUriScheme_ms_uup()
    {
        const auto feature{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::PackageUriScheme_ms_uup };
        PCWSTR message{ L"ms-uup: not supported on this system. Skipping test" };
        return TPMT::SkipIfFeatureNotSupported(feature, message);
    }

    inline bool SkipIfFeatureNotSupported_IsPackageReadyOrNewerAvailable()
    {
        const auto feature{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::IsPackageReadyOrNewerAvailable };
        PCWSTR message{ L"IsPackageReadyOrNewerAvailable not supported on this system. Skipping test" };
        return TPMT::SkipIfFeatureNotSupported(feature, message);
    }

    inline bool SkipIfFeatureNotSupported_RemovePackageByUri()
    {
        const auto feature{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RemovePackageByUri };
        PCWSTR message{ L"RemovePackageByUri not supported on this system. Skipping test" };
        return TPMT::SkipIfFeatureNotSupported(feature, message);
    }

    inline bool SkipIfFeatureNotSupported_ResetPackage()
    {
        const auto feature{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::ResetPackage };
        PCWSTR message{ L"ResetPackage not supported on this system. Skipping test" };
        return TPMT::SkipIfFeatureNotSupported(feature, message);
    }

    inline bool SkipIfFeatureNotSupported_RepairPackage()
    {
        const auto feature{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentFeature::RepairPackage };
        PCWSTR message{ L"RepairPackage not supported on this system. Skipping test" };
        return TPMT::SkipIfFeatureNotSupported(feature, message);
    }

    class PackageDeploymentManagerTests_Base
    {
    protected:
        winrt::Windows::ApplicationModel::PackageStatus GetPackageStatus(PCWSTR packageFullName)
        {
            return TPMT::GetPackageStatus(m_packageManager, packageFullName);
        }

        void SetPackageStatus(PCWSTR packageFullName, winrt::Windows::Management::Deployment::PackageStatus status)
        {
            TPMT::SetPackageStatus(m_packageManager, packageFullName, status);
        }

        void SetPackageStatusByPackageFamilyName(PCWSTR packageFamilyName, winrt::Windows::Management::Deployment::PackageStatus status)
        {
            TPMT::SetPackageStatusByPackageFamilyName(m_packageManager, packageFamilyName, status);
        }

        void ClearPackageStatus(PCWSTR packageFullName, winrt::Windows::Management::Deployment::PackageStatus status)
        {
            TPMT::ClearPackageStatus(m_packageManager, packageFullName, status);
        }

        void ClearPackageStatusByPackageFamilyName(PCWSTR packageFamilyName, winrt::Windows::Management::Deployment::PackageStatus status)
        {
            TPMT::ClearPackageStatusByPackageFamilyName(m_packageManager, packageFamilyName, status);
        }

        winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult WaitForDeploymentOperation(
            winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentResult, winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentProgress>& deploymentOperation)
        {
            using namespace winrt::Windows::Foundation;
            using namespace winrt::Microsoft::Windows::Management::Deployment;
            AsyncOperationProgressHandler<PackageDeploymentResult, PackageDeploymentProgress> progressCallback(
                [&](const IAsyncOperationWithProgress<PackageDeploymentResult, PackageDeploymentProgress>&, PackageDeploymentProgress progress)
            {
                    WEX::Logging::Log::Comment(WEX::Common::String().Format(L"...State:%d Percentage:%lf", static_cast<int>(progress.Status), progress.Progress));
                }
            );
            deploymentOperation.Progress(progressCallback);
            auto deploymentResult{ deploymentOperation.get() };
            return deploymentResult;
        }

    protected:
        winrt::Windows::Management::Deployment::PackageManager m_packageManager;
    };
}
