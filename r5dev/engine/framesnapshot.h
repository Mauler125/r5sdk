#if !defined( FRAMESNAPSHOT_H )
#define FRAMESNAPSHOT_H
#include "public/inetchannel.h"

struct CFrameSnapshot
{
	char field_0;
	_DWORD last_entity;
	_DWORD snap_dword8;
	_DWORD snap_dwordC;
	_BYTE snap_byte10;
	_BYTE gap11[3];
	_DWORD snap_dword14;
	_DWORD snap_dword18;
	_BYTE snap_byte1C;
	_BYTE gap1D[3];
	_DWORD snap_dword20;
	_BYTE snap_byte24;
	_BYTE snap_byte25;
	_BYTE snap_byte26;
	__unaligned __declspec(align(1)) _WORD word27;
	nettick_t m_TickUpdate;
	_BYTE gap44[4];
	_QWORD qword48;
	_QWORD qword50;
	char buffer_0x20000[131072];
	char buffer_0x800[2048];
	char transmit_entity[4096];
};

static_assert(sizeof(CFrameSnapshot) == 0x21858);

#endif // FRAMESNAPSHOT_H