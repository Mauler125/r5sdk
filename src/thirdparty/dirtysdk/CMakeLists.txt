cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "DirtySDK" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Platform"
# Core source files
	"source/platform/plat-str.c"
	"source/platform/plat-time.c"
)

add_sources( SOURCE_GROUP "Comm"
# Core source files
	"source/comm/commsrp.c"
	"source/comm/commudp.c"
	"source/comm/commudputil.c"
)

add_sources( SOURCE_GROUP "Crypto"
# Core source files
	"source/crypt/cryptaes.c"
	"source/crypt/cryptarc4.c"
	"source/crypt/cryptchacha.c"
	"source/crypt/cryptcurve.c"
	"source/crypt/cryptgcm.c"
	"source/crypt/crypthash.c"
	"source/crypt/crypthmac.c"
	"source/crypt/cryptmd5.c"
	"source/crypt/cryptmont.c"
	"source/crypt/cryptnist.c"
	"source/crypt/cryptrandcommon.c"
	"source/crypt/cryptrsa.c"
	"source/crypt/cryptsha1.c"
	"source/crypt/cryptsha2.c"
# Optional core source files
	"source/crypt/cryptbn.c"
# PC plat source files
	"source/crypt/cryptrand.c"
)

add_sources( SOURCE_GROUP "Crypto/Include"
# Core include files
	"source/crypt/cryptrandpriv.h"
	"source/crypt/cryptrandcommon.h"
)

add_sources( SOURCE_GROUP "Socket"
# Core source files
	"source/dirtysock/dirtyaddr.c"
	"source/dirtysock/dirtycert.c"
	"source/dirtysock/dirtyerr.c"
	"source/dirtysock/dirtylib.c"
	"source/dirtysock/dirtylib.cpp"
	"source/dirtysock/dirtymem.c"
	"source/dirtysock/dirtynames.c"
	"source/dirtysock/dirtynet.c"
	"source/dirtysock/dirtythread.cpp"
	"source/dirtysock/dirtyuser.c"
	"source/dirtysock/netconn.c"
	"source/dirtysock/netconncommon.c"
	#"source/dirtysock/netconnlocaluser.cpp"
# PC plat source files
	"source/dirtysock/dirtyaddr.c"
	"source/dirtysock/pc/dirtyerrpc.c"
	"source/dirtysock/pc/dirtylibwin.c"
	"source/dirtysock/pc/dirtynetwin.c"
	"source/dirtysock/pc/netconnwin.c"
)

add_sources( SOURCE_GROUP "Socket/Include"
# Core include files
	"source/dirtysock/dirtynetpriv.h"
	"source/dirtysock/netconncommon.h"
	"source/dirtysock/netconnlocaluser.h"
)

add_sources( SOURCE_GROUP "Game"
# Core source files
	"source/game/connapi.c"
	"source/game/netgamedist.c"
	"source/game/netgamedistserv.c"
	"source/game/netgamelink.c"
	"source/game/netgameutil.c"
)

add_sources( SOURCE_GROUP "Codec"
# Core source files
	"source/graph/dirtygif.c"
	"source/graph/dirtygraph.c"
	"source/graph/dirtyjpg.c"
	"source/graph/dirtypng.c"
)

add_sources( SOURCE_GROUP "Misc"
# Core source files
	"source/misc/qosclient.c"
	"source/misc/qoscommon.c"
	"source/misc/weblog.c"
)

add_sources( SOURCE_GROUP "Misc/Include"
# Core include files
	"source/misc/userapipriv.h"
)

add_sources( SOURCE_GROUP "Protocol"
# Core source files
	"source/proto/protoadvt.c"
	"source/proto/protohttpmanager.c"
	"source/proto/protohttpserv.c"
	"source/proto/protohttputil.c"
	"source/proto/protohttp2.c"
	"source/proto/protoname.c"
	"source/proto/protostream.c"
	"source/proto/prototunnel.c"
	"source/proto/protoupnp.c"
# PC plat source files
	"source/proto/protohttp.c"
	"source/proto/protomangle.c"
	"source/proto/protossl.c"
	"source/proto/protowebsocket.c"
)

add_sources( SOURCE_GROUP "Utility"
# Core source files
	"source/util/aws.c"
	"source/util/binary7.c"
	"source/util/base64.c"
	"source/util/hpack.c"
	"source/util/jsonformat.c"
	"source/util/jsonparse.c"
	"source/util/murmurhash3.c"
	"source/util/protobufcommon.c"
	"source/util/protobufread.c"
	"source/util/protobufwrite.c"
	"source/util/utf8.c"
)

add_sources( SOURCE_GROUP "Voip"
# Core source files
	"source/voip/voiptunnel.c"
	"source/voip/voipgroup.c"
# PC plat source files
	"source/voip/voip.c"
	"source/voip/voipblocklist.c"
	"source/voip/voipconduit.c"
	"source/voip/voipcodec.c"
	"source/voip/voipcommon.c"
	"source/voip/voipconnection.c"
	"source/voip/voipdvi.c"
	"source/voip/voipmixer.c"
	"source/voip/voippcm.c"
	"source/voip/pc/voipheadsetpc.c"
	"source/voip/pc/voippc.c"
	"source/voip/voiptranscribe.c"

	"source/voip/pc/voipnarratepc.cpp"
)

add_sources( SOURCE_GROUP "Voip/Include"
# Core include files
	"source/voip/voipcommon.h"
	"source/voip/voipconduit.h"
	"source/voip/voipconnection.h"
	"source/voip/voipdvi.h"
	"source/voip/voipmixer.h"
	"source/voip/voippacket.h"
	"source/voip/voippcm.h"
	"source/voip/voippriv.h"
)

add_sources( SOURCE_GROUP "XML"
# Core source files
	"source/xml/xmlformat.c"
	"source/xml/xmlparse.c"
)

end_sources()
thirdparty_suppress_warnings()

target_include_directories( ${PROJECT_NAME} PRIVATE
	"${THIRDPARTY_SOURCE_DIR}/dirtysdk/include/"
	"${THIRDPARTY_SOURCE_DIR}/ea/"
	"${THIRDPARTY_SOURCE_DIR}/ea/EAThread/include/"

	"${THIRDPARTY_SOURCE_DIR}/dirtysdk/source/crypt/"
	"${THIRDPARTY_SOURCE_DIR}/dirtysdk/source/dirtysock/"
	"${THIRDPARTY_SOURCE_DIR}/dirtysdk/source/voip/"
)

target_compile_definitions( ${PROJECT_NAME} PRIVATE DIRTYCODE_LOGGING=0 )
