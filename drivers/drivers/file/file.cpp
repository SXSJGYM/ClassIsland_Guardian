#include <ntddk.h>

VOID CigDriverUnload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    DbgPrint("[CIG] Driver has been successfully unloaded.\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("[CIG] Hello, Kernel! Driver is loading...\n");
    DriverObject->DriverUnload = CigDriverUnload;
    return STATUS_SUCCESS;
}