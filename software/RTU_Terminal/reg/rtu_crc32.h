
#ifndef __RT_CRC32_H__
#define __RT_CRC32_H__

#ifdef __cplusplus
extern "C" {
#endif

unsigned long rtu_crc32(unsigned long crc, const void *data, int len);
unsigned long rtu_ncrc32(unsigned long crc, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif


