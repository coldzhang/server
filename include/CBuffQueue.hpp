#ifndef __CBUFFQUEUE_H__
#define __CBUFFQUEUE_H__
#include "baseHeader.h"
#include "../Thread/Mutex.h"

template<typename T>
class CBuffQueue
{
public:
    CBuffQueue():
        m_pData(NULL),
        m_pHead(NULL),
        m_pTail(NULL),
        m_nLength(0),
        m_nSize(0),
        m_nExtraSize(0)
    {
    }

    ~CBuffQueue()
    {
        delete[] m_pData;
        m_pData = NULL;
    }
    void clear()
    {
        m_pHead = m_pData;
        m_pTail = m_pData;
        m_nLength = 0;
        m_nSize = 0;
        m_nExtraSize = 0;
    }

    bool init(int32 size, int32 extraSize)
    {
        m_pData = new T[size + extraSize];
        if (NULL != m_pData)
        {
            m_pHead = m_pData;
            m_pTail = m_pData;
            m_nLength = 0;
            m_nSize = size;
            m_nExtraSize = extraSize;
            return true;
        }
        return false;
    }

    int32 pushMsg(T *target, int32 size)
    {
        if (calcFreeSpace() < size)
        {
            printf("message too long...");
            return -1;
        }
        AutoLock qlock(&m_mutex);
        if (target != NULL)
        {
            if (m_pHead < m_pTail)
            {
                int32 backSize = m_nSize - ((m_pTail - m_pData) / sizeof(T));
                if (backSize <= size)
                {
                    memcpy(m_pTail, target, size * sizeof(T));
                }
                else
                {
                    memcpy(m_pTail, target, backSize*sizeof(T));
                    memcpy(m_pTail, target + backSize, (size - backSize) * sizeof(T));
                }
            }
            else
            {
                memcpy(m_pTail, target, size * sizeof(T));
            }
        }
        
        m_pTail += size * sizeof(T);
        m_pTail = m_pData + ((m_pTail - m_pData) % m_nSize);
        m_nLength += size;
        return size;
    }

    int32 popMsg(T *des, int32 size)
    {
        AutoLock qlock(&m_mutex);
        
        if (m_nLength < size)
        {
            return -1;
        }
        if (NULL != des)
        {
            if (m_pHead < m_pTail)
            {
                memcpy(des, m_pHead, size * sizeof(T));
            }
            else
            {
                int32 leftSize = m_nSize - (m_pHead - m_pData) / sizeof(T);
                if (leftSize > size)
                {
                    memcpy(des, m_pHead, size * sizeof(T));
                }
                else
                {
                    memcpy(des, m_pHead, leftSize * sizeof(T));
                    memcpy(des, m_pData, (size - leftSize) * sizeof(T));
                }
            }
        }
        
        int32 HeadSize = ((m_pHead + size - m_pData) / sizeof(T)) % m_nSize;
        m_pHead = m_pData + HeadSize;
        m_nLength -= size;
        return size;
    }
    

    inline int32 calcFreeSpace()
    {
        AutoLock qlock(&m_mutex);
        return (m_nSize - m_nLength);
    }

    inline T* getReadPtr(int32 copySize) // if the backmsg is truncate into two parts, copy "copySize" memory from the head of buffqueue
    {
        AutoLock qlock(&m_mutex);
        T *ret = m_pHead;
        if (getReadableLen() < copySize)
        {
            memcpy(m_pData + m_nSize, m_pData, copySize);
        }
        
        return ret;
    }

    inline T* getWritePtr()
    {
        AutoLock qlock(&m_mutex);
        return m_pTail;
    }
    
    inline int32 getBufLen()
    {
        AutoLock qlock(&m_mutex);
        return m_nLength;
    }
    
    inline int32 getReadableLen()
    {
        AutoLock qlock(&m_mutex);
        if (m_pHead == m_pTail) return (getBufLen() > 0 ? getBufLen() : 0);
        else if (m_pHead < m_pTail) return (m_pTail - m_pHead) / sizeof(T);
        else return m_nSize - (m_pHead - m_pData) / sizeof(T);  // just return backmem size
    }

    inline int32 getWriteableLen()
    {
        AutoLock qlock(&m_mutex);
        if (m_pHead == m_pTail)
        {
            return (getBufLen() > 0 ? 0 : m_nSize - (m_pTail - m_pData) / sizeof(T));
        }
        else if (m_pHead < m_pTail)
        {
            return (m_pTail - m_pHead) / sizeof(T);
        }
        else
        {
            return m_nSize - (m_pTail - m_pData) / sizeof(T);
        }
    }
protected:
    CBuffQueue(CBuffQueue &queue)
    {
    }
    CBuffQueue& operator=(CBuffQueue &queue)
    {
    }
private:
    T *m_pData;
    T *m_pHead;
    T *m_pTail;
    int32 m_nLength;
    int32 m_nSize;
    int32 m_nExtraSize;
    CMutex m_mutex;
};
#endif 
