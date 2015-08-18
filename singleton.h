#pragma once
#include <windows.h>

#define SINGLETON_TEMPLATE_DECLARE(theClass) friend class y::Singleton<theClass>
#define SINGLETON_DECLARE(theClass,func)                \
    public:                                             \
    static theClass& func()                             \
    {                                                   \
        static theClass* s_ptr = 0;                     \
        static volatile LONG s_lock = 0;                \
        while (!s_ptr)                                  \
        {                                               \
            if (1 == ::InterlockedIncrement(&s_lock))   \
                {                                       \
                    s_ptr = _obj_creator();             \
                    ::InterlockedDecrement(&s_lock);    \
                }                                       \
                else                                    \
                {                                       \
                    ::InterlockedDecrement(&s_lock);    \
                    ::Sleep(10);                        \
                }                                       \
        }                                               \
        return *s_ptr;                                  \
    }                                                   \
    static theClass* _obj_creator()                     \
    {                                                   \
        static theClass s_obj;                          \
        return &s_obj;                                  \
    }


namespace cu // cu -> common utils
{

template<class T>
class Singleton
{
    typedef Singleton<T> _Myt;

public:
    static T& instance()
    {
        static T obj;
        m_oc.do_nothing();
        return obj;
    }

private:
    struct _obj_creator
    {
        _obj_creator() 
        {
            _Myt::instance(); 
        }
        inline void do_nothing() const {}
    };

private:
    static _obj_creator m_oc;
};

template<typename T> typename Singleton<T>::_obj_creator Singleton<T>::m_oc;

} // end of namespace cu
