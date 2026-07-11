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
    { IRP_MJ_DIRECTORY_CONTROL,0, CIGPreRead, NULL },  // 遍历目录（列举文件）
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

    // 先不写，直接返回成功
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

    DbgPrint("[CIG] PreRead entered\n");

    // 快速核验是否为白名单进程
    if (KeGetCurrentIrql() <= PASSIVE_LEVEL) {
        PUNICODE_STRING processPath = NULL;
        if (NT_SUCCESS(SeLocateProcessImageName(PsGetCurrentProcess(), &processPath)) && processPath != NULL) {
            //DbgPrint("Process Image Path: %wZ\n",processPath);
            ExFreePool(processPath);
        }
    }
    else {
        // 不安全：放弃获取进程路径，直接放行
        //DbgPrint("[CIG] IRQL too high, skipping process path check\n");
    }

    // GuardianRecovery目录
    UNICODE_STRING guardianRecoveryPath = { 0 };
    RtlInitUnicodeString(&guardianRecoveryPath, L"\\GuardianRecovery");

    // 获取被操作目录
    PFLT_FILE_NAME_INFORMATION fileNameInfo = NULL;
    if (NT_SUCCESS(FltGetFileNameInformation(
        Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
        &fileNameInfo
    ))) {
        DbgPrint("[CIG] FltGetFileNameInformation entered\n");
        if (NT_SUCCESS(FltParseFileNameInformation(fileNameInfo))) {
            // 拦截逻辑
            DbgPrint("[CIG] Read Name: %wZ\n", &fileNameInfo->Name);
            if (RtlPrefixUnicodeString(&guardianRecoveryPath, &fileNameInfo -> Name, TRUE)) {
                DbgPrint("[CIG] FLT_PREOP_COMPLETE\n");
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