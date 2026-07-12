#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

typedef struct _CIGFile_FILTER_DATA {

    PFLT_FILTER FilterHandle;

} CIGFile_FILTER_DATA, * PCIGFILE_FILTER_DATA;

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

CIGFile_FILTER_DATA CIGFileFilterData;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE,   CIGFileUnload)
#pragma alloc_text(PAGE, CIGFileQueryTeardown)
#endif

// 添加写入回调函数
FLT_PREOP_CALLBACK_STATUS
CIGPreWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
);

// 添加读取回调函数
FLT_PREOP_CALLBACK_STATUS
CIGPreRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
);


// 定义操作回调表-全部拦截
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE, 0, CIGPreWrite, NULL },  // 打开/创建
    { IRP_MJ_READ, 0, CIGPreRead, NULL },  // 读取
    { IRP_MJ_WRITE, 0, CIGPreWrite, NULL },  // 写入
    { IRP_MJ_SET_INFORMATION, 0, CIGPreWrite, NULL },  // 删除/重命名/改属性
    { IRP_MJ_DIRECTORY_CONTROL,0,  CIGPreRead, NULL },  // 遍历目录（列举文件）
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
        &CIGFileFilterData.FilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)) {
        status = FltStartFiltering(CIGFileFilterData.FilterHandle);

        if (!NT_SUCCESS(status)) {
            FltUnregisterFilter(CIGFileFilterData.FilterHandle);
        }
    }
    DbgPrint("[CIG] Driver initialized and filtering started\n");
    return status;
}

NTSTATUS
CIGFileUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    FltUnregisterFilter(CIGFileFilterData.FilterHandle);

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

// 写入拦截策略
FLT_PREOP_CALLBACK_STATUS
CIGPreWrite(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
)
{
    *CompletionContext = NULL;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

     // 快速核验
    // 高IRQL下直接放行
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 快速核验是否为白名单进程
    PUNICODE_STRING processPath = NULL;
    if (NT_SUCCESS(SeLocateProcessImageName(PsGetCurrentProcess(), &processPath)) && processPath != NULL) {
        UNICODE_STRING guardianExePath = { 0 };
        RtlInitUnicodeString(&guardianExePath, L"\\DEVICE\\HARDDISKVOLUME*\\PROGRAM FILES\\GUARDIAN\\GUARDIAN.EXE");
        UNICODE_STRING configExePath = { 0 };
        RtlInitUnicodeString(&configExePath, L"\\DEVICE\\HARDDISKVOLUME*\\PROGRAM FILES\\GUARDIAN\\CONFIG.EXE");
        __try {
            if (FsRtlIsNameInExpression(&guardianExePath, processPath, TRUE, NULL) || FsRtlIsNameInExpression(&configExePath, processPath, TRUE, NULL)) {
                DbgPrint("[CIG] PassApp: %wZ\n", processPath);
                ExFreePool(processPath);
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            ;
        }
        ExFreePool(processPath);
    }

    // 特例处理
    // GuardianRecovery目录
    UNICODE_STRING guardianRecoveryPath = { 0 };
    RtlInitUnicodeString(&guardianRecoveryPath, L"\\DEVICE\\HARDDISKVOLUME*\\GUARDIANRECOVERY*");
    // Guardian目录
    UNICODE_STRING guardianPath = { 0 };
    RtlInitUnicodeString(&guardianPath, L"\\DEVICE\\HARDDISKVOLUME*\\PROGRAM FILES\\GUARDIAN*");

    UCHAR majorFunction = Data->Iopb->MajorFunction;

    // 判断是否为打开
    if (majorFunction == IRP_MJ_CREATE) {
        ULONG createDisposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
        if (createDisposition == FILE_OPEN) {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
    }

    // 通用拦截
    // 获取被操作目录
    PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
    if (NT_SUCCESS(FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fileNameInfo
    ))) {
        if (NT_SUCCESS(FltParseFileNameInformation(fileNameInfo))) {
            // 拦截逻辑
            if (FsRtlIsNameInExpression(&guardianRecoveryPath, &fileNameInfo->Name, TRUE, NULL) || FsRtlIsNameInExpression(&guardianPath, &fileNameInfo->Name, TRUE, NULL)) {
                DbgPrint("[CIG] ParentDir: %wZ\n", &fileNameInfo->Name);
                DbgPrint("[CIG] FLT_PREOP_COMPLETE\n\n");
                Data->IoStatus.Status = STATUS_ACCESS_DENIED;
                Data->IoStatus.Information = 0;
                FltReleaseFileNameInformation(fileNameInfo);
                return FLT_PREOP_COMPLETE;
            }
        }
        FltReleaseFileNameInformation(fileNameInfo);
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

// 读取拦截策略
FLT_PREOP_CALLBACK_STATUS
CIGPreRead(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Out_ PVOID* CompletionContext
)
{
    *CompletionContext = NULL;
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    // 高IRQL下直接放行
    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // 快速核验是否为白名单进程
    PUNICODE_STRING processPath = NULL;
    if (NT_SUCCESS(SeLocateProcessImageName(PsGetCurrentProcess(), &processPath)) && processPath != NULL) {
        UNICODE_STRING guardianExePath = { 0 };
        RtlInitUnicodeString(&guardianExePath, L"\\DEVICE\\HARDDISKVOLUME*\\PROGRAM FILES\\GUARDIAN\\GUARDIAN.EXE");
        UNICODE_STRING configExePath = { 0 };
        RtlInitUnicodeString(&configExePath, L"\\DEVICE\\HARDDISKVOLUME*\\PROGRAM FILES\\GUARDIAN\\CONFIG.EXE");
        __try {
            if (FsRtlIsNameInExpression(&guardianExePath, processPath, TRUE, NULL) || FsRtlIsNameInExpression(&configExePath, processPath, TRUE, NULL)) {
                DbgPrint("[CIG] PassApp: %wZ\n", processPath);
                ExFreePool(processPath);
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER){
            ;
        }
        ExFreePool(processPath);
    }

    // GuardianRecovery目录
    UNICODE_STRING guardianRecoveryPath = { 0 };
    RtlInitUnicodeString(&guardianRecoveryPath, L"\\DEVICE\\HARDDISKVOLUME*\\GUARDIANRECOVERY*");

    // 获取被操作目录
    PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
    if (NT_SUCCESS(FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fileNameInfo
    ))) {
        if (NT_SUCCESS(FltParseFileNameInformation(fileNameInfo))) {
            // 拦截逻辑
            if (FsRtlIsNameInExpression(&guardianRecoveryPath, &fileNameInfo->Name, TRUE, NULL)) {
                DbgPrint("[CIG] ParentDir: %wZ\n", &fileNameInfo->Name);
                DbgPrint("[CIG] FLT_PREOP_COMPLETE\n\n");
                Data->IoStatus.Status = STATUS_ACCESS_DENIED;
                Data->IoStatus.Information = 0;
                FltReleaseFileNameInformation(fileNameInfo);
                return FLT_PREOP_COMPLETE;
            }
        }
        FltReleaseFileNameInformation(fileNameInfo);
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}