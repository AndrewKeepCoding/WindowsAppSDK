// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"

#include "PackageDeploymentManagerTests.h"

#include <IsWindowsVersion.h>

namespace TD = ::Test::Diagnostics;
namespace TB = ::Test::Bootstrap;
namespace TP = ::Test::Packages;
namespace TPF = ::Test::Packages::Framework;
namespace TPM = ::Test::Packages::Main;
namespace TPMT = ::Test::PackageManager::Tests;

namespace Test::PackageManager::Tests
{
    class PackageDeploymentManagerTests_Remove : PackageDeploymentManagerTests_Base
    {
    public:
        BEGIN_TEST_CLASS(PackageDeploymentManagerTests_Remove)
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        {
            if (!::WindowsVersion::IsWindows10_20H1OrGreater())
            {
                WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"PackageDeploymentManager requires >= 20H1 (Vibranium). Skipping tests");
                return true;
            }
            RemovePackage_Blue();
            RemovePackage_Green();
            RemovePackage_Redder();
            RemovePackage_Red();
            ::TB::Setup();
            return true;
        }

        TEST_CLASS_CLEANUP(ClassCleanup)
        {
            RemovePackage_Blue();
            RemovePackage_Green();
            RemovePackage_Redder();
            RemovePackage_Red();
            ::TB::Cleanup();
            return true;
        }

        TEST_METHOD(RemovePackageAsync_PackageFullName_NoSuchPackage_Fail)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            PCWSTR packageFullName{ L"Does.Not.Exist_0.0.0.0_neutral__1234567890abc" };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentResult{ packageDeploymentManager.RemovePackageAsync(packageFullName, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));
        }

        TEST_METHOD(RemovePackageAsync_PackageFullName_NotInstalled_Fail)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFullName_NotInstalled_Success)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFullName_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            StagePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFullName_Registered_Success)
        {
            AddPackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFamilyName_NoSuchPackage_Success)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            PCWSTR packageFamilyName{ L"Does.Not.Exist_1234567890abc" };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());   // Ignored matter for PackageFamilyName
            auto deploymentResult{ packageDeploymentManager.RemovePackageAsync(packageFamilyName, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFamilyName_NotInstalled_Success)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFamilyName_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            StagePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageAsync_PackageFamilyName_Registered_Success)
        {
            AddPackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFullNameAsync_NoSuchPackage_Fail)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            PCWSTR packageFullName{ L"Does.Not.Exist_0.0.0.0_neutral__1234567890abc" };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentResult{ packageDeploymentManager.RemovePackageByFullNameAsync(packageFullName, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));
        }

        TEST_METHOD(RemovePackageByFullNameAsync_NotInstalled_Fail)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFullNameAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFullNameAsync_NotInstalled_Success)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFullNameAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFullNameAsync_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            StagePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFullNameAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFullNameAsync_Registered_Success)
        {
            AddPackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFullName{ ::TPF::Red::GetPackageFullName() };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFullNameAsync(packageFullName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFamilyNameAsync_NoSuchPackage_Success)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            PCWSTR packageFamilyName{ L"Does.Not.Exist_1234567890abc" };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());   // Ignored matter for PackageFamilyName
            auto deploymentResult{ packageDeploymentManager.RemovePackageByFamilyNameAsync(packageFamilyName, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));
        }

        TEST_METHOD(RemovePackageByFamilyNameAsync_NotInstalled_Success)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFamilyNameAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFamilyNameAsync_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            StagePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFamilyNameAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByFamilyNameAsync_Registered_Success)
        {
            AddPackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            const winrt::hstring packageFamilyName{ ::TPF::Red::c_packageFamilyName };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageByFamilyNameAsync(packageFamilyName, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageByUriAsync_NoSuchPackage_Fail)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            PCWSTR c_packageUriAsString{ L"ms-uup://Product/does.not.exist" };
            winrt::Windows::Foundation::Uri packageUri{ c_packageUriAsString };

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentResult{ packageDeploymentManager.RemovePackageByUriAsync(packageUri, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));
        }

        TEST_METHOD(RemovePackageByUriAsync_NotInstalled_Fail)
        {
            WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Cannot test ms-uup: URIs here. Skipping test");
        }

        TEST_METHOD(RemovePackageByUriAsync_NotInstalled_Success)
        {
            WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Cannot test ms-uup: URIs here. Skipping test");
        }

        TEST_METHOD(RemovePackageByUriAsync_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Cannot test ms-uup: URIs here. Skipping test");
        }

        TEST_METHOD(RemovePackageByUriAsync_Registered_Success)
        {
            WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped, L"Cannot test ms-uup: URIs here. Skipping test");
        }

        TEST_METHOD(RemovePackageSetAsync_1_NoSuchPackage_Fail)
        {
            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"Does.Not.Exist" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem packageSetItem;
            PCWSTR c_packageFamilyName{ L"Does.Not.Exist_1234567890abc" };
            packageSetItem.PackageFamilyName(c_packageFamilyName);
            PCWSTR c_packageUriAsString{ L"file://c:/does/not/exist.msix" };
            winrt::Windows::Foundation::Uri packageUri{ c_packageUriAsString };
            packageSetItem.PackageUri(packageUri);
            packageSet.Items().Append(packageSetItem);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentResult{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options).get() };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedFailure, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_1_NotInstalled_Fail)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageSetAsync_1_NotInstalled_Success)
        {
            RemovePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_1_Staged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            StagePackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_1_Registered_Success)
        {
            AddPackage_Red();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_N_NotInstalled_Fail)
        {
            RemovePackage_Red();
            RemovePackage_Green();
            RemovePackage_Blue();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem green{ Make_PackageSetItem(::TPF::Green::GetPackageFullName(), ::TPF::Green::c_packageDirName) };
            packageSet.Items().Append(green);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem blue{ Make_PackageSetItem(::TPF::Blue::GetPackageFullName(), ::TPF::Blue::c_packageDirName) };
            packageSet.Items().Append(blue);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            VERIFY_IS_TRUE(options.FailIfNotFound());
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_NOT_FOUND), deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_FALSE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(IsPackageRegistered_Red());
        }

        TEST_METHOD(RemovePackageSetAsync_N_NotInstalled_Success)
        {
            RemovePackage_Red();
            RemovePackage_Green();
            RemovePackage_Blue();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem green{ Make_PackageSetItem(::TPF::Green::GetPackageFullName(), ::TPF::Green::c_packageDirName) };
            packageSet.Items().Append(green);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem blue{ Make_PackageSetItem(::TPF::Blue::GetPackageFullName(), ::TPF::Blue::c_packageDirName) };
            packageSet.Items().Append(blue);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            options.FailIfNotFound(false);
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_N_Registered_Success)
        {
            AddPackage_Red();
            AddPackage_Green();
            AddPackage_Blue();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem green{ Make_PackageSetItem(::TPF::Green::GetPackageFullName(), ::TPF::Green::c_packageDirName) };
            packageSet.Items().Append(green);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem blue{ Make_PackageSetItem(::TPF::Blue::GetPackageFullName(), ::TPF::Blue::c_packageDirName) };
            packageSet.Items().Append(blue);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_N_RegisteredAndNotInstalled_Success)
        {
            AddPackage_Red();
            RemovePackage_Green();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem green{ Make_PackageSetItem(::TPF::Green::GetPackageFullName(), ::TPF::Green::c_packageDirName) };
            packageSet.Items().Append(green);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }

        TEST_METHOD(RemovePackageSetAsync_N_RegisteredAndNotInstalledAndStaged_Success)
        {
            BEGIN_TEST_METHOD_PROPERTIES()
                TEST_CLASS_PROPERTY(L"RunAs", L"ElevatedUser")
            END_TEST_METHOD_PROPERTIES()

            AddPackage_Red();
            RemovePackage_Green();
            StagePackage_Blue();

            auto packageDeploymentManager{ winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentManager::GetDefault() };

            winrt::Microsoft::Windows::Management::Deployment::PackageSet packageSet;
            PCWSTR c_packageSetId{ L"RGB" };
            packageSet.Id(c_packageSetId);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem red{ Make_PackageSetItem(::TPF::Red::GetPackageFullName(), ::TPF::Red::c_packageDirName) };
            packageSet.Items().Append(red);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem green{ Make_PackageSetItem(::TPF::Green::GetPackageFullName(), ::TPF::Green::c_packageDirName) };
            packageSet.Items().Append(green);
            winrt::Microsoft::Windows::Management::Deployment::PackageSetItem blue{ Make_PackageSetItem(::TPF::Blue::GetPackageFullName(), ::TPF::Blue::c_packageDirName) };
            packageSet.Items().Append(blue);

            winrt::Microsoft::Windows::Management::Deployment::RemovePackageOptions options;
            auto deploymentOperation{ packageDeploymentManager.RemovePackageSetAsync(packageSet, options) };
            auto deploymentResult{ WaitForDeploymentOperation(deploymentOperation) };
            VERIFY_ARE_EQUAL(winrt::Microsoft::Windows::Management::Deployment::PackageDeploymentStatus::CompletedSuccess, deploymentResult.Status());
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.Error(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_ARE_EQUAL(S_OK, deploymentResult.ExtendedError(), WEX::Common::String().Format(L"0x%X", deploymentResult.ExtendedError()));
            VERIFY_IS_TRUE(deploymentResult.ErrorText().empty(), WEX::Common::String().Format(L"%s", deploymentResult.ErrorText().c_str()));

            VERIFY_IS_FALSE(packageDeploymentManager.IsPackageSetReady(packageSet));
        }
    };
}
