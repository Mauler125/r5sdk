//=============================================================================//
//
// Purpose: Standalone VPK tool
//
//=============================================================================//
#include "core/logdef.h"
#include "core/logger.h"
#include "tier0/fasttimer.h"
#include "tier0/cpu.h"
#include "tier1/cmd.h"
#include "tier1/fmtstr.h"
#include "tier1/keyvalues.h"
#include "windows/console.h"
#include "vpklib/packedstore.h"

#include "vstdlib/keyvaluessystem.h"
#include "filesystem/filesystem_std.h"

#define PACK_COMMAND "pack"
#define UNPACK_COMMAND "unpack"

static CKeyValuesSystem s_KeyValuesSystem;
static CFileSystem_Stdio g_FullFileSystem;

//-----------------------------------------------------------------------------
// Purpose: keyvalues singleton accessor
//-----------------------------------------------------------------------------
IKeyValuesSystem* KeyValuesSystem()
{
    return &s_KeyValuesSystem;
}

//-----------------------------------------------------------------------------
// Purpose: filesystem singleton accessor
//-----------------------------------------------------------------------------
CFileSystem_Stdio* FileSystem()
{
    return &g_FullFileSystem;
}

//-----------------------------------------------------------------------------
// Purpose: init
//-----------------------------------------------------------------------------
static void ReVPK_Init()
{
    CheckCPUforSSE2();

    g_CoreMsgVCallback = EngineLoggerSink;
    lzham_enable_fail_exceptions(true);

    Console_Init(true);
    SpdLog_Init(true);
}

//-----------------------------------------------------------------------------
// Purpose: shutdown
//-----------------------------------------------------------------------------
static void ReVPK_Shutdown()
{
    // Must be done to flush all buffers.
    SpdLog_Shutdown();
    Console_Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: logs tool's usage
//-----------------------------------------------------------------------------
static void ReVPK_Usage()
{
    CFmtStr1024 usage;

    usage.Format(
        "ReVPK instructions and options:\n"
        "For packing; run 'revpk %s' with the following parameters:\n"
        "\t<%s>\t- locale prefix for the directory tree file\n"
        "\t<%s>\t- context scope for the VPK files [\"server\", \"client\"]\n"
        "\t<%s>\t- level name for the VPK files\n"
        "\t<%s>\t- ( optional ) path to the workspace containing the control file\n"
        "\t<%s>\t- ( optional ) path in which the VPK files will be built\n"
        "\t<%s>\t- ( optional ) max LZHAM helper threads [\"%d\", \"%d\"] \"%d\" ( default ) for max practical\n"
        "\t<%s>\t- ( optional ) the level of compression [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"]\n\n"

        "For unpacking; run 'revpk %s' with the following parameters:\n"
        "\t<%s>\t- name of the target directory tree file\n"
        "\t<%s>\t- ( optional ) path to directory containing the target directory tree file\n"
        "\t<%s>\t- ( optional ) whether to parse the directory tree file name from the data block file name\n",

        PACK_COMMAND, // Pack parameters:
        "locale", "context", "levelName", "workspacePath", "buildPath",
        
        "numThreads", // Num helper threads.
        -1, LZHAM_MAX_HELPER_THREADS, -1,
        
        "compressLevel", // Compress level.
        "fastest", "faster", "default", "better", "uber",

        UNPACK_COMMAND,// Unpack parameters:
        "fileName", "inputDir", "sanitize"
    );

    Warning(eDLL_T::FS, "%s", usage.Get());
}

//-----------------------------------------------------------------------------
// Purpose: packs VPK files into 'SHIP' VPK directory
//-----------------------------------------------------------------------------
static void ReVPK_Pack(const CCommand& args)
{
    const int argCount = args.ArgC();

    if (argCount < 5)
    {
        ReVPK_Usage();
        return;
    }

    VPKPair_t pair(args.Arg(2), args.Arg(3), args.Arg(4), NULL);
    CFastTimer timer;

    Msg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", pair.m_DirName.Get());
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzEncoder(
        argCount > 7 ? (std::min)(atoi(args.Arg(7)), LZHAM_MAX_HELPER_THREADS) : -1, // Num threads.
        argCount > 8 ? args.Arg(8) : "default"); // Compress level.

    builder.PackStore(pair,
        argCount > 5 ? args.Arg(5) : "ship/", // Workspace path.
        argCount > 6 ? args.Arg(6) : "vpk/"); // build path.

    timer.End();
    Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
    Msg(eDLL_T::FS, "\n");
}

//-----------------------------------------------------------------------------
// Purpose: unpacks VPK files into workspace directory
//-----------------------------------------------------------------------------
static void ReVPK_Unpack(const CCommand& args)
{
    const int argCount = args.ArgC();

    if (argCount < 3)
    {
        ReVPK_Usage();
        return;
    }

    CUtlString arg = args.Arg(2);

    VPKDir_t vpk(arg, (argCount > 4));
    CFastTimer timer;

    Msg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", arg.Get());
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzDecoder();
    builder.UnpackStore(vpk, argCount > 3 ? args.Arg(3) : "ship/");

    timer.End();
    Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
    Msg(eDLL_T::FS, "\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    ReVPK_Init();

    CCommand args;
    CUtlString str;

    for (int i = 0; i < argc; i++)
    {
        str.Append(argv[i]);
        str.Append(' ');
    }

    args.Tokenize(str.Get(), cmd_source_t::kCommandSrcCode);

    if (!args.ArgC())
        ReVPK_Usage();
    else
    {
        if (strcmp(args.Arg(1), PACK_COMMAND) == NULL)
            ReVPK_Pack(args);
        else if (strcmp(args.Arg(1), UNPACK_COMMAND) == NULL)
            ReVPK_Unpack(args);
        else
            ReVPK_Usage();
    }

    ReVPK_Shutdown();
}
