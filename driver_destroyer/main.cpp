#include <ntifs.h>
#include <ntddk.h>

NTSTATUS DelDriverFile(PUNICODE_STRING pUsDriverPath)
{
	DbgPrintEx(0, 0, "[+] %wZ\n", pUsDriverPath);

	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE FileHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(
		&ObjectAttributes,
		pUsDriverPath,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		0,
		0);

	NTSTATUS Status = IoCreateFileEx(&FileHandle,
		SYNCHRONIZE | DELETE,
		&ObjectAttributes,
		&IoStatusBlock,
		nullptr,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_DELETE,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
		nullptr,
		0,
		CreateFileTypeNone,
		nullptr,
		IO_NO_PARAMETER_CHECKING,
		nullptr);

	if (!NT_SUCCESS(Status))
	{
		DbgPrintEx(0, 0, "IoCreateFileEx ERROR\n");
		return Status;
	}

	PFILE_OBJECT FileObject;
	Status = ObReferenceObjectByHandleWithTag(FileHandle,
		SYNCHRONIZE | DELETE,
		*IoFileObjectType,
		KernelMode,
		'eliF',
		reinterpret_cast<PVOID*>(&FileObject),
		nullptr);
	if (!NT_SUCCESS(Status))
	{
		ObCloseHandle(FileHandle, KernelMode);
		return Status;
	}

	const PSECTION_OBJECT_POINTERS SectionObjectPointer = FileObject->SectionObjectPointer;
	SectionObjectPointer->ImageSectionObject = nullptr;

	// call MmFlushImageSection, make think no backing image and let NTFS to release file lock
	CONST BOOLEAN ImageSectionFlushed = MmFlushImageSection(SectionObjectPointer, MmFlushForDelete);

	ObfDereferenceObject(FileObject);
	ObCloseHandle(FileHandle, KernelMode);

	if (ImageSectionFlushed)
	{
		// chicken fried rice
		Status = ZwDeleteFile(&ObjectAttributes);
		if (NT_SUCCESS(Status))
		{
			return Status;
		}
	}
	return Status;
}

NTSTATUS DriverEntry(PVOID drv_obj, PVOID registry_path) {
	UNREFERENCED_PARAMETER(drv_obj);
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING drv_path = { 0 };
	RtlInitUnicodeString(&drv_path, L"\\DosDevices\\C:\\Users\\gamep\\AppData\\Local\\Temp\\qiLIjR64pokNPamw");

	if (!NT_SUCCESS(DelDriverFile(&drv_path)))
	{
		DbgPrintEx(0, 0, "Delete ERROR\n");
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	DbgPrintEx(0, 0, "Delete SUCCESS!!\n");
	return STATUS_SUCCESS;
}
