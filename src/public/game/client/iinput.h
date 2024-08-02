#ifndef IINPUT_H
#define IINPUT_H

class bf_write;
class bf_read;
class CUserCmd;
struct kbutton_t;

struct CameraThirdData_t
{
	float	m_flPitch;
	float	m_flYaw;
	float	m_flDist;
	float	m_flLag;
	Vector3D	m_vecHullMin;
	Vector3D	m_vecHullMax;
};

abstract_class IInput
{
public:
	virtual ~IInput() {};

	// Initialization/shutdown of the subsystem
	virtual	void		Init_All( void ) = 0;
	virtual void		Shutdown_All( void ) = 0;
	// Latching button states
	virtual int			GetButtonBits( bool resetState ) = 0;
	// Create movement command
	virtual void		CreateMove ( int sequenceNumber, float inputSampleFrametime, bool active ) = 0;
	virtual void		ExtraMouseSample( float frametime ) = 0;
	virtual bool		WriteUsercmdDeltaToBuffer( int slot, bf_write *buf, int from, int to ) = 0;
	virtual void		EncodeUserCmdToBuffer( int slot, bf_write& buf, int sequenceNumber ) = 0;
	virtual void		DecodeUserCmdFromBuffer( int slot, bf_read& buf, int sequenceNumber ) = 0;

	virtual CUserCmd	*GetUserCmd( int slot, int sequenceNumber ) = 0; // TODO[ AMOS ]: CUserCmdExteded*

	virtual void MakeWeaponSelection( int secondary, int primary ) = 0;

	virtual void sub_140702970() = 0;
	virtual void sub_140702980() = 0;

	virtual bool MouseInitialized() = 0;

	// Retrieve key state
	virtual float		KeyState( kbutton_t* key ) = 0;

	virtual void sub_140701EC0() = 0;
	virtual void sub_140701F50() = 0;
	virtual void sub_140702000() = 0;

	// Look for key
	virtual kbutton_t* FindKey(const char* name) = 0;

	// Extra initialization for some joysticks
	virtual bool		ControllerModeActive() = 0;
	virtual bool		JoystickActive() = 0;

	virtual void		Joystick_SetSampleTime( float frametime ) = 0;
	virtual void		IN_SetSampleTime( float frametime ) = 0;

	// Accumulate mouse delta
	virtual void		AccumulateMouse( int nSlot ) = 0; // Unimplemented in R5

	// Activate/deactivate mouse
	virtual void		ActivateMouse( void ) = 0;
	virtual void		DeactivateMouse( void ) = 0;

	// Clear mouse state data
	virtual void		ClearStates( void ) = 0;
	// Retrieve lookspring setting
	virtual float		GetLookSpring( void ) = 0;

	// Clear weapon activities and additional UserCmd related data
	virtual void		ClearSupplementalWeaponData( void ) = 0;

	// Retrieve mouse position
	virtual void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = 0, int *unclampedy = 0 ) = 0;
	virtual void		SetFullscreenMousePos( int mx, int my ) = 0;
	virtual void		ResetMouse() = 0;
	virtual	float		GetLastForwardMove( void ) = 0;

	// Third Person camera
	virtual void		CAM_Think( void ) = 0;
	virtual int			CAM_IsThirdPerson( int nSlot = -1 ) = 0;
	virtual void		CAM_GetCameraOffset( Vector3D& ofs ) = 0;
	virtual void		CAM_SetCameraOffset( Vector3D& ofs ) = 0;
	virtual void		CAM_ToThirdPerson( void ) = 0;
	virtual void		CAM_ToFirstPerson( void ) = 0;
	virtual void		CAM_StartMouseMove( void ) = 0;
	virtual void		CAM_EndMouseMove( void ) = 0;
	virtual void		CAM_StartDistance( void ) = 0;
	virtual void		CAM_EndDistance( void ) = 0;
	virtual int			CAM_InterceptingMouse( void ) = 0;
	virtual	void		CAM_Command( int command ) = 0;

	virtual void		Unknown_SetFloatB0( float a2 ) = 0;
	virtual float		Unknown_GetFloatB0() = 0;

	virtual void		nullsub_0() = 0;
	virtual int			sub_1407060E0() = 0;

	// Causes an input to have to be re-pressed to become active
	virtual void		ClearInputButton( int bits ) = 0;

	virtual	void		CAM_SetCameraThirdData( CameraThirdData_t *pCameraData, const QAngle &vecCameraOffset ) = 0;
	virtual void		CAM_CameraThirdThink( void ) = 0;
};

#endif // IINPUT_H
