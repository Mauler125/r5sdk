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
    Warning(eDLL_T::FS, "Usage:\n");
    Warning(eDLL_T::FS, "  revpk " PACK_COMMAND " <locale> <context> <levelName>\n");
    Warning(eDLL_T::FS, "  revpk " UNPACK_COMMAND " <fileName> <sanitize>\n");
}

//-----------------------------------------------------------------------------
// Purpose: packs VPK files into 'SHIP' VPK directory
//-----------------------------------------------------------------------------
static void ReVPK_Pack(const CCommand& args)
{
    if (args.ArgC() < 5)
    {
        ReVPK_Usage();
        return;
    }

    VPKPair_t pair(args.Arg(2), args.Arg(3), args.Arg(4), NULL);
    CFastTimer timer;

    Msg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", pair.m_DirName.Get());
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzCompParams();
    builder.PackWorkspace(pair, "ship/", "vpk/");

    timer.End();
    Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
    Msg(eDLL_T::FS, "\n");
}

//-----------------------------------------------------------------------------
// Purpose: unpacks VPK files into workspace directory
//-----------------------------------------------------------------------------
static void ReVPK_Unpack(const CCommand& args)
{
    if (args.ArgC() < 3)
    {
        ReVPK_Usage();
        return;
    }

    CUtlString arg = args.Arg(2);

    VPKDir_t vpk(arg, (args.ArgC() > 3));
    CFastTimer timer;

    Msg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", arg.Get());
    timer.Start();

    CPackedStoreBuilder builder;

    builder.InitLzDecompParams();
    builder.UnpackWorkspace(vpk, "ship/");

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
