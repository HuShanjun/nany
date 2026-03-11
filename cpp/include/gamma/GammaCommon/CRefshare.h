#pragma once
#include "GammaCommon/CAtomicOperation.h"
#include "GammaCommon/CNullpointException.h"
#include "GammaCommon/CGammaObject.h"

typedef void (*FreeRefFunc)(void *);

template <class T>
class CRefObject {
protected:
    T *m_pRefObj;
    int m_nRefKey = 0;

public:
    CRefObject() : m_pRefObj(nullptr) {
    }

    CRefObject(const CRefObject &Right) {
        m_pRefObj = Right.m_pRefObj;
        if (m_pRefObj != nullptr) {
            m_pRefObj->incRef();
            m_nRefKey = m_pRefObj->getRefKey();
        }
    }

    CRefObject(T *pObj) {
        m_pRefObj = (T *)pObj;
        if (m_pRefObj != nullptr) {
            m_pRefObj->incRef();
            m_nRefKey = m_pRefObj->getRefKey();
        }
    }

    ~CRefObject() {
        if (m_pRefObj != nullptr) {
            if (m_pRefObj->decRef()) {
                m_pRefObj = nullptr;
                m_nRefKey = 0;
            }
        }
    }

    CRefObject &operator=(const CRefObject &Right) {
        if (this == &Right)
            return *this;

        if (m_pRefObj != nullptr)
            m_pRefObj->decRef();
        m_pRefObj = Right.m_pRefObj;
        if (m_pRefObj != nullptr) {
            m_pRefObj->incRef();
            m_nRefKey = m_pRefObj->getRefKey();
        }
        return *this;
    }

    CRefObject &operator=(T *pPointer) {
        if (m_pRefObj != nullptr)
            m_pRefObj->decRef();
        m_pRefObj = pPointer;
        if (m_pRefObj != nullptr) {
            m_pRefObj->incRef();
            m_nRefKey = m_pRefObj->getRefKey();
        }
        return *this;
    }

    operator void *() const {
        return m_pRefObj;
    }

    // 指针类都不能改写取值函数，如果改写了，会导致无法放入容器中
    // T* operator&()
    // {
    // 	return m_pRefObj;
    // }

    // const T* operator&() const
    // {
    // 	return m_pRefObj;
    // }

    bool IsValid() const {
        if (m_pRefObj == nullptr || m_nRefKey != m_pRefObj->getRefKey()) {
            return false;
        }
        return true;
    }

    T *operator->() {
        // 空指针抛异常
        if (!this->IsValid()) {
            // string stack;
            // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
            // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
            ThrowException<CNullPointException>("Null Point Exception");
        }
        return m_pRefObj;
    }

    const T *operator->() const {
        // 空指针抛异常
        if (!this->IsValid()) {
            // string stack;
            // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
            // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
            ThrowException<CNullPointException>("Null Point Exception");
        }
        return m_pRefObj;
    }

    T *get() {
        return m_pRefObj;
    }

    T *get() const {
        return m_pRefObj;
    }

    static CRefObject<T> createInstance() {
        return CRefObject<T>(new T());
    }

    /*
     *强制释放对象
     */
    void release(void) {
        if (!this->IsValid()) {
            return;
        } else {
            m_pRefObj->release();
            m_pRefObj = nullptr;
            m_nRefKey = 0;
        }
    }
};

/*
引用计数对象，用于只能对象的删除
*/
class GAMMA_COMMON_API CRefShare : public Gamma::CGammaObject {
private:
    atomic_t m_nRefCount;
    int m_nRefKey = 0;

public:
    CRefShare();
    CRefShare(const CRefShare &Right);
    CRefShare &operator=(const CRefShare &Right);
    virtual ~CRefShare();

    void incRef();
  
    bool decRef();

    virtual bool decRefEx(FreeRefFunc pFreeFunc);

    //如果当前的实例是一个栈对象，必须需要调用此函数。防止作为参数传递过程中被释放资源
    void setStackObject();

   	//强制释放自身资源
    void release(FreeRefFunc pFreeFunc = nullptr);

    int getRefKey() const;
    static int genRefKey();

    // void* operator new(size_t size)
    //{
    //	return Gamma::GammaAlloc(size, ((void**)&size)[-1]);
    // }

    // void operator delete(void* p) throw()
    //{
    //	return Gamma::GammaFree(p);
    // }
};

typedef CRefObject<CRefShare> CRefSharePtr;

/*
对普通的指针进行引用计数保护
*/
template <class C>
class TRefPointer : public CRefShare {
private:
    template <class T>
    class CInnerPointer : virtual public CRefShare {
    private:
        CInnerPointer(CInnerPointer &Right);
        CInnerPointer &operator=(CInnerPointer &Right);

    protected:
        friend class TRefPointer;
        T *m_pPointer;

    public:
        CInnerPointer() : m_pPointer(nullptr) {
        }

        CInnerPointer(T *pObj) {
            m_pPointer = pObj;
        }

        ~CInnerPointer() {
            delete m_pPointer;
        }

        T *operator->() {
            // 空指针抛异常
            if (m_pPointer == nullptr) {
                // string stack;
                // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
                // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
                ThrowException<CNullPointException>("Null Point Exception");
            }
            return m_pPointer;
        }

        const T *operator->() const {
            // 空指针抛异常
            if (m_pPointer == nullptr) {
                // string stack;
                // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
                // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
                ThrowException<CNullPointException>("Null Point Exception");
            }
            return m_pPointer;
        }

        // 指针类都不能改写取值函数，如果改写了，会导致无法放入容器中
        //	T* operator &()
        //	{
        //		return m_pPointer;
        //	}

        operator void *() const {
            return m_pPointer;
        }
    };

protected:
    CInnerPointer<C> *m_pRefObj;

public:
    TRefPointer() : m_pRefObj(nullptr) {
    }

    TRefPointer(const TRefPointer &Right) {
        m_pRefObj = Right.m_pRefObj;
        if (m_pRefObj != nullptr)
            m_pRefObj->incRef();
    }

    TRefPointer(C *pObj) {
        if (pObj != nullptr) {
            m_pRefObj = new CInnerPointer<C>(pObj);
            m_pRefObj->incRef();
        } else
            m_pRefObj = nullptr;
    }

    ~TRefPointer() {
        if (m_pRefObj != nullptr) {
            if (m_pRefObj->decRef())
                m_pRefObj = nullptr;
        }
    }

    TRefPointer &operator=(const TRefPointer &Right) {
        if (this == &Right) {
            return *this;
        }
        if (m_pRefObj != nullptr)
            m_pRefObj->decRef();
        m_pRefObj = Right.m_pRefObj;
        if (m_pRefObj != nullptr)
            m_pRefObj->incRef();
        return *this;
    }

    operator void *() const {
        if (m_pRefObj == nullptr)
            return nullptr;
        return m_pRefObj->operator void *();
    }

    C *operator->() {
        // 空指针抛异常
        if (m_pRefObj == nullptr) {
            // string stack;
            // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
            // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
            ThrowException<CNullPointException>("Null Point Exception");
        }
        return m_pRefObj->operator->();
    }

    const C *operator->() const {
        // 空指针抛异常
        if (m_pRefObj == nullptr) {
            // string stack;
            // CGlobal::getInstance()->m_pSystem->GetLoger()->GetCurrentCallStack(stack, 50);
            // MODULE_LOGFILE2_ERROR("nullpoint", "Null Point Exception, static=%", stack);
            ThrowException<CNullPointException>("Null Point Exception");
        }
        return m_pRefObj->operator->();
    }

    // 指针类都不能改写取值函数，如果改写了，会导致无法放入容器中
    //	C* operator &()
    //	{
    //		return m_pRefObj->operator &();
    //	}
};

/*
由于引用计数指针和智能指针改写了取址函数
所以不能直接放入list中，需要修改为此对象保存
*/
template <class T>
class CSTLRefObject {
private:
    CSTLRefObject();
    CRefObject<T> m_Ref;

public:
    CSTLRefObject(const CRefObject<T> &Other) {
        m_Ref = Other;
    }

    CSTLRefObject(T *pOther) : m_Ref(pOther) {
    }

    CRefObject<T> &get() {
        return m_Ref;
    }

    const CRefObject<T> &get() const {
        return m_Ref;
    }

    operator void *() const {
        return m_Ref;
    }
};
