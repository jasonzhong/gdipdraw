#pragma once

namespace gdipdraw
{
    class CLock
    {
    public:
        CLock()
        {
            ::InitializeCriticalSection(&m_Lock);
        }
        ~CLock()
        {
            ::DeleteCriticalSection(&m_Lock);
        }

        void Init() {
            ::EnterCriticalSection(&m_Lock);
        }

        void Del() {
            ::LeaveCriticalSection(&m_Lock);
        }
    private:
        CRITICAL_SECTION m_Lock;
    }; 

    class CLockContainer
    {
    public:
        CLockContainer(CLock* pCritical)
            : m_pCritical(pCritical)
        {
            if(m_pCritical) {
                m_pCritical->Init();
            }
        }
        ~CLockContainer()
        {
            if(m_pCritical) {
                m_pCritical->Del();
            }
        }
    private:
        CLock* m_pCritical;
    }; 
}