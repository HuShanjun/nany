/**@author 徐飞 2008-03-27
*/
#pragma once

#include <stdexcept>
#include <utility>

namespace Gamma
{
    namespace Private
    {
        /// 移除类型引用模板
        template<typename T>
        struct RemoveReference
        {
            typedef T Result;
        };

        template<typename T>
        struct RemoveReference<T&>
        {
            typedef T Result;
        };

        template<typename T>
        struct RemoveReference<const T&>
        {
            typedef T Result;
        };
    }

    /// 能接受任意类型的“Any”对象
    class Any
    {
    public:
        Any()
            : m_pContent(0)
        {
        }

        ~Any()
        {
            delete m_pContent;
        };

        template<typename T>
        Any(const T& value)
            : m_pContent(new Holder<T>(value))
        {
        }

        Any(const Any& rhs)
            : m_pContent(rhs.m_pContent?rhs.m_pContent->Clone():0)
        {
        }

        Any& Swap(Any& rhs)
        {
            std::swap(m_pContent, rhs.m_pContent);
            return *this;
        }

        template<typename ValueType>
        Any& operator=(const ValueType& rhs)
        {
            Any(rhs).Swap(*this);
            return *this;
        }

        Any& operator=(const Any& rhs)
        {
            Any(rhs).Swap(*this);
            return *this;
        }

        const std::type_info& Type() const
        {
            return m_pContent ? m_pContent->Type() : typeid(void);
        };

        bool Empty() const
        {
            return m_pContent == 0;
        }

    private:
        class PlaceHolder
        {
        public:            
            virtual ~PlaceHolder(){}
            virtual PlaceHolder* Clone() const = 0;
            virtual const std::type_info& Type() const = 0;
        };

        template<typename T>
        class Holder
            : public PlaceHolder
        {
        public:
            Holder(const T& val)
                : m_value(val)
            {                
            }

            virtual PlaceHolder* Clone() const
            {
                return new Holder(m_value);
            }

            virtual const std::type_info& Type() const
            {
                return typeid(m_value);
            }

        public:
            T m_value;
        };

        PlaceHolder* m_pContent;
        
        template<typename ValueType>
        friend ValueType * any_cast(Any * operand);
    };

    /// any_cast函数不同版本
    template<typename ValueType>
    ValueType* any_cast(Any* operand)
    {
        return operand && operand->Type() == typeid(ValueType)
            ? &static_cast<Any::Holder<ValueType> *>(operand->m_pContent)->m_value
            : 0;
    }

    template<typename ValueType>
    const ValueType* any_cast(const Any* operand)
    {
        return any_cast<ValueType>(const_cast<Any*>(operand));
    }

    template<typename ValueType>
    ValueType any_cast(const Any & operand)
    {
        typedef typename Private::RemoveReference<ValueType>::Result NoneRefType;

        const NoneRefType * result = any_cast<NoneRefType>(&operand);
        if(!result)
			GammaThrow( L"BadAnyCastException: failed conversion using BadAnyCastException" );

        return *result;
    }

    template<typename ValueType>
    ValueType any_cast(Any & operand)
    {
        typedef typename Private::RemoveReference<ValueType>::Result NoneRefType;

        NoneRefType * result = any_cast<NoneRefType>(&operand);
		if(!result)
			GammaThrow( L"BadAnyCastException: failed conversion using BadAnyCastException" );
        
        return *result;
    }
}
