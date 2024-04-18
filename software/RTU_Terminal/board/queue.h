
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <rtdef.h>

// ԭ��, ������ʹ��
typedef struct {
    rt_base_t nFront;       /* ͷָ�� */
    rt_base_t nRear;        /* βָ�룬�����в��գ�ָ�����βԪ�ص���һ��λ�� */
    rt_uint8_t pData[0];
} BaseQueue_t;

//�궨�����Ч��
// read ��emptyʱ������
// read ����, get ������
// get ǰ��ȷ�����㹻Ԫ��
// remove ͷָ�����1

// write ��fullʱ������Ԫ��
// push ��fullʱ��remove��write
#define nQueueSize(_q) ( sizeof( _q.pData ) / sizeof( _q.pData[0] ) )
#define bQueueFull(_q) (((_q.nRear + 1) % (nQueueSize(_q)) == _q.nFront) ? RT_TRUE : RT_FALSE)
#define bQueueEmpty(_q) ((_q.nFront == _q.nRear) ? RT_TRUE : RT_FALSE)
#define bQueueRead(_q, _e) ((_q.nFront == _q.nRear) ? RT_FALSE : ((_e = _q.pData[_q.nFront]), (_q.nFront = (_q.nFront + 1) % (nQueueSize(_q))), (RT_TRUE)))
#define bQueueRemove(_q) ((_q.nFront == _q.nRear) ? RT_FALSE : ((_q.nFront = (_q.nFront + 1) % (nQueueSize(_q))), (RT_TRUE)))
#define vQueueClear(_q) (_q.nFront = _q.nRear = 0)
#define xQueueGet(_q, _index) (_q.pData[(_q.nFront + (_index)) % (nQueueSize(_q))])
#define bQueueWrite(_q, _e) ((_q.nRear + 1) % (nQueueSize(_q)) == _q.nFront ? RT_FALSE : ((_q.pData[_q.nRear] = _e), (_q.nRear = (_q.nRear + 1) % (nQueueSize(_q))), (RT_TRUE)))
#define bQueuePush(_q, _e) ((_q.nRear + 1) % (nQueueSize(_q)) == _q.nFront ? (bQueueRemove(_q), bQueueWrite(_q, _e)) : (bQueueWrite(_q, _e), (RT_TRUE)))
#define nQueueLen(_q) ((_q.nRear - _q.nFront + nQueueSize(_q)) % nQueueSize(_q))
#define bQueueOverHalf(_q) ((nQueueLen(_q)>=(nQueueSize(_q)>>1))? RT_TRUE : RT_FALSE)

#endif

