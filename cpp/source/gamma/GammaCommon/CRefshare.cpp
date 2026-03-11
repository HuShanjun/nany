#include "GammaCommon/CRefshare.h"

/*

        Class	CRefShare

*/
void CRefShare::setStackObject() {
    incRef();
}

bool CRefShare::decRef() {
    return decRefEx(NULL);
}

CRefShare &CRefShare::operator=(const CRefShare &Right) {
    return *this;
}
void CRefShare::release(FreeRefFunc pFreeFunc) {
    atomic_set(&m_nRefCount, 1);
    decRefEx(pFreeFunc);
}

int CRefShare::genRefKey() {
    static int s_RefKey;
    return s_RefKey++;
}

int CRefShare::getRefKey() const {
    return m_nRefKey;
}

#if defined(HAS_MULTITHREAD)
CRefShare::CRefShare() {
    atomic_set(&m_nRefCount, 0);
    m_nRefKey = CRefShare::genRefKey();
}

CRefShare::CRefShare(const CRefShare &Right) {
    atomic_set(&m_nRefCount, 0);
    m_nRefKey = CRefShare::genRefKey();
}

CRefShare::~CRefShare() {
    m_nRefKey = 0;
}

void CRefShare::incRef() {
    atomic_inc(&m_nRefCount);
}

bool CRefShare::decRefEx(FreeRefFunc pFreeFunc) {
    if (atomic_dec_and_test(&m_nRefCount)) {
        if (pFreeFunc)
            (*pFreeFunc)(this);
        else
            delete this;
        return true;
    }
    return false;
}
#else

CRefShare::CRefShare() {
    m_nRefCount = 0;
    m_nRefKey = CRefShare::genRefKey();
}

CRefShare::~CRefShare() {
    m_nRefKey = 0;
}

CRefShare::CRefShare(const CRefShare &Right) {
    m_nRefCount = 0;
    m_nRefKey = CRefShare::genRefKey();
}

void CRefShare::incRef() {
    m_nRefCount++;
}

bool CRefShare::decRefEx(FreeRefFunc pFreeFunc) {
    if (--m_nRefCount == 0) {
        if (pFreeFunc)
            (*pFreeFunc)(this);
        else
            delete this;
        return true;
    }
    return false;
}
#endif
