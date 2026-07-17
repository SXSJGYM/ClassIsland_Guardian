/*
 * process.sys - Process Protection Driver for ClassIsland Guardian
 * Copyright (C) 2026 GYM_Latest
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Based on Microsoft ObCallback sample.
 */

#include <ntddk.h>
#include <ntstrsafe.h>

#define CB_PROCESS_TERMINATE 0x0001
#define NAME_SIZE            256
#define PROTECT_COUNT        2

 //
 // Global variables
 //
KGUARDED_MUTEX g_CallbacksMutex;
BOOLEAN        g_CallbacksInstalled = FALSE;
PVOID          g_RegistrationHandle = NULL;

OB_CALLBACK_REGISTRATION  g_ObRegistration = { 0 };
OB_OPERATION_REGISTRATION g_OpRegistrations[1] = { { 0 } };
UNICODE_STRING            g_Altitude = { 0 };

// Hard-coded process names to protect
WCHAR g_ProtectNames[PROTECT_COUNT][NAME_SIZE] = {
    L"guardian.exe",
    L"ClassIsland.Desktop.exe"
};

// Runtime captured protected process info
PVOID   g_ProtectedProcess = NULL;
HANDLE  g_ProtectedProcessId = NULL;

//
// Function prototypes
//
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

VOID DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
);

VOID ProcessCreateNotify(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
);

OB_PREOP_CALLBACK_STATUS
ProcessPreOperation(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
);

//
// Driver entry point
//
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("[process.sys] DriverEntry\n");

    KeInitializeGuardedMutex(&g_CallbacksMutex);

    status = PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, FALSE);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[process.sys] PsSetCreateProcessNotifyRoutineEx failed: 0x%X\n", status);
        return status;
    }

    g_OpRegistrations[0].ObjectType = PsProcessType;
    g_OpRegistrations[0].Operations |= OB_OPERATION_HANDLE_CREATE;
    g_OpRegistrations[0].Operations |= OB_OPERATION_HANDLE_DUPLICATE;
    g_OpRegistrations[0].PreOperation = ProcessPreOperation;
    g_OpRegistrations[0].PostOperation = NULL;

    RtlInitUnicodeString(&g_Altitude, L"328000.1");

    g_ObRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    g_ObRegistration.OperationRegistrationCount = 1;
    g_ObRegistration.Altitude = g_Altitude;
    g_ObRegistration.RegistrationContext = NULL;
    g_ObRegistration.OperationRegistration = g_OpRegistrations;

    status = ObRegisterCallbacks(&g_ObRegistration, &g_RegistrationHandle);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[process.sys] ObRegisterCallbacks failed: 0x%X\n", status);
        PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, TRUE);
        return status;
    }
    g_CallbacksInstalled = TRUE;

    DriverObject->DriverUnload = DriverUnload;

    DbgPrint("[process.sys] Driver initialized successfully.\n");
    return STATUS_SUCCESS;
}

//
// Driver unload
//
VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);

    DbgPrint("[process.sys] DriverUnload\n");

    PsSetCreateProcessNotifyRoutineEx(ProcessCreateNotify, TRUE);

    if (g_CallbacksInstalled) {
        ObUnRegisterCallbacks(g_RegistrationHandle);
        g_RegistrationHandle = NULL;
        g_CallbacksInstalled = FALSE;
    }
}

//
// Process creation notification callback
//
VOID
ProcessCreateNotify(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    int i;
    UNREFERENCED_PARAMETER(Process);

    if (CreateInfo == NULL) {
        if (ProcessId == g_ProtectedProcessId) {
            KeAcquireGuardedMutex(&g_CallbacksMutex);
            g_ProtectedProcess = NULL;
            g_ProtectedProcessId = NULL;
            KeReleaseGuardedMutex(&g_CallbacksMutex);
            DbgPrint("[process.sys] Protected process terminated (PID: %p)\n", (PVOID)ProcessId);
        }
        return;
    }

    if (CreateInfo->CommandLine != NULL && CreateInfo->CommandLine->Buffer != NULL) {
        for (i = 0; i < PROTECT_COUNT; i++) {
            if (wcsstr(CreateInfo->CommandLine->Buffer, g_ProtectNames[i]) != NULL) {
                KeAcquireGuardedMutex(&g_CallbacksMutex);
                g_ProtectedProcess = Process;
                g_ProtectedProcessId = ProcessId;
                KeReleaseGuardedMutex(&g_CallbacksMutex);

                DbgPrint("[process.sys] Protected: %ws (PID: %p)\n",
                    g_ProtectNames[i], (PVOID)ProcessId);
                break;
            }
        }
    }
}

//
// ObRegisterCallbacks pre-operation callback
//
OB_PREOP_CALLBACK_STATUS
ProcessPreOperation(
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
)
{
    PACCESS_MASK DesiredAccess = NULL;
    UNREFERENCED_PARAMETER(RegistrationContext);

    if (PreInfo->ObjectType != *PsProcessType) {
        return OB_PREOP_SUCCESS;
    }

    if (PreInfo->Object != g_ProtectedProcess) {
        return OB_PREOP_SUCCESS;
    }

    if (PreInfo->KernelHandle == 1) {
        return OB_PREOP_SUCCESS;
    }

    if (PreInfo->Object == PsGetCurrentProcess()) {
        return OB_PREOP_SUCCESS;
    }

    switch (PreInfo->Operation) {
    case OB_OPERATION_HANDLE_CREATE:
        DesiredAccess = &PreInfo->Parameters->CreateHandleInformation.DesiredAccess;
        break;
    case OB_OPERATION_HANDLE_DUPLICATE:
        DesiredAccess = &PreInfo->Parameters->DuplicateHandleInformation.DesiredAccess;
        break;
    default:
        return OB_PREOP_SUCCESS;
    }

    if (DesiredAccess) {
        *DesiredAccess &= ~CB_PROCESS_TERMINATE;
    }

    DbgPrint("[process.sys] Blocked termination on PID: %p\n", (PVOID)g_ProtectedProcessId);

    return OB_PREOP_SUCCESS;
}