#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "versionhelpers.h"

#pragma comment(lib, "dismapi.lib")
#include "DismAPI.h"

void DismProgressCallback(UINT Current, UINT Total, PVOID UserData)
{
    _tprintf(TEXT("%d of %d\n"), Current, Total);
}


HRESULT EnableFeatureIfNotInstalled(DismSession session, PCWSTR featureName)
{
    HRESULT hResult = S_OK;
    DismFeatureInfo* featureInfo = NULL;
    void* userData = NULL;

    if (session == NULL ||
        featureName == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    hResult = DismGetFeatureInfo(session, featureName, NULL, DismPackageNone, &featureInfo);

    if (FAILED(hResult) ||
        NULL == featureInfo)
        goto Cleanup;

    if (DismStateInstalled != featureInfo->FeatureState)
    {

        _tprintf(TEXT("Enabling %s...\n"), featureName);
        hResult = DismEnableFeature(session, featureName, NULL, DismPackageNone, TRUE, NULL, 0, TRUE, NULL, NULL, NULL);
        if (FAILED(hResult))
        {
            _tprintf(TEXT("Failed to enable %s.\n"), featureName);
        } else if (HRESULT_FROM_WIN32(ERROR_SUCCESS_REBOOT_REQUIRED) == hResult)
        {
            _tprintf(TEXT("%s enabled successfully, reboot required.\n"), featureName);
        }
    }
    else
    {
        _tprintf(TEXT("%s already enabled.\n"), featureName);
    }

Cleanup:
    DismDelete(featureInfo);

    return hResult;
}


bool Win1809OrGreater()
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    DWORDLONG dwlConditionMask =
        VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(
                    VerSetConditionMask(
                        VerSetConditionMask(0,
                            VER_MAJORVERSION, VER_GREATER_EQUAL),
                        VER_MINORVERSION, VER_GREATER_EQUAL),
                    VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL),
                VER_SERVICEPACKMINOR, VER_GREATER_EQUAL),
            VER_BUILDNUMBER, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = 10;
    osvi.dwMinorVersion = 0;
    osvi.wServicePackMajor = 0;
    osvi.wServicePackMinor = 0;
    osvi.dwBuildNumber = 17763;

    return !VerifyVersionInfo(&osvi,
        VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR | VER_BUILDNUMBER,
        dwlConditionMask);
 }

int main()
{
    HRESULT hr = S_OK;
    DismSession session = DISM_SESSION_DEFAULT;

    if (!Win1809OrGreater())
    {
        _tprintf(TEXT("1809+ required\n"));
        return ERROR_VERSION_PARSE_ERROR;
    }

    hr = DismInitialize(DismLogErrorsWarningsInfo, NULL, NULL);

    if (S_OK != hr) {
        switch (HRESULT_FROM_WIN32(hr)) {
            case ERROR_ELEVATION_REQUIRED:
                break;
        }
    }

    if (HRESULT_FROM_WIN32(ERROR_ELEVATION_REQUIRED) == hr)
    {
        _tprintf(TEXT("ERROR_ELEVATION_REQUIRED\n"));
    }

    hr = DismOpenSession(DISM_ONLINE_IMAGE, NULL, NULL, &session);
    if (FAILED(hr))
    {
        _tprintf(TEXT("DismOpenSession Failed: %x\n"), hr);
        goto Cleanup;
    }

    hr = EnableFeatureIfNotInstalled(session, L"Client-ProjFS");
    
    _tprintf(TEXT("%d"), hr);

Cleanup:
    hr = DismCloseSession(session);
    if (FAILED(hr))
    {
        _tprintf(TEXT("DismCloseSession Failed: %x\n"), hr);
    }

    hr = DismShutdown();
    return 0;
}
