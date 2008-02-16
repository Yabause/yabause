/*
                      CyberSpace Network Protocol

	Base network protocol specification used by the ShipWars.
 */

#ifndef CS_H
#define CS_H


/*
 *	String Deliminator Character:
 */
#define CS_STRING_DELIMINATOR_CHAR	';'


/*
 *	Data Segment Maximums Length (in bytes):
 *
 *	CS_DATA_MAX_LEN must be big enough to contain all the bytes
 *	of all the different CS message types.
 */
#define CS_DATA_MAX_LEN			384
#define CS_DATA_MAX_BACKLOG		100000



/*
 *	Message length Maximums:
 *
 *	For CS_CODE_LIVEMESSAGE and others
 *
 *	Must be smaller than CS_DATA_MAX_LEN.
 */
#define CS_MESG_MAX			256


/*
 *	CS Message Types:
 *
 *	Each CS message consists of:
 *
 *	A command code (one of CS_CODE_*) which identifies the CS
 *	message type (this is always the first value).
 *
 *	Arguments which describe the subsequent values of a CS message,
 *	the arguments specification may differ depending on if the
 *	CS message is sent to the server or to the client.
 */

/*
 *	To client: major minor release
 *	To server: major minor release
 *
 *	Both the client and the server should send this to one another
 *	on initial connect (before login) to identify which CS protocol
 *	version it is using.
 */
#define CS_CODE_VERSION			10

/*
 *	To client:
 *	To server: name;password;client_type
 */
#define CS_CODE_LOGIN			11

/*
 *	To client:
 *	To server:
 */
#define CS_CODE_LOGOUT			12

/*
 *	To client: object_num;name
 *	To server:
 */
#define CS_CODE_WHOAMI			13

/*
 *      To client: *not sent*
 *      To server:
 */
#define CS_CODE_REFRESH			14

/*
 *      To client: *not sent*
 *      To server: interval
 */
#define CS_CODE_INTERVAL		15

/*
 *      To client: path
 *      To server:
 */
#define CS_CODE_IMAGESET		16

/*
 *      To client: path
 *      To server:
 */
#define CS_CODE_SOUNDSET		17


/*
 *	To client: url
 *	To server: *not sent*
 */
#define CS_CODE_FORWARD			20

/*
 *	To client: message
 *	To server: *not sent*
 */
#define CS_CODE_LIVEMESSAGE		30

/*
 *	To client: sys_mesg_code message
 *	To server: *not sent*
 *
 *	Note: sys_mesg_code can be one of CS_SYSMESG_*.
 */
#define CS_CODE_SYSMESSAGE		31

/*
 *	To client: *not sent*
 *	To server: command
 */
#define CS_CODE_LITERALCMD		32


/*
 *	To client: sndref_num left_volume right_volume
 *	To server: *not sent*
 */
#define CS_CODE_PLAYSOUND		37


/*
 *	To client: object_num, type, isref_num, owner, size,
 *	           locked_on, intercepting_object, scanner_range,
 *		   sect_x, sect_y, sect_z,
 *	           x, y, z, heading, pitch, bank,
 *	           velocity, velocity_heading, velocity_pitch,
 *	           velocity_bank, current_frame, anim_int,
 *	           total_frames, cycle_times
 *	To server: *not sent*
 */
#define CS_CODE_CREATEOBJ		40

/* 
 *      To client: object_num
 *      To server: *not sent*
 */
#define CS_CODE_RECYCLEOBJ		41

/*
 *      To client: object_num, type, imageset, size, x, y, z,
 *                 heading, pitch, bank, velocity, velocity_heading,
 *                 velocity_pitch, velocity_bank, current_frame
 *      To server: *not sent*
 */
#define CS_CODE_POSEOBJ			42

/*
 *	To client: object_num, type, imageset, size, x, y, z,
 *	           heading, pitch, bank, velocity, velocity_heading,
 *	           velocity_pitch, velocity_bank, current_frame
 *	To server: *not sent*
 */
#define CS_CODE_FORCEPOSEOBJ		43


/*
 *	To client: propriatery_code [arg...]
 *	To server: propriatery_code [arg...]
 */
#define CS_CODE_EXT			80




/*
 *	Systems Message Codes:
 *
 *	These are prefix codes for each CS_CODE_SYSMESSAGE.
 *	Client and server should NOT send any message with a prefix code
 *	not listed here.
 *
 *	Example for a CS_CODE_SYSMESSAGE using
 *	CS_SYSMESG_LOGINFAIL:
 *
 *	    "31 10 Login failed."
 *
 *	The message (third optional argument) must be less than
 *	CS_MESG_MAX in bytes.
 */
#define CS_SYSMESG_CODE_ERROR		0
#define CS_SYSMESG_CODE_WARNING		1
#define CS_SYSMESG_CODE_INFO		2

#define CS_SYSMESG_CODE_LOGINFAIL	10
#define CS_SYSMESG_CODE_LOGINSUCC	11

#define CS_SYSMESG_CODE_ABNDISCON	15	/* Abnormal disconnect */

#define CS_SYSMESG_CODE_BADVALUE	20
#define CS_SYSMESG_CODE_BADARG		21


#endif /* CS_H */
