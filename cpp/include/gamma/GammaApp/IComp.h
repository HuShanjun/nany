#pragma once

#include <cstdint>
#include <vector>

#define DEFINE_COMP_ID(name, EnumType)                                                             \
    static constexpr uint8_t GetID() {                                                             \
        return static_cast<uint8_t>(EnumType::eACID_##name);                                       \
    }                                                                                              \
    static constexpr const char *GetName() {                                                       \
        return #name;                                                                              \
    }

class IComp {
public:
    IComp(const IComp &) = delete;
    IComp(IComp &&) = delete;
    IComp &operator=(const IComp &) = delete;
    IComp &operator=(IComp &&) = delete;
    virtual ~IComp() {};

    virtual void OnInit() = 0;
    virtual void OnCleanup() = 0;

    virtual void OnStarted() = 0;
    virtual void OnQuit() = 0;
};

class CompMgr {
public:
    CompMgr() {}

    virtual ~CompMgr() {
        for (auto pComp : m_vecComp) {
            if (pComp) {
                delete pComp;
            }
        }
        m_vecComp.clear();
    };

    template <typename T, typename... Args>
    T *AddComp(Args &&...args) {
        T *pComp = new T(std::forward<Args>(args)...);
        AddComp(pComp);
        return pComp;
    }

    IComp *AddComp(uint8_t nCompID, IComp *pComp) {
        if (nCompID >= m_vecComp.size()) {
            m_vecComp.resize(nCompID + 1);
        }
        m_vecComp[nCompID] = pComp;
        return pComp;
    }

    void DelComp(uint8_t nCompID) {
        if (nCompID < m_vecComp.size()) {
            m_vecComp[nCompID] = nullptr;
        }
    }

    IComp *GetComp(uint8_t nCompID) {
        if (nCompID < m_vecComp.size()) {
            return m_vecComp[nCompID];
        }
        return nullptr;
    }

    template <typename T>
    void DelComp() {
        DelComp(T::GetID());
    }

    template <typename T>
    T *GetComp() {
        return dynamic_cast<T *>(GetComp(T::GetID()));
    }

    void OnInit() {
        for (auto pComp : m_vecComp) {
            if (pComp) {
                pComp->OnInit();
            }
        }
    }

    void OnCleanup() {
        for (auto pComp : m_vecComp) {
            if (pComp) {
                pComp->OnCleanup();
            }
        }
    }

    void OnStarted() {
        for (auto pComp : m_vecComp) {
            if (pComp) {
                pComp->OnStarted();
            }
        }
    }

    void OnQuit() {
        for (auto pComp : m_vecComp) {
            if (pComp) {
                pComp->OnQuit();
            }
        }
    }

private:
    std::vector<IComp *> m_vecComp;
};