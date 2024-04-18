/*
   ByteBuffer (C implementation)
   bytebuffer.h
*/

#ifndef _BYTEBUFFER_H_
#define _BYTEBUFFER_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

 

#if !BIG_EDIAN && !LITTLE_EDIAN

#error Must Define BIG_EDIAN Or LITTLE_EDIAN

#elif BIG_EDIAN && LITTLE_EDIAN

#error Can Not Define BIG_EDIAN And LITTLE_EDIAN Both

#else

#ifdef BIG_EDIAN
#define htons(_n) (_n)
#define ntohs(_n) (_n)
#define htonl(_n) (_n)
#define ntohl(_n) (_n)
#else
#define htons(_n) __REV16(_n)
#define ntohs(_n) __REV16(_n)
#define htonl(_n) __REV(_n)
#define ntohl(_n) __REV(_n)
#endif

#endif

#define ___INLINE static inline

//#define size_t u32 

typedef struct {
    uint32_t pos;	// Read/Write position
    size_t len;		// Length of buf array
    uint8_t *buf;
} byte_buffer_t;

// Creat functions
byte_buffer_t *bb_new_from_buf(byte_buffer_t *bb, uint8_t *buf, size_t len);

// Utility
void bb_skip(byte_buffer_t *bb, size_t len);
void bb_rewind(byte_buffer_t *bb);
size_t bb_limit(byte_buffer_t *bb);
size_t bb_remain(byte_buffer_t *bb);
void bb_set_pos(byte_buffer_t *bb, uint32_t pos);
size_t bb_get_pos(byte_buffer_t *bb);

void bb_clear(byte_buffer_t *bb);

// Read functions
uint8_t bb_peek(byte_buffer_t *bb);
uint8_t *bb_buffer(byte_buffer_t *bb);
uint8_t *bb_point(byte_buffer_t *bb);
uint8_t bb_get(byte_buffer_t *bb);
uint8_t bb_get_at(byte_buffer_t *bb, uint32_t index);
void bb_get_bytes(byte_buffer_t *bb, uint8_t *dest, size_t len);
void bb_get_bytes_at(byte_buffer_t *bb, uint32_t index, uint8_t *dest, size_t len);
uint32_t bb_get_int(byte_buffer_t *bb);
uint32_t bb_get_int_at(byte_buffer_t *bb, uint32_t index);
uint16_t bb_get_short(byte_buffer_t *bb);
uint16_t bb_get_short_at(byte_buffer_t *bb, uint32_t index);

// Put functions (simply drop bytes until there is no more room)
void bb_put(byte_buffer_t *bb, uint8_t value);
void bb_put_at(byte_buffer_t *bb, uint8_t value, uint32_t index);
void bb_put_bytes(byte_buffer_t *bb, uint8_t *arr, size_t len);
void bb_put_bytes_at(byte_buffer_t *bb, uint8_t *arr, size_t len, uint32_t index);
void bb_put_int(byte_buffer_t *bb, uint32_t value);
void bb_put_int_at(byte_buffer_t *bb, uint32_t value, uint32_t index);
void bb_put_short(byte_buffer_t *bb, uint16_t value);
void bb_put_short_at(byte_buffer_t *bb, uint16_t value, uint32_t index);

// Wrap around an existing buf - will not copy buf
___INLINE byte_buffer_t *bb_new_from_buf(byte_buffer_t *bb, uint8_t *buf, size_t len) {
	bb->pos = 0;
	bb->len = len;
	bb->buf = buf;
	return bb;
}

// ����len�ֽ�
___INLINE void bb_skip(byte_buffer_t *bb, size_t len) {
	bb->pos += len;
}

// ��λpos
___INLINE void bb_rewind(byte_buffer_t *bb) {
	bb->pos = 0;
}

// ����С
___INLINE size_t bb_limit(byte_buffer_t *bb) {
	return bb->len;
}

// ʣ���С
___INLINE size_t bb_remain(byte_buffer_t *bb) {
	return bb->len - bb->pos;
}

// ��ǰλ��
___INLINE void bb_set_pos(byte_buffer_t *bb, uint32_t pos) {
	bb->pos = pos;
}

___INLINE size_t bb_get_pos(byte_buffer_t *bb) {
	return bb->pos;
}

// ���buf����, ͬʱ��λpos
___INLINE void bb_clear(byte_buffer_t *bb) {
	memset(bb->buf, 0, bb->len);
	bb->pos = 0;
}

// ��ȡ��ǰ�ֽ�, pos ������
___INLINE uint8_t bb_peek(byte_buffer_t *bb) {
	//return *(uint8_t*)(bb->buf+bb->pos);
	return bb->buf[bb->pos];
}

___INLINE uint8_t *bb_buffer(byte_buffer_t *bb) {
	return bb->buf;
}

// ȡ�õ�ǰָ��
___INLINE uint8_t *bb_point(byte_buffer_t *bb) {
	return bb->buf + bb->pos;
}

// ��ȡ��ǰ�ֽ�, pos+1
___INLINE uint8_t bb_get(byte_buffer_t *bb) {
	return bb->buf[bb->pos++];
}

// ��ȡָ��λ���ֽ�, pos ������
___INLINE uint8_t bb_get_at(byte_buffer_t *bb, uint32_t index) {
	return bb->buf[index];
}

___INLINE void bb_get_bytes(byte_buffer_t *bb, uint8_t *dest, size_t len) {
	memcpy(dest, bb->buf + bb->pos, len);
	bb->pos += len;
}

___INLINE void bb_get_bytes_at(byte_buffer_t *bb, uint32_t index, uint8_t *dest, size_t len) {
	memcpy(dest, bb->buf + index, len);
}

___INLINE uint32_t bb_get_int(byte_buffer_t *bb) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 24);
	ret |= (bb->buf[bb->pos++] << 16);
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 0);
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 16);
	ret |= (bb->buf[bb->pos++] << 24);
#endif

	return ret;
}

___INLINE uint32_t bb_get_int_at(byte_buffer_t *bb, uint32_t index) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 24);
	ret |= (bb->buf[index++] << 16);
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 0);
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 16);
	ret |= (bb->buf[index++] << 24);
#endif

	return ret;
}

___INLINE uint16_t bb_get_short(byte_buffer_t *bb) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 8);
	ret |= (bb->buf[bb->pos++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[bb->pos++] << 0);
	ret |= (bb->buf[bb->pos++] << 8);
#endif

	return ret;
}

___INLINE uint16_t bb_get_short_at(byte_buffer_t *bb, uint32_t index) {
#ifdef LITTLE_EDIAN
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 8);
	ret |= (bb->buf[index++] << 0);
#else
	uint32_t ret = 0;
	ret |= (bb->buf[index++] << 0);
	ret |= (bb->buf[index++] << 8);
#endif

	return ret;
}

// Relative write of the entire contents of another ByteBuffer (src)
___INLINE void bb_put(byte_buffer_t *bb, uint8_t value) {
	bb->buf[bb->pos++] = value;
}

___INLINE void bb_put_at(byte_buffer_t *bb, uint8_t value, uint32_t index) {
	bb->buf[index] = value;
}

___INLINE void bb_put_bytes(byte_buffer_t *bb, uint8_t *arr, size_t len) {
	memcpy (bb->buf + bb->pos, arr, len);
	bb->pos += len;
}

___INLINE void bb_put_bytes_at(byte_buffer_t *bb, uint8_t *arr, size_t len, uint32_t index) {
	memcpy (bb->buf + index, arr, len);
}

___INLINE void bb_put_int(byte_buffer_t *bb, uint32_t value) {
#ifdef LITTLE_EDIAN
	bb->buf[bb->pos++] = (value >> 24) & 0xFF;
	bb->buf[bb->pos++] = (value >> 16) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
#else
	bb->buf[bb->pos++] = (value >> 0 ) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 16) & 0xFF;
	bb->buf[bb->pos++] = (value >> 24) & 0xFF;
#endif
}

___INLINE void bb_put_int_at(byte_buffer_t *bb, uint32_t value, uint32_t index) {
#ifdef LITTLE_EDIAN
	bb->buf[index++] = (value >> 24) & 0xFF;
	bb->buf[index++] = (value >> 16) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 0) & 0xFF;
#else
	bb->buf[index++] = (value >> 0 ) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 16) & 0xFF;
	bb->buf[index++] = (value >> 24) & 0xFF;
#endif
}

___INLINE void bb_put_short(byte_buffer_t *bb, uint16_t value) {
#ifdef LITTLE_EDIAN
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
#else
	bb->buf[bb->pos++] = (value >> 0) & 0xFF;
	bb->buf[bb->pos++] = (value >> 8) & 0xFF;
#endif
}

___INLINE void bb_put_short_at(byte_buffer_t *bb, uint16_t value, uint32_t index) {
#ifdef LITTLE_EDIAN
	bb->buf[index++] = (value >> 8) & 0xFF;
	bb->buf[index++] = (value >> 0) & 0xFF;
#else
	bb->buf[index++] = (value >> 0) & 0xFF;
	bb->buf[index++] = (value >> 8) & 0xFF;
#endif
}

#endif
