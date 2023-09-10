//
// [TEB/PEB UNDER 64-BIT WINDOWS]
// This file represents the 64-bit PEB and associated data structures for 64-bit Windows
// This PEB is allegedly valid between XP thru [at least] Windows 8
//
// [REFERENCES]
//      http://terminus.rewolf.pl/terminus/structures/ntdll/_PEB_x64.html
//      http://terminus.rewolf.pl/terminus/structures/ntdll/_TEB64_x86.html
//      https://github.com/giampaolo/psutil/commit/babd2b73538fcb6f3931f0ab6d9c100df6f37bcb     (RTL_USER_PROCESS_PARAMETERS)
//      https://redplait.blogspot.com/2011/09/w8-64bit-teb-peb.html                             (TEB)
//
// [CHANGELIST]
//    2018-05-02:   -now can be compiled alongside windows.h (without changes) or by defining WANT_ALL_WINDOWS_H_DEFINITIONS so this file can be used standalone
//                  -this file may also be included alongside tebpeb32.h which can be found at http://bytepointer.com/resources/tebpeb32.h
//                  -64-bit types no longer clash with the 32-bit ones; e.g. UNICODE_STRING64, RTL_USER_PROCESS_PARAMETERS64, PEB64 (same result whether 32 or 64-bit compiler is used)
//                  -added more QWORD aliases (i.e. HANDLE64 and PTR64) so underlying types are clearer, however most PEB members remain generic QWORD placeholders for now
//                  -fixed missing semicolon bug in UNICODE_STRING64
//                  -added prliminary RTL_USER_PROCESS_PARAMETERS64 and TEB64 with offsets
//                  -included byte offsets for PEB64
//
//    2017-08-25:   initial public release
//
#ifndef TEBPEB_64_H
#define TEBPEB_64_H

//
// base types
//

#include "Windows.h"

typedef struct _PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
    BOOLEAN ShutdownInProgress;
    HANDLE ShutdownThreadId;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

//always declare 64-bit types
#ifdef _MSC_VER
    //Visual C++
    typedef unsigned __int64    QWORD;
    typedef __int64             INT64;
#else
    //GCC
    typedef unsigned long long  QWORD;
    typedef long long           INT64;
#endif
typedef QWORD                   PTR64;

//UNCOMMENT line below if you are not including windows.h
//#define WANT_ALL_WINDOWS_H_DEFINITIONS
#ifdef WANT_ALL_WINDOWS_H_DEFINITIONS

//base types
typedef unsigned char           BYTE;
typedef char                    CHAR;
typedef unsigned short          WORD;
typedef short                   INT16;
typedef unsigned long           DWORD;
typedef long                    INT32;
typedef unsigned __int64        QWORD;
typedef __int64                 INT64;
typedef void*                   HANDLE;
typedef unsigned short          WCHAR;

//base structures
union LARGE_INTEGER
{
    struct
    {
        DWORD   LowPart;
        INT32   HighPart;
    } u;
    INT64       QuadPart;
};

union ULARGE_INTEGER
{
    struct
    {
       DWORD LowPart;
       DWORD HighPart;
    } u;
    QWORD       QuadPart;
};

#endif //#ifdef WANT_ALL_WINDOWS_H_DEFINITIONS

struct UNICODE_STRING64
{
    union
    {
        struct
        {
            WORD Length;
            WORD MaximumLength;
        } u;
        QWORD dummyalign;
    };
    QWORD Buffer;
};

typedef struct _CLIENT_ID64
{
     QWORD  ProcessId;
     QWORD  ThreadId;
} CLIENT_ID64;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    union
    {
        LIST_ENTRY InInitializationOrderLinks;
        LIST_ENTRY InProgressLinks;
    };
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING64 FullDllName;
    UNICODE_STRING64 BaseDllName;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;// [ PIXIE ]: Narrowed down version, don't need full.

//NOTE: the members of this structure are not yet complete
typedef struct _RTL_USER_PROCESS_PARAMETERS64
{
    BYTE                    Reserved1[16];                 //0x00
    QWORD                   Reserved2[5];                  //0x10
    UNICODE_STRING64        CurrentDirectoryPath;          //0x38
    QWORD                   CurrentDirectoryHandle;        //0x48
    UNICODE_STRING64        DllPath;                       //0x50 
    UNICODE_STRING64        ImagePathName;                 //0x60 
    UNICODE_STRING64        CommandLine;                   //0x70 
    PTR64                   Environment;                   //0x80
} RTL_USER_PROCESS_PARAMETERS64;

//
// PEB64 structure - TODO: comb more through http://terminus.rewolf.pl/terminus/structures/ntdll/_PEB_x64.html and add OS delineations and Windows 10 updates
//
// The structure represented here is a work-in-progress as only members thru offset 0x320 are listed; the actual sizes per OS are:
//    0x0358    XP/WS03
//    0x0368    Vista
//    0x037C    Windows 7
//    0x0388    Windows 8
//    0x07A0    Windows 10
//
struct PEB64
{
    union
    {
        struct
        {
            BYTE InheritedAddressSpace;                                 //0x000
            BYTE ReadImageFileExecOptions;                              //0x001
            BYTE BeingDebugged;                                         //0x002
            BYTE _SYSTEM_DEPENDENT_01;                                  //0x003
        } flags;
        QWORD dummyalign;
    } dword0;
    QWORD                           Mutant;                             //0x0008
    QWORD                           ImageBaseAddress;                   //0x0010
    PPEB_LDR_DATA                   Ldr;                                //0x0018
    PTR64                           ProcessParameters;                  //0x0020 / pointer to RTL_USER_PROCESS_PARAMETERS64
    QWORD                           SubSystemData;                      //0x0028
    QWORD                           ProcessHeap;                        //0x0030
    QWORD                           FastPebLock;                        //0x0038
    QWORD                           _SYSTEM_DEPENDENT_02;               //0x0040
    QWORD                           _SYSTEM_DEPENDENT_03;               //0x0048
    QWORD                           _SYSTEM_DEPENDENT_04;               //0x0050
    union
    {
        QWORD                       KernelCallbackTable;                //0x0058
        QWORD                       UserSharedInfoPtr;                  //0x0058
    };
    DWORD                           SystemReserved;                     //0x0060
    DWORD                           _SYSTEM_DEPENDENT_05;               //0x0064
    QWORD                           _SYSTEM_DEPENDENT_06;               //0x0068
    QWORD                           TlsExpansionCounter;                //0x0070
    QWORD                           TlsBitmap;                          //0x0078
    DWORD                           TlsBitmapBits[2];                   //0x0080
    QWORD                           ReadOnlySharedMemoryBase;           //0x0088
    QWORD                           _SYSTEM_DEPENDENT_07;               //0x0090
    QWORD                           ReadOnlyStaticServerData;           //0x0098
    QWORD                           AnsiCodePageData;                   //0x00A0
    QWORD                           OemCodePageData;                    //0x00A8
    QWORD                           UnicodeCaseTableData;               //0x00B0
    DWORD                           NumberOfProcessors;                 //0x00B8
    union
    {
        DWORD                       NtGlobalFlag;                       //0x00BC
        DWORD                       dummy02;                            //0x00BC
    };
    LARGE_INTEGER                   CriticalSectionTimeout;             //0x00C0
    QWORD                           HeapSegmentReserve;                 //0x00C8
    QWORD                           HeapSegmentCommit;                  //0x00D0
    QWORD                           HeapDeCommitTotalFreeThreshold;     //0x00D8
    QWORD                           HeapDeCommitFreeBlockThreshold;     //0x00E0
    DWORD                           NumberOfHeaps;                      //0x00E8
    DWORD                           MaximumNumberOfHeaps;               //0x00EC
    QWORD                           ProcessHeaps;                       //0x00F0
    QWORD                           GdiSharedHandleTable;               //0x00F8
    QWORD                           ProcessStarterHelper;               //0x0100
    QWORD                           GdiDCAttributeList;                 //0x0108
    QWORD                           LoaderLock;                         //0x0110
    DWORD                           OSMajorVersion;                     //0x0118
    DWORD                           OSMinorVersion;                     //0x011C
    WORD                            OSBuildNumber;                      //0x0120
    WORD                            OSCSDVersion;                       //0x0122
    DWORD                           OSPlatformId;                       //0x0124
    DWORD                           ImageSubsystem;                     //0x0128
    DWORD                           ImageSubsystemMajorVersion;         //0x012C
    QWORD                           ImageSubsystemMinorVersion;         //0x0130
    union
    {
        QWORD                       ImageProcessAffinityMask;           //0x0138
        QWORD                       ActiveProcessAffinityMask;          //0x0138
    };
    QWORD                           GdiHandleBuffer[30];                //0x0140
    QWORD                           PostProcessInitRoutine;             //0x0230
    QWORD                           TlsExpansionBitmap;                 //0x0238
    DWORD                           TlsExpansionBitmapBits[32];         //0x0240
    QWORD                           SessionId;                          //0x02C0
    ULARGE_INTEGER                  AppCompatFlags;                     //0x02C8
    ULARGE_INTEGER                  AppCompatFlagsUser;                 //0x02D0
    QWORD                           pShimData;                          //0x02D8
    QWORD                           AppCompatInfo;                      //0x02E0
    UNICODE_STRING64                CSDVersion;                         //0x02E8
    QWORD                           ActivationContextData;              //0x02F8
    QWORD                           ProcessAssemblyStorageMap;          //0x0300
    QWORD                           SystemDefaultActivationContextData; //0x0308
    QWORD                           SystemAssemblyStorageMap;           //0x0310
    QWORD                           MinimumStackCommit;                 //0x0318

}; //struct PEB64

//
// TEB64 structure - preliminary structure; the portion listed current at least as of Windows 8
//
struct TEB64
{
    NT_TIB64                        NtTib;                              //0x0000
    PVOID                           EnvironmentPointer;                 //0x0038
    CLIENT_ID64                     ClientId;                           //0x0040
    PVOID                           ActiveRpcInfo;                      //0x0050
    PVOID                           ThreadLocalStoragePointer;          //0x0058
    PEB64*                          ProcessEnvironmentBlock;            //0x0060
    ULONG                           LastErrorValue;                     //0x0068
    ULONG                           CountOfOwnedCriticalSections;       //0x006C
    PVOID                           CsrClientThread;                    //0x0070
    PVOID                           Win32ThreadInfo;                    //0x0078
    ULONG                           Win32ClientInfo[0x1F];              //0x0080
    PVOID                           WOW32Reserved;                      //0x0100
    ULONG                           CurrentLocale;                      //0x0108
    ULONG                           FpSoftwareStatusRegister;           //0x010C
    PVOID                           SystemReserved1[0x36];              //0x0110
    PVOID                           Spare1;                             //0x02C0
    ULONG                           ExceptionCode;                      //0x02C8
    PVOID                           ActivationContextStackPointer;      //0x02D0
    ULONG                           SpareBytes1[0x26];                  //0x02D8
    PVOID                           SystemReserved2[0xA];               //0x0370
    ULONG                           GdiRgn;                             //0x03C0
    ULONG                           GdiPen;                             //0x03C4
    ULONG                           GdiBrush;                           //0x03C8
    CLIENT_ID64                     RealClientId;                       //0x03D0
    PVOID                           GdiCachedProcessHandle;             //0x03E0
    ULONG                           GdiClientPID;                       //0x03E8
    ULONG                           GdiClientTID;                       //0x03EC
    PVOID                           GdiThreadLocaleInfo;                //0x03F0
    PVOID                           UserReserved[5];                    //0x03F8
    PVOID                           GlDispatchTable[0x118];             //0x0420
    ULONG                           GlReserved1[0x1A];                  //0x0CE0
    PVOID                           GlReserved2;                        //0x0D48
    PVOID                           GlSectionInfo;                      //0x0D50
    PVOID                           GlSection;                          //0x0D58
    PVOID                           GlTable;                            //0x0D60
    PVOID                           GlCurrentRC;                        //0x0D68
    PVOID                           GlContext;                          //0x0D70
    NTSTATUS                        LastStatusValue;                    //0x0D78
    UNICODE_STRING64                StaticUnicodeString;                //0x0D80
    WCHAR                           StaticUnicodeBuffer[0x105];         //0x0D90
    PVOID                           DeallocationStack;                  //0x0FA0
    PVOID                           TlsSlots[0x40];                     //0x0FA9
    LIST_ENTRY                      TlsLinks;                           //0x11A8
    PVOID                           Vdm;                                //0x11B8
    PVOID                           ReservedForNtRpc;                   //0x11C0
    PVOID                           DbgSsReserved[0x2];                 //0x11C8
    ULONG                           HardErrorDisabled;                  //0x11D8
    PVOID                           Instrumentation[0x10];              //0x11E0
    PVOID                           WinSockData;                        //0x1260
    ULONG                           GdiBatchCount;                      //0x1268
    ULONG                           Spare2;                             //0x126C
    ULONG                           Spare3;                             //0x1270
    ULONG                           Spare4;                             //0x1274
    PVOID                           ReservedForOle;                     //0x1278
    ULONG                           WaitingOnLoaderLock;                //0x1280
    PVOID                           StackCommit;                        //0x1288
    PVOID                           StackCommitMax;                     //0x1290
    PVOID                           StackReserved;                      //0x1298
    PVOID                           TlsExpansionSlots;                  //0x12A0
};

#endif // TEBPEB_64_H
