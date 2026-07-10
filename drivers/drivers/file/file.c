#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

typedef struct _CIG_FILTER_DATA {

    PFLT_FILTER FilterHandle;

} CIGFile_FILTER_DATA, * PNULL_FILTER_DATA;

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
CIGFileUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
CIGFileQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

CIGFile_FILTER_DATA NullFilterData;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE,   CIGFileUnload)
#pragma alloc_text(PAGE, CIGFileQueryTeardown)
#endif

// 添加回调函数
FLT_PREOP_CALLBACK_STATUS
CIGPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
);

// 定义操作回调表-全部拦截
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,           0, CIGPreCreate, NULL },  // 打开/创建
    { IRP_MJ_READ,             0, CIGPreCreate, NULL },  // 读取
    { IRP_MJ_WRITE,            0, CIGPreCreate, NULL },  // 写入
    { IRP_MJ_SET_INFORMATION,  0, CIGPreCreate, NULL },  // 删除/重命名/改属性
    { IRP_MJ_DIRECTORY_CONTROL,0, CIGPreCreate, NULL },  // 遍历目录（列举文件）
    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                               //  Operation callbacks

    CIGFileUnload,                         //  FilterUnload

    NULL,                               //  InstanceSetup
    CIGFileQueryTeardown,                  //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    status = FltRegisterFilter(DriverObject,
        &FilterRegistration,
        &NullFilterData.FilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)) {
        status = FltStartFiltering(NullFilterData.FilterHandle);

        if (!NT_SUCCESS(status)) {
            FltUnregisterFilter(NullFilterData.FilterHandle);
        }
    }

    return status;
}

NTSTATUS
CIGFileUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    FltUnregisterFilter(NullFilterData.FilterHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
CIGFileQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
CIGPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
)
{
    *CompletionContext = NULL;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;

    status = FltGetFileNameInformation(Data,
        FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
        &nameInfo);
    if (!NT_SUCCESS(status)) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 在 CIGPreCreate 中，获取 nameInfo 之后、目录拦截之前
    PUNICODE_STRING processPath = NULL;
    BOOLEAN isTrusted = FALSE;

    if (NT_SUCCESS(SeLocateProcessImageName(PsGetCurrentProcess(), &processPath)) && processPath != NULL) {
        // 精确匹配完整路径（注意路径格式可能是 \??\C:\... 或 \Device\HarddiskVolume...）
        if (wcsstr(processPath->Buffer, L"\\ClassIslandGuardian\\config.exe") != NULL ||
            wcsstr(processPath->Buffer, L"\\ClassIslandGuardian\\guardian.exe") != NULL) {
            isTrusted = TRUE;
        }
        ExFreePool(processPath);
        processPath = NULL;
    }

    // 如果是可信进程，直接放行（绕过所有目录检查）
    if (isTrusted) {
        FltReleaseFileNameInformation(nameInfo);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 空指针保护
    if (nameInfo->Name.Buffer == NULL) {
        FltReleaseFileNameInformation(nameInfo);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // === 目录保护策略 ===
    // 1. GuardianRecovery：完全封锁
    if (wcsstr(nameInfo->Name.Buffer, L"\\GuardianRecovery") != NULL) {
        DbgPrint("[CIG] BLOCKED: Access to GuardianRecovery\n");
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        FltReleaseFileNameInformation(nameInfo);
        return FLT_PREOP_COMPLETE;
    }

    // 2. Guardian 目录：读放行，写拦截
    if (wcsstr(nameInfo->Name.Buffer, L"\\Program Files\\ClassIslandGuardian") != NULL) {
        ULONG createDisposition = Data->Iopb->Parameters.Create.Options >> 24;
        ACCESS_MASK desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;

        BOOLEAN isWrite = (createDisposition != FILE_OPEN) ||
            (desiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_DELETE_CHILD));

        if (isWrite) {
            DbgPrint("[CIG] BLOCKED: Write access to Guardian directory\n");
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;
            FltReleaseFileNameInformation(nameInfo);
            return FLT_PREOP_COMPLETE;
        }
        // 读取放行
        DbgPrint("[CIG] ALLOWED: Read access to Guardian directory\n");
        FltReleaseFileNameInformation(nameInfo);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 其他路径放行
    FltReleaseFileNameInformation(nameInfo);
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
