/**************************************************************************************************
  Revised:        2014-12-04
  Author:         Zhu Jie . Jay . Sleepace
**************************************************************************************************/

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   �ض���
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

#ifndef __MD_TYPEDEF_H__
#define __MD_TYPEDEF_H__

#include <string.h>
#include <stdio.h>
#include <stdint.h>


#define mdNULL      0
#define mdTRUE      1
#define mdFALSE     0

typedef int8_t          mdINT8;
typedef int16_t         mdINT16;
typedef int32_t         mdINT32;
typedef int64_t         mdINT64;

typedef uint8_t         mdUINT8;
typedef uint16_t        mdUINT16;
typedef uint32_t        mdUINT32;
typedef uint64_t        mdUINT64;

// Special numbers

typedef mdUINT8             mdBOOL;
typedef mdUINT16	    mdWORD;
typedef mdUINT32	    mdDWORD;

// Unsigned numbers
typedef unsigned char     mdBYTE;
typedef unsigned char     byte;
typedef unsigned char     *pbyte;
typedef unsigned char     u8;
typedef unsigned char     mdUCHAR;

typedef unsigned short  mdUSHORT;
typedef unsigned short  u16;
typedef unsigned short  ushort;
// int, long ƽ̨���
typedef unsigned int    mdUINT;
typedef unsigned int    u32;
typedef unsigned long   mdULONG;

// Signed numbers
typedef signed char     mdCHAR;
typedef signed short    mdSHORT;
typedef signed short    i16;
// int, long ƽ̨���
typedef signed int      mdINT;
typedef signed int      i32;
typedef signed long     mdLONG;

// decimal
typedef float           mdFLOAT;
typedef double          mdDOUBLE;

typedef enum {
    MD_ENOERR,      // ����
    MD_EPARAM,      // �����쳣
    MD_EOVERFLOW,   // Խ��
    MD_ENOINIT,     // δ��ʼ��

    MD_EBUSY,       // æ
    MD_ETIMEOUT,    // ��ʱ

    MD_EUNKNOWN     // δ֪����
} eMDErrorCode;

typedef enum {
    MD_IDE_TYPE_IAR         = 0x00,     //IAR
    MD_IDE_TYPE_KEIL        = 0x01,     //KEIL
    MD_IDE_TYPE_GCC         = 0x02,     //GCC
} eMDIDEType;

#endif

