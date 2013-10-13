//
// Copyright (c) 2012-2013 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#if !BOOST_PP_IS_ITERATING

#include <utility>
#include <boost/type.hpp>
#include <boost/shared_ptr.hpp>

#include "boost/di/type_traits/ctor_traits.hpp"

namespace boost {
namespace di {

template<bool = false>
struct priority_impl
{ };

template<>
struct priority_impl<true>
{
    typedef void is_priority;
};

struct fake_scope_entry { };
struct fake_scope_exit { };

template<bool Priority = false>
struct fake_scope : priority_impl<Priority>
{
    template<typename T>
    class convertible
    {
    public:
        convertible(shared_ptr<T> obj) // non explicit
            : obj_(obj)
        { }

        template<typename I>
        shared_ptr<I> convert(const type<shared_ptr<I>>&) const {
            return obj_;
        }

        template<typename I>
        I convert(const type<I>&) const {
            return *obj_;
        }

        operator T() const {
            return convert(type<T>());
        }

    private:
        shared_ptr<T> obj_;
    };

    template<typename T, typename>
    struct scope
    {
        typedef convertible<T> result_type;

        void call(const fake_scope_entry&) {
            entry_calls()++;
        }

        void call(const fake_scope_exit&) {
            exit_calls()++;
        }

        template<typename... Args>
        result_type create(Args&&... args) {
            using ctor = typename type_traits::ctor_traits<T>::type;
            if (entry_calls() > exit_calls()) {
                return shared_ptr<T>(new T(std::forward<Args>(args)...));
            }

            return result_type(shared_ptr<T>());
        }
    };

    static int& entry_calls() {
        static int calls;
        return calls;
    }

    static int& exit_calls() {
        static int calls;
        return calls;
    }
};

} // namespace di
} // namespace boost

#endif

