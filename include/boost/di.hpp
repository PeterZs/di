//
// Copyright (c) 2012-2015 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_HPP
#define BOOST_DI_HPP

#if (__cplusplus < 201305L && _MSC_VER < 1900)
   #error "C++14 is required by Boost.DI"
#endif

#if defined(BOOST_DI_CFG_NO_PREPROCESSED_HEADERS)

// config
#include "boost/di/config.hpp"

// bindings
#include "boost/di/bindings.hpp"

// injections
#include "boost/di/inject.hpp"
#include "boost/di/injector.hpp"
#include "boost/di/make_injector.hpp"

// scopes
#include "boost/di/scopes/deduce.hpp"
#include "boost/di/scopes/external.hpp"
#include "boost/di/scopes/exposed.hpp"
#include "boost/di/scopes/singleton.hpp"
#include "boost/di/scopes/unique.hpp"

// policies
#include "boost/di/policies/constructible.hpp"

// providers
#include "boost/di/providers/heap.hpp"
#include "boost/di/providers/stack_over_heap.hpp"

#else

#include <memory>
#include <type_traits>

#ifndef BOOST_DI_AUX_UTILITY_HPP
#define BOOST_DI_AUX_UTILITY_HPP

namespace boost { namespace di { inline namespace v1 {

struct _ { _(...) { } };

namespace aux {

template<class...>
struct type { };

struct none_type { };

template<class T, T>
struct non_type { };

template<class...>
struct void_t { using type = void; };

template<class...>
struct always : std::true_type { };

template<class...>
struct never : std::false_type { };

template<class, class>
struct pair { using type = pair; };

template<bool...>
struct bool_list { using type = bool_list; };

template<class...>
struct type_list { using type = type_list; };

template<class... Ts>
struct inherit : Ts... { using type = inherit; };

template<class...>
struct join;

template<>
struct join<> { using type = type_list<>; };

template<class... TArgs>
struct join<type_list<TArgs...>> {
    using type = type_list<TArgs...>;
};

template<class... TArgs1, class... TArgs2>
struct join<type_list<TArgs1...>, type_list<TArgs2...>> {
    using type = type_list<TArgs1..., TArgs2...>;
};

template<class... TArgs1, class... TArgs2, class... Ts>
struct join<type_list<TArgs1...>, type_list<TArgs2...>, Ts...> {
    using type = typename join<type_list<TArgs1..., TArgs2...>, Ts...>::type;
};

template<class... TArgs>
using join_t = typename join<TArgs...>::type;

template<class, class...>
struct is_unique_impl;

template<class...>
struct not_unique : std::false_type {
    using type = not_unique;
};

template<>
struct not_unique<> : std::true_type {
    using type = not_unique;
};

template<class T>
struct is_unique_impl<T> : not_unique<> { };

template<class T1, class T2, class... Ts>
struct is_unique_impl<T1, T2, Ts...>
    : std::conditional_t<
          std::is_base_of<type<T2>, T1>::value
        , not_unique<T2>
        , is_unique_impl<inherit<T1, type<T2>>, Ts...>
      >
{ };

template<class... Ts>
using is_unique = is_unique_impl<none_type, Ts...>;

}}}} // boost::di::v1::aux

#endif

#ifndef BOOST_DI_FWD_HPP
#define BOOST_DI_FWD_HPP

namespace boost {

template<class> class shared_ptr;

namespace di { inline namespace v1 {

class config;
template<class...> class injector;
struct no_name { const char* operator()() const noexcept { return nullptr; } };
namespace providers { class heap; class stack_over_heap; } // providers
namespace core { template<class> struct any_type_fwd; template<class> struct any_type_ref_fwd; }

}}} // boost::di::v1

#endif

#ifndef BOOST_DI_AUX_TYPE_TRAITS_HPP
#define BOOST_DI_AUX_TYPE_TRAITS_HPP

#define BOOST_DI_HAS_TYPE(name)                                     \
    template<class, class = void>                                   \
    struct has_##name : std::false_type { };                        \
                                                                    \
    template<class T>                                               \
    struct has_##name<                                              \
        T, typename aux::void_t<typename T::name>::type             \
    > : std::true_type { }

#define BOOST_DI_HAS_METHOD(name, call_name)                        \
    template<class T, class... TArgs>                               \
    decltype(std::declval<T>().call_name(std::declval<TArgs>()...)  \
           , std::true_type())                                      \
    has_##name##_impl(int);                                         \
                                                                    \
    template<class, class...>                                       \
    std::false_type has_##name##_impl(...);                         \
                                                                    \
    template<class T, class... TArgs>                               \
    struct has_##name : decltype(has_##name##_impl<T, TArgs...>(0)) \
    { }

#define BOOST_DI_REQUIRES(...) \
    typename std::enable_if<__VA_ARGS__, int>::type = 0

#define BOOST_DI_REQUIRES_T(...) \
    std::enable_if_t<__VA_ARGS__>

#define BOOST_DI_REQUIRES_MSG(...) \
    typename constraint_not_satisfied<__VA_ARGS__>::type = 0

#define BOOST_DI_REQUIRES_MSG_T(...) \
    constraint_not_satisfied<__VA_ARGS__>::type

namespace boost { namespace di { inline namespace v1 {

template<class...>
struct constraint_not_satisfied { };

template<>
struct constraint_not_satisfied<std::true_type> {
    using type = int;
};

template<class T>
struct constraint_not_satisfied<std::true_type, T> {
    using type = T;
};

namespace aux {

template<class...>
using is_valid_expr = std::true_type;

template<class>
struct is_smart_ptr : std::false_type { };

template<class T, class TDeleter>
struct is_smart_ptr<std::unique_ptr<T, TDeleter>>
    : std::true_type
{ };

template<class T>
struct is_smart_ptr<std::shared_ptr<T>>
    : std::true_type
{ };

template<class T>
struct is_smart_ptr<boost::shared_ptr<T>>
    : std::true_type
{ };

template<class T>
struct is_smart_ptr<std::weak_ptr<T>>
    : std::true_type
{ };

template<class T, class... TArgs>
decltype(void(T{std::declval<TArgs>()...}), std::true_type{})
test_is_braces_constructible(int);

template<class, class...>
std::false_type test_is_braces_constructible(...);

template<class T, class... TArgs>
using is_braces_constructible =
    decltype(test_is_braces_constructible<T, TArgs...>(0));

template<class T, class... TArgs>
using is_braces_constructible_t =
    typename is_braces_constructible<T, TArgs...>::type;

template<class T>
using remove_accessors =
    std::remove_cv<std::remove_pointer_t<std::remove_reference_t<T>>>;

template<class T>
using remove_accessors_t = typename remove_accessors<T>::type;

template<class, class = void>
struct deref_type;

template<typename T>
using deref_type_t = typename deref_type<T>::type;

template<class T, class>
struct deref_type {
    using type = T;
};

template<class T>
struct deref_type<T, std::enable_if_t<is_smart_ptr<T>::value>> {
    using type = typename T::element_type;
};

template<class T>
using decay =
    deref_type<remove_accessors_t<deref_type_t<remove_accessors_t<T>>>>;

template<class T>
using decay_t = typename decay<T>::type;

template<class T1, class T2>
struct is_same_or_base_of {
    static constexpr auto value =
        std::is_same<aux::decay_t<T1>, aux::decay_t<T2>>::value ||
        std::is_base_of<aux::decay_t<T2>, aux::decay_t<T1>>::value;
};

template<class>
struct function_traits;

template<class R, class... TArgs>
struct function_traits<R(*)(TArgs...)> {
    using result_type = R;
    using base_type = none_type;
    using args = type_list<TArgs...>;
};

template<class R, class... TArgs>
struct function_traits<R(TArgs...)> {
    using result_type = R;
    using base_type = none_type;
    using args = type_list<TArgs...>;
};

template<class R, class T, class... TArgs>
struct function_traits<R(T::*)(TArgs...)> {
    using result_type = R;
    using base_type = T;
    using args = type_list<TArgs...>;
};

template<class R, class T, class... TArgs>
struct function_traits<R(T::*)(TArgs...) const> {
    using result_type = R;
    using base_type = T;
    using args = type_list<TArgs...>;
};

}}}} // boost::di::v1::aux

#endif

#ifndef BOOST_DI_CORE_POOL_HPP
#define BOOST_DI_CORE_POOL_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

template<class = aux::type_list<>>
class pool;

template<class... TArgs>
using pool_t = pool<aux::type_list<TArgs...>>;

template<class... TArgs>
class pool<aux::type_list<TArgs...>> : public TArgs... {
public:
    template<class... Ts>
    explicit pool(const Ts&... args) noexcept
        : Ts(args)...
    { }

    template<class... Ts, class TPool>
    pool(const aux::type_list<Ts...>&, const TPool& p) noexcept
        : pool(static_cast<const Ts&>(p)...)
    { }
};

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_AUX_COMPILER_HPP
#define BOOST_DI_AUX_COMPILER_HPP

#if defined(__clang__)
    #define BOOST_DI_CLANG
#elif defined(__GNUC__)
    #define BOOST_DI_GCC
#elif defined(_MSC_VER)
    #define BOOST_DI_MSVC
#endif

#if defined(BOOST_DI_CLANG)
    #define BOOST_DI_UNUSED __attribute__((unused))
    #define BOOST_DI_ATTR_ERROR(...) [[deprecated(__VA_ARGS__)]]
    #define BOOST_DI_LIKELY(...) __builtin_expect((__VA_ARGS__), 1)
    #define BOOST_DI_UNLIKELY(...) __builtin_expect((__VA_ARGS__), 0)
#elif defined(BOOST_DI_GCC)
    #define BOOST_DI_UNUSED __attribute__((unused))
    #define BOOST_DI_ATTR_ERROR(...) __attribute__ ((error(__VA_ARGS__)))
    #define BOOST_DI_LIKELY(...) __builtin_expect((__VA_ARGS__), 1)
    #define BOOST_DI_UNLIKELY(...) __builtin_expect((__VA_ARGS__), 0)
#elif defined(BOOST_DI_MSVC)
    #pragma warning(disable : 4503) // decorated name length exceeded, name was truncated
    #pragma warning(disable : 4822) // local class member function does not have a body
    #if !defined(__has_include)
        #define __has_include(...) 0
    #endif
    #define BOOST_DI_UNUSED
    #define BOOST_DI_ATTR_ERROR(...) __declspec(deprecated(__VA_ARGS__))
    #define BOOST_DI_LIKELY(...) __VA_ARGS__
    #define BOOST_DI_UNLIKELY(...) __VA_ARGS__
#endif

#endif

#ifndef BOOST_DI_WRAPPERS_UNIQUE_HPP
#define BOOST_DI_WRAPPERS_UNIQUE_HPP

namespace boost { namespace di { inline namespace v1 { namespace wrappers {

template<class T>
struct unique {
    template<class I>
    inline operator I() const noexcept {
        return object;
    }

    inline operator T&&() noexcept {
        return std::move(object);
    }

    T object;
};

template<class T>
struct unique<T*> {
    template<class I>
    inline operator I() const noexcept {
        return *std::unique_ptr<I>{object};
    }

    template<class I>
    inline operator I*() const noexcept {
        return object; // ownership transfer
    }

    template<class I>
    inline operator const I*() const noexcept {
        return object; // ownership transfer
    }

    template<class I>
    inline operator std::shared_ptr<I>() const noexcept {
        return std::shared_ptr<I>{object};
    }

    template<class I>
    inline operator boost::shared_ptr<I>() const noexcept {
        return boost::shared_ptr<I>{object};
    }

    template<class I>
    inline operator std::unique_ptr<I>() const noexcept {
        return std::unique_ptr<I>{object};
    }

    #if defined(BOOST_DI_MSVC)
        explicit unique(T* object)
            : object(object)
        { }
    #endif

    T* object = nullptr;
};

template<class T, class TDeleter>
struct unique<std::unique_ptr<T, TDeleter>> {
    template<class I>
    inline operator I() const noexcept {
        return *object;
    }

    template<class I>
    inline operator I*() noexcept {
        return object.release();
    }

    template<class I>
    inline operator const I*() noexcept {
        return object.release();
    }

    template<class I>
    inline operator std::shared_ptr<I>() noexcept {
        return {object.release(), object.get_deleter()};
    }

    template<class I>
    inline operator boost::shared_ptr<I>() noexcept {
        return {object.release(), object.get_deleter()};
    }

    template<class I, class D>
    inline operator std::unique_ptr<I, D>() noexcept {
        return std::move(object);
    }

    std::unique_ptr<T, TDeleter> object;
};

}}}} // boost::di::v1::wrappers

#endif

#ifndef BOOST_DI_TYPE_TRAITS_MEMORY_TRAITS_HPP
#define BOOST_DI_TYPE_TRAITS_MEMORY_TRAITS_HPP

namespace boost { namespace di { inline namespace v1 { namespace type_traits {

struct stack { };
struct heap { };

template<class T, class = void>
struct memory_traits {
    using type = stack;
};

template<class T>
struct memory_traits<T&> {
    using type = stack;
};

template<class T>
struct memory_traits<const T&> {
    using type = stack;
};

template<class T>
struct memory_traits<T*> {
    using type = heap;
};

template<class T>
struct memory_traits<const T*> {
    using type = heap;
};

template<class T>
struct memory_traits<T&&> {
    using type = stack;
};

template<class T>
struct memory_traits<const T&&> {
    using type = stack;
};

template<class T, class TDeleter>
struct memory_traits<std::unique_ptr<T, TDeleter>> {
    using type = heap;
};

template<class T, class TDeleter>
struct memory_traits<const std::unique_ptr<T, TDeleter>&> {
    using type = heap;
};

template<class T>
struct memory_traits<std::shared_ptr<T>> {
    using type = heap;
};

template<class T>
struct memory_traits<const std::shared_ptr<T>&> {
    using type = heap;
};

template<class T>
struct memory_traits<boost::shared_ptr<T>> {
    using type = heap;
};

template<class T>
struct memory_traits<const boost::shared_ptr<T>&> {
    using type = heap;
};

template<class T>
struct memory_traits<std::weak_ptr<T>> {
    using type = heap;
};

template<class T>
struct memory_traits<const std::weak_ptr<T>&> {
    using type = heap;
};

template<class T>
struct memory_traits<T, std::enable_if_t<std::is_polymorphic<T>::value>> {
    using type = heap;
};

template<class T>
using memory_traits_t = typename memory_traits<T>::type;

}}}} // boost::di::v1::type_traits

#endif

#ifndef BOOST_DI_SCOPES_UNIQUE_HPP
#define BOOST_DI_SCOPES_UNIQUE_HPP

namespace boost { namespace di { inline namespace v1 { namespace scopes {

class unique {
public:
    template<class, class>
    class scope {
    public:
        template<class>
        using is_referable = std::false_type;

        template<class T, class TProvider>
        decltype(wrappers::unique<decltype(std::declval<TProvider>().get(type_traits::memory_traits_t<T>{}))>{
            std::declval<TProvider>().get(type_traits::memory_traits_t<T>{})})
        try_create(const TProvider&) const;

        template<class T, class TProvider>
        auto create(const TProvider& provider) const {
            using memory = type_traits::memory_traits_t<T>;
            using wrapper = wrappers::unique<decltype(provider.get(memory{}))>;
            return wrapper{provider.get(memory{})};
        }
    };
};

}}}} // boost::di::v1::scopes

#endif

#ifndef BOOST_DI_WRAPPERS_SHARED_HPP
#define BOOST_DI_WRAPPERS_SHARED_HPP

namespace boost { namespace di { inline namespace v1 { namespace wrappers {

template<class T, bool Ref = true>
struct shared {
    using type = std::conditional_t<std::is_same<T, void>::value, _, T>;

    template<class>
    struct is_referable
        : std::true_type
    { };

    template<class I>
    struct is_referable<std::shared_ptr<I>>
        : std::false_type
    { };

    template<class I>
    inline operator std::shared_ptr<I>() const noexcept {
        return object;
    }

    template<class TSharedPtr>
    struct sp_holder {
        TSharedPtr object;

        void operator()(...) noexcept {
            object.reset();
        }
    };

    template<class I>
    inline operator boost::shared_ptr<I>() const noexcept {
        using sp = sp_holder<boost::shared_ptr<T>>;
        if (auto* deleter = std::get_deleter<sp, T>(object)) {
            return deleter->object;
        } else {
            return {object.get(), sp_holder<std::shared_ptr<T>>{object}};
        }
    }

    template<class I>
    inline operator std::weak_ptr<I>() const noexcept {
        return object;
    }

    inline operator type&() noexcept {
        return *object;
    }

    inline operator const type&() const noexcept {
        return *object;
    }

    std::conditional_t<Ref, const std::shared_ptr<T>&, std::shared_ptr<T>> object;
};

template<class T>
struct shared<T&> {
    shared(T& object) : object(&object) { }
    shared(const shared&) noexcept = default;
    shared& operator=(const shared&) noexcept = default;

    template<class I, BOOST_DI_REQUIRES(std::is_convertible<std::reference_wrapper<T>, I>::value)>
    inline operator I() const noexcept {
        return *object;
    }

    inline operator T&() const noexcept {
        return *object;
    }

#if defined(BOOST_DI_MSVC)
    template<class I>
    inline operator I*() noexcept { // only for compilation clean
        return {};
    }
#endif

    T* object = nullptr;
};

}}}} // boost::di::v1::wrappers

#endif

#ifndef BOOST_DI_SCOPES_SINGLETON_HPP
#define BOOST_DI_SCOPES_SINGLETON_HPP

namespace boost { namespace di { inline namespace v1 { namespace scopes {

class singleton {
public:
    template<class, class T>
    class scope {
    public:
        template<class T_>
        using is_referable = typename wrappers::shared<T>::template is_referable<T_>;

        template<class, class TProvider>
        decltype(wrappers::shared<T>{std::shared_ptr<T>{std::declval<TProvider>().get()}})
        try_create(const TProvider&);

        template<class>
        void try_create(...);

        template<class, class TProvider>
        auto create(const TProvider& provider) {
            if (BOOST_DI_UNLIKELY(!get_instance())) {
                get_instance() = std::shared_ptr<T>{provider.get()};
            }
            return wrappers::shared<T>{get_instance()};
        }

    private:
        static std::shared_ptr<T>& get_instance() noexcept {
            static std::shared_ptr<T> object;
            return object;
        }
    };
};

}}}} // boost::di::v1::scopes

#endif

#ifndef BOOST_DI_TYPE_TRAITS_SCOPE_TRAITS_HPP
#define BOOST_DI_TYPE_TRAITS_SCOPE_TRAITS_HPP

namespace boost { namespace di { inline namespace v1 { namespace type_traits {

template<class T>
struct scope_traits {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<T&> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<const T&> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<T*> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<const T*> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<T&&> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<const T&&> {
    using type = scopes::unique;
};

template<class T, class TDeleter>
struct scope_traits<std::unique_ptr<T, TDeleter>> {
    using type = scopes::unique;
};

template<class T, class TDeleter>
struct scope_traits<const std::unique_ptr<T, TDeleter>&> {
    using type = scopes::unique;
};

template<class T>
struct scope_traits<std::shared_ptr<T>> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<const std::shared_ptr<T>&> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<boost::shared_ptr<T>> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<const boost::shared_ptr<T>&> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<std::weak_ptr<T>> {
    using type = scopes::singleton;
};

template<class T>
struct scope_traits<const std::weak_ptr<T>&> {
    using type = scopes::singleton;
};

template<class T>
using scope_traits_t = typename scope_traits<T>::type;

}}}} // boost::di::v1::type_traits

#endif

#ifndef BOOST_DI_SCOPES_DEDUCE_HPP
#define BOOST_DI_SCOPES_DEDUCE_HPP

namespace boost { namespace di { inline namespace v1 { namespace scopes {

class deduce {
public:
    template<class TExpected, class TGiven>
    class scope {
    public:
        template<class T>
        using is_referable = typename type_traits::scope_traits_t<T>::template
            scope<TExpected, TGiven>::template is_referable<aux::remove_accessors_t<T>>;

        template<class T, class TProvider>
        decltype(typename type_traits::scope_traits_t<T>::template
            scope<TExpected, TGiven>{}.template try_create<T>(std::declval<TProvider>()))
        try_create(const TProvider& provider);

        template<class>
        void try_create(...);

        template<class T, class TProvider>
        auto create(const TProvider& provider) {
            using scope_traits = type_traits::scope_traits_t<T>;
            using scope = typename scope_traits::template scope<TExpected, TGiven>;
            return scope{}.template create<T>(provider);
        }
    };
};

}}}} // boost::di::v1::scopes

#endif

#ifndef BOOST_DI_SCOPES_EXPOSED_HPP
#define BOOST_DI_SCOPES_EXPOSED_HPP

namespace boost { namespace di { inline namespace v1 { namespace scopes {

template<class TScope = scopes::deduce>
class exposed {
public:
    template<class TExpected, class TGiven>
    class scope {
        #if defined(BOOST_DI_GCC) || defined(BOOST_DI_MSVC)
            using type = std::conditional_t<
                std::is_copy_constructible<TExpected>::value
              , TExpected
              , TExpected*
            >;
        #else
            using type = TExpected;
        #endif

        struct iprovider {
            TExpected* (*heap)(const iprovider*);
            type (*stack)(const iprovider*);

            auto get(const type_traits::heap& = {}) const noexcept {
                return ((iprovider*)(this))->heap(this);
            }

            auto get(const type_traits::stack&) const noexcept {
                return ((iprovider*)(this))->stack(this);
            }
        };

        template<class TInjector>
        struct provider_impl {
            TExpected* (*heap)(const provider_impl*);
            type (*stack)(const provider_impl*);

            template<class T>
            static T create(const provider_impl* object) noexcept {
                return object->injector.create_impl(aux::type<T>{});
            }

            explicit provider_impl(const TInjector& injector) noexcept
                : provider_impl(injector
                              , std::integral_constant<bool, TInjector::template is_creatable<TExpected*>::value>{}
                              , std::integral_constant<bool, TInjector::template is_creatable<TExpected>::value>{}
                  )
            { }

            provider_impl(const TInjector& injector, const std::true_type&, const std::true_type&) noexcept
                : heap(&provider_impl::template create<TExpected*>)
                , stack(&provider_impl::template create<type>)
                , injector(injector)
            { }

            provider_impl(const TInjector& injector, const std::false_type&, const std::true_type&) noexcept
                : stack(&provider_impl::template create<type>)
                , injector(injector)
            { }

            provider_impl(const TInjector& injector, const std::true_type&, const std::false_type&) noexcept
                : heap(&provider_impl::template create<TExpected*>)
                , injector(injector)
            { }

            provider_impl(const TInjector& injector, const std::false_type&, const std::false_type&)
                : heap(&provider_impl::template create<TExpected*>) // for creatable error
                , injector(injector)
            { }

            TInjector injector;
        };

    public:
        template<class>
        using is_referable = std::false_type;

        template<class TInjector>
        explicit scope(const TInjector& injector) noexcept {
            static auto provider = provider_impl<TInjector>{injector};
            provider.injector = injector;
            provider_ = (iprovider*)&provider;
        }

        template<class T, class TProvider>
        T try_create(const TProvider&);

        template<class T>
        void try_create(...);

        template<class T, class TProvider>
        auto create(const TProvider&) {
            return scope_.template create<T>(*provider_);
        }

    private:
        iprovider* provider_ = nullptr;
        typename TScope::template scope<TExpected, TGiven> scope_;
    };
};

}}}} // boost::di::v1::scopes

#endif

#ifndef BOOST_DI_TYPE_TRAITS_WRAPPER_TRAITS_HPP
#define BOOST_DI_TYPE_TRAITS_WRAPPER_TRAITS_HPP

namespace boost { namespace di { inline namespace v1 { namespace type_traits {

template<class T>
struct wrapper_traits {
    using type = wrappers::unique<T>;
};

template<class T>
struct wrapper_traits<std::shared_ptr<T>> {
    using type = wrappers::shared<T, false>;
};

template<class T>
using wrapper_traits_t = typename wrapper_traits<T>::type;

}}}} // boost::di::v1::type_traits

#endif

#ifndef BOOST_DI_SCOPES_EXTERNAL_HPP
#define BOOST_DI_SCOPES_EXTERNAL_HPP

namespace boost { namespace di { inline namespace v1 {

BOOST_DI_HAS_METHOD(call_operator, operator());

namespace scopes {

BOOST_DI_HAS_TYPE(result_type);

class external {
    struct injector {
        template<class T> T create() const;
    };

    template<class T, class TExpected, class TGiven>
    struct arg {
        using type = T;
        using expected = TExpected;
        using given = TGiven;
    };

public:
    template<class TExpected, class, class = void>
    struct scope {
        template<class>
        using is_referable = std::false_type;

        explicit scope(const TExpected& object)
            : object_{object}
        { }

        template<class, class TProvider>
        TExpected try_create(const TProvider&);

        template<class, class TProvider>
        auto create(const TProvider&) const noexcept {
            return wrappers::unique<TExpected>{object_};
        }

        TExpected object_;
    };

    template<class TExpected, class TGiven>
    struct scope<TExpected, TGiven&,
        BOOST_DI_REQUIRES_T(!has_call_operator<TGiven, const injector&>::value &&
                            !has_call_operator<TGiven, const injector&, const arg<aux::none_type, TExpected, TGiven>&>::value)
    > {
        template<class>
        using is_referable = std::true_type;

        explicit scope(TGiven& object)
            : object_{object}
        { }

        template<class, class TProvider>
        wrappers::shared<TGiven&> try_create(const TProvider&);

        template<class, class TProvider>
        auto create(const TProvider&) const noexcept {
            return object_;
        }

        wrappers::shared<TGiven&> object_;
    };

    template<class TExpected, class TGiven>
    struct scope<TExpected, std::shared_ptr<TGiven>> {
        template<class T>
        using is_referable = typename wrappers::shared<TGiven>::template is_referable<aux::remove_accessors_t<T>>;

        explicit scope(const std::shared_ptr<TGiven>& object)
            : object_{object}
        { }

        template<class, class TProvider>
        wrappers::shared<TGiven> try_create(const TProvider&);

        template<class, class TProvider>
        auto create(const TProvider&) const noexcept {
            return wrappers::shared<TGiven>{object_};
        }

        std::shared_ptr<TGiven> object_;
    };

    template<class TExpected, class TGiven>
    struct scope<TExpected, TGiven,
        BOOST_DI_REQUIRES_T(!has_call_operator<TGiven, const injector&>::value &&
                            !has_call_operator<TExpected>::value &&
                             has_call_operator<TGiven>::value)
    > {
        template<class>
        using is_referable = std::false_type;

        explicit scope(const TGiven& object)
            : object_(object)
        { }

        template<class T, class TProvider>
        auto try_create(const TProvider&) -> type_traits::wrapper_traits_t<decltype(std::declval<TGiven>()())>;

        template<class, class TProvider>
        auto create(const TProvider&) const noexcept {
            using wrapper = type_traits::wrapper_traits_t<decltype(std::declval<TGiven>()())>;
            return wrapper{object_()};
        }

        TGiven object_;
    };

    template<class TExpected, class TGiven>
    struct scope<TExpected, TGiven,
        BOOST_DI_REQUIRES_T(has_call_operator<TGiven, const injector&>::value &&
                           !has_result_type<TGiven>::value)
    > {
        template<class>
        using is_referable = std::false_type;

        explicit scope(const TGiven& object)
            : object_(object)
        { }

        template<class T, class TProvider>
        T try_create(const TProvider&);

        template<class, class TProvider>
        auto create(const TProvider& provider) const noexcept {
            using wrapper = type_traits::wrapper_traits_t<decltype((object_)(provider.injector_))>;
            return wrapper{(object_)(provider.injector_)};
        }

        TGiven object_;
    };

    template<class TExpected, class TGiven>
    struct scope<TExpected, TGiven,
        BOOST_DI_REQUIRES_T(has_call_operator<TGiven, const injector&, const arg<aux::none_type, TExpected, TGiven>&>::value &&
                           !has_result_type<TGiven>::value)
    > {
        template<class>
        using is_referable = std::false_type;

        explicit scope(const TGiven& object)
            : object_(object)
        { }

        template<class T, class TProvider>
        T try_create(const TProvider&);

        template<class T, class TProvider>
        auto create(const TProvider& provider) const noexcept {
            using wrapper = type_traits::wrapper_traits_t<decltype((object_)(provider.injector_, arg<T, TExpected, TGiven>{}))>;
            return wrapper{(object_)(provider.injector_, arg<T, TExpected, TGiven>{})};
        }

        TGiven object_;
    };
};

}}}} // boost::di::v1::scopes

#endif

#ifndef BOOST_DI_AUX_PREPROCESSOR_HPP
#define BOOST_DI_AUX_PREPROCESSOR_HPP

#define BOOST_DI_IF(cond, t, f) BOOST_DI_IF_I(cond, t, f)
#define BOOST_DI_REPEAT(i, m, ...) BOOST_DI_REPEAT_N(i, m, __VA_ARGS__)
#define BOOST_DI_CAT(a, ...) BOOST_DI_PRIMITIVE_CAT(a, __VA_ARGS__)
#define BOOST_DI_CALL(m, ...) m(__VA_ARGS__)
#define BOOST_DI_EMPTY()
#define BOOST_DI_COMMA() ,
#define BOOST_DI_EAT(...)
#define BOOST_DI_EXPAND(...) __VA_ARGS__
#define BOOST_DI_SIZE(...) BOOST_DI_CAT(BOOST_DI_VARIADIC_SIZE_I(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,),)
#define BOOST_DI_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
#define BOOST_DI_ELEM(n, ...) BOOST_DI_ELEM_I(n,__VA_ARGS__)
#define BOOST_DI_IS_EMPTY(...) BOOST_DI_DETAIL_IS_EMPTY_IIF(BOOST_DI_IBP(__VA_ARGS__))(BOOST_DI_DETAIL_IS_EMPTY_GEN_ZERO, BOOST_DI_DETAIL_IS_EMPTY_PROCESS)(__VA_ARGS__)

#define BOOST_DI_DETAIL_IS_EMPTY_PRIMITIVE_CAT(a, b) a ## b
#define BOOST_DI_DETAIL_IS_EMPTY_IIF(bit) BOOST_DI_DETAIL_IS_EMPTY_PRIMITIVE_CAT(BOOST_DI_DETAIL_IS_EMPTY_IIF_,bit)
#define BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C(...) ()
#define BOOST_DI_DETAIL_IS_EMPTY_GEN_ZERO(...) 0
#define BOOST_DI_VARIADIC_SIZE_I(e0, e1, e2, e3, e4, e5, e6, e7, e8, e9, size, ...) size
#define BOOST_DI_IF_I(cond, t, f) BOOST_DI_IIF(cond, t, f)
#define BOOST_DI_IIF_0(t, f) f
#define BOOST_DI_IIF_1(t, f) t
#define BOOST_DI_IIF_2(t, f) t
#define BOOST_DI_IIF_3(t, f) t
#define BOOST_DI_IIF_4(t, f) t
#define BOOST_DI_IIF_5(t, f) t
#define BOOST_DI_IIF_6(t, f) t
#define BOOST_DI_IIF_7(t, f) t
#define BOOST_DI_IIF_8(t, f) t
#define BOOST_DI_IIF_9(t, f) t
#define BOOST_DI_ELEM_I(n, ...) BOOST_DI_CAT(BOOST_DI_CAT(BOOST_DI_ELEM, n)(__VA_ARGS__,),)
#define BOOST_DI_ELEM0(p1, ...) p1
#define BOOST_DI_ELEM1(p1, p2, ...) p2
#define BOOST_DI_ELEM2(p1, p2, p3, ...) p3
#define BOOST_DI_ELEM3(p1, p2, p3, p4, ...) p4
#define BOOST_DI_ELEM4(p1, p2, p3, p4, p5, ...) p5
#define BOOST_DI_ELEM5(p1, p2, p3, p4, p5, p6, ...) p6
#define BOOST_DI_ELEM6(p1, p2, p3, p4, p5, p6, p7, ...) p7
#define BOOST_DI_ELEM7(p1, p2, p3, p4, p5, p6, p7, p8, ...) p8
#define BOOST_DI_ELEM8(p1, p2, p3, p4, p5, p6, p7, p8, p9, ...) p9
#define BOOST_DI_ELEM9(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, ...) p10
#define BOOST_DI_REPEAT_N(i, m, ...) BOOST_DI_REPEAT_##i(m, __VA_ARGS__)
#define BOOST_DI_REPEAT_1(m, ...) m(0, __VA_ARGS__)
#define BOOST_DI_REPEAT_2(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__)
#define BOOST_DI_REPEAT_3(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__)
#define BOOST_DI_REPEAT_4(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__)
#define BOOST_DI_REPEAT_5(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__)
#define BOOST_DI_REPEAT_6(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__)
#define BOOST_DI_REPEAT_7(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) m(6, __VA_ARGS__)
#define BOOST_DI_REPEAT_8(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) m(6, __VA_ARGS__) m(7, __VA_ARGS__)
#define BOOST_DI_REPEAT_9(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) m(6, __VA_ARGS__) m(7, __VA_ARGS__) m(8, __VA_ARGS__)
#define BOOST_DI_REPEAT_10(m, ...) m(0, __VA_ARGS__) m(1, __VA_ARGS__) m(2, __VA_ARGS__) m(3, __VA_ARGS__) m(4, __VA_ARGS__) m(5, __VA_ARGS__) m(6, __VA_ARGS__) m(7, __VA_ARGS__) m(8, __VA_ARGS__) m(9, __VA_ARGS__)

#if defined(BOOST_DI_MSVC)
    #define BOOST_DI_VD_IBP_CAT(a, b) BOOST_DI_VD_IBP_CAT_I(a, b)
    #define BOOST_DI_VD_IBP_CAT_I(a, b) BOOST_DI_VD_IBP_CAT_II(a ## b)
    #define BOOST_DI_VD_IBP_CAT_II(res) res
    #define BOOST_DI_IBP_SPLIT(i, ...) BOOST_DI_VD_IBP_CAT(BOOST_DI_IBP_PRIMITIVE_CAT(BOOST_DI_IBP_SPLIT_,i)(__VA_ARGS__),BOOST_DI_EMPTY())
    #define BOOST_DI_IBP_IS_VARIADIC_C(...) 1 1
    #define BOOST_DI_IBP_SPLIT_0(a, ...) a
    #define BOOST_DI_IBP_SPLIT_1(a, ...) __VA_ARGS__
    #define BOOST_DI_IBP_CAT(a, ...) BOOST_DI_IBP_PRIMITIVE_CAT(a,__VA_ARGS__)
    #define BOOST_DI_IBP_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__
    #define BOOST_DI_IBP_IS_VARIADIC_R_1 1,
    #define BOOST_DI_IBP_IS_VARIADIC_R_BOOST_DI_IBP_IS_VARIADIC_C 0,
    #define BOOST_DI_IBP(...) BOOST_DI_IBP_SPLIT(0, BOOST_DI_IBP_CAT(BOOST_DI_IBP_IS_VARIADIC_R_, BOOST_DI_IBP_IS_VARIADIC_C __VA_ARGS__))
    #define BOOST_DI_IIF(bit, t, f) BOOST_DI_IIF_OO((bit, t, f))
    #define BOOST_DI_IIF_OO(par) BOOST_DI_IIF_I ## par
    #define BOOST_DI_IIF_I(bit, t, f) BOOST_DI_IIF_ ## bit(t, f)
    #define BOOST_DI_DETAIL_IS_EMPTY_IIF_0(t, b) b
    #define BOOST_DI_DETAIL_IS_EMPTY_IIF_1(t, b) t
    #define BOOST_DI_DETAIL_IS_EMPTY_PROCESS(...) BOOST_DI_IBP(BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C __VA_ARGS__ ())
#else
    #define BOOST_DI_IBP_SPLIT(i, ...) BOOST_DI_PRIMITIVE_CAT(BOOST_DI_IBP_SPLIT_, i)(__VA_ARGS__)
    #define BOOST_DI_IBP_SPLIT_0(a, ...) a
    #define BOOST_DI_IBP_SPLIT_1(a, ...) __VA_ARGS__
    #define BOOST_DI_IBP_IS_VARIADIC_C(...) 1
    #define BOOST_DI_IBP_IS_VARIADIC_R_1 1,
    #define BOOST_DI_IBP_IS_VARIADIC_R_BOOST_DI_IBP_IS_VARIADIC_C 0,
    #define BOOST_DI_IBP(...) BOOST_DI_IBP_SPLIT(0, BOOST_DI_CAT( BOOST_DI_IBP_IS_VARIADIC_R_, BOOST_DI_IBP_IS_VARIADIC_C __VA_ARGS__))
    #define BOOST_DI_IIF(bit, t, f) BOOST_DI_IIF_I(bit, t, f)
    #define BOOST_DI_IIF_I(bit, t, f) BOOST_DI_IIF_II(BOOST_DI_IIF_ ## bit(t, f))
    #define BOOST_DI_IIF_II(id) id
    #define BOOST_DI_DETAIL_IS_EMPTY_IIF_0(t, ...) __VA_ARGS__
    #define BOOST_DI_DETAIL_IS_EMPTY_IIF_1(t, ...) t
    #define BOOST_DI_DETAIL_IS_EMPTY_PROCESS(...) BOOST_DI_IBP(BOOST_DI_DETAIL_IS_EMPTY_NON_FUNCTION_C __VA_ARGS__ ())
#endif

#endif

#ifndef BOOST_DI_INJECT_HPP
#define BOOST_DI_INJECT_HPP

#if !defined(BOOST_DI_INJECTOR)
    #define BOOST_DI_INJECTOR boost_di_injector__
#endif

#if !defined(BOOST_DI_CFG_CTOR_LIMIT_SIZE)
    #define BOOST_DI_CFG_CTOR_LIMIT_SIZE 10
#endif

namespace boost { namespace di { inline namespace v1 { namespace detail {
template<class, class>
struct named_type { };

struct named_impl { template<class T> T operator=(const T&) const; };
static constexpr BOOST_DI_UNUSED named_impl named{};

template<class T, class TName>
struct combine_impl {
    using type = named_type<TName, T>;
};

template<class T>
struct combine_impl<T, aux::none_type> {
    using type = T;
};

template<class, class>
struct combine;

template<class... T1, class... T2>
struct combine<aux::type_list<T1...>, aux::type_list<T2...>> {
    using type = aux::type_list<typename combine_impl<T1, T2>::type...>;
};
}}}} // boost::di::v1::detail

#define BOOST_DI_GEN_CTOR_IMPL(p, i) \
    BOOST_DI_IF(i, BOOST_DI_COMMA, BOOST_DI_EAT)() \
    BOOST_DI_IF(BOOST_DI_IBP(p), BOOST_DI_EAT p, p)
#define BOOST_DI_GEN_CTOR(i, ...) BOOST_DI_GEN_CTOR_IMPL(BOOST_DI_ELEM(i, __VA_ARGS__,), i)
#define BOOST_DI_GEN_ARG_NAME(p) BOOST_DI_GEN_ARG_NAME_IMPL p )
#define BOOST_DI_GEN_NONE_TYPE(p) ::boost::di::aux::none_type
#define BOOST_DI_GEN_ARG_NAME_IMPL(p) decltype(::boost::di::detail::p) BOOST_DI_EAT(
#define BOOST_DI_GEN_NAME_IMPL(p, i) \
    BOOST_DI_IF(i, BOOST_DI_COMMA, BOOST_DI_EAT)() \
    BOOST_DI_IF(BOOST_DI_IBP(p), BOOST_DI_GEN_ARG_NAME, BOOST_DI_GEN_NONE_TYPE)(p)
#define BOOST_DI_GEN_NAME(i, ...) BOOST_DI_GEN_NAME_IMPL(BOOST_DI_ELEM(i, __VA_ARGS__,), i)

#define BOOST_DI_INJECT_TRAITS_EMPTY_IMPL(...) \
    using BOOST_DI_INJECTOR BOOST_DI_UNUSED = ::boost::di::aux::type_list<>

#define BOOST_DI_INJECT_TRAITS_IMPL(...) \
    static void BOOST_DI_CAT(BOOST_DI_INJECTOR, ctor)( \
        BOOST_DI_REPEAT(BOOST_DI_SIZE(__VA_ARGS__), BOOST_DI_GEN_CTOR, __VA_ARGS__) \
    ); \
    static void BOOST_DI_CAT(BOOST_DI_INJECTOR, names)( \
        BOOST_DI_REPEAT(BOOST_DI_SIZE(__VA_ARGS__), BOOST_DI_GEN_NAME, __VA_ARGS__) \
    ); \
    using BOOST_DI_INJECTOR BOOST_DI_UNUSED = ::boost::di::detail::combine< \
        typename ::boost::di::aux::function_traits<decltype(BOOST_DI_CAT(BOOST_DI_INJECTOR, ctor))>::args \
      , typename ::boost::di::aux::function_traits<decltype(BOOST_DI_CAT(BOOST_DI_INJECTOR, names))>::args \
    >::type; \
    static_assert( \
        BOOST_DI_SIZE(__VA_ARGS__) <= BOOST_DI_CFG_CTOR_LIMIT_SIZE \
      , "Number of constructor arguments is out of range - see BOOST_DI_CFG_CTOR_LIMIT_SIZE" \
    )

#if !defined(BOOST_DI_INJECT_TRAITS)
    #define BOOST_DI_INJECT_TRAITS(...) \
        BOOST_DI_IF( \
            BOOST_DI_IS_EMPTY(__VA_ARGS__) \
          , BOOST_DI_INJECT_TRAITS_EMPTY_IMPL \
          , BOOST_DI_INJECT_TRAITS_IMPL \
        )(__VA_ARGS__)
#endif

#if !defined(BOOST_DI_INJECT_TRAITS_NO_LIMITS)
    #define BOOST_DI_INJECT_TRAITS_NO_LIMITS(...) \
        static void BOOST_DI_CAT(BOOST_DI_INJECTOR, ctor)(__VA_ARGS__); \
        using BOOST_DI_INJECTOR BOOST_DI_UNUSED = \
            ::boost::di::aux::function_traits<decltype(BOOST_DI_CAT(BOOST_DI_INJECTOR, ctor))>::args
#endif

#if !defined(BOOST_DI_INJECT)
    #define BOOST_DI_INJECT(type, ...) \
        BOOST_DI_INJECT_TRAITS(__VA_ARGS__); \
        type(BOOST_DI_REPEAT( \
            BOOST_DI_SIZE(__VA_ARGS__) \
          , BOOST_DI_GEN_CTOR \
          , __VA_ARGS__) \
        )
#endif

#endif

#ifndef BOOST_DI_TYPE_TRAITS_CTOR_TRAITS_HPP
#define BOOST_DI_TYPE_TRAITS_CTOR_TRAITS_HPP

namespace boost { namespace di { inline namespace v1 { namespace type_traits {

struct direct { };
struct uniform { };

BOOST_DI_CALL(BOOST_DI_HAS_TYPE, BOOST_DI_INJECTOR);

template<class T, std::size_t>
using get = T;

template<template<class...> class, class, class, class = void>
struct ctor_impl;

template<template<class...> class TIsConstructible, class T, std::size_t... TArgs>
struct ctor_impl<TIsConstructible, T, std::index_sequence<TArgs...>
    , BOOST_DI_REQUIRES_T((sizeof...(TArgs) > 0) && !TIsConstructible<T, get<core::any_type_fwd<T>, TArgs>...>::value)>
    : std::conditional<
           TIsConstructible<T, get<core::any_type_ref_fwd<T>, TArgs>...>::value
         , aux::type_list<get<core::any_type_ref_fwd<T>, TArgs>...>
         , typename ctor_impl<
               TIsConstructible
             , T
             , std::make_index_sequence<sizeof...(TArgs) - 1>
           >::type
      >
{ };

template<template<class...> class TIsConstructible, class T, std::size_t... TArgs>
struct ctor_impl<TIsConstructible, T, std::index_sequence<TArgs...>,
      BOOST_DI_REQUIRES_T((sizeof...(TArgs) > 0) && TIsConstructible<T, get<core::any_type_fwd<T>, TArgs>...>::value)>
    : aux::type_list<get<core::any_type_fwd<T>, TArgs>...>
{ };

template<template<class...> class TIsConstructible, class T>
struct ctor_impl<TIsConstructible, T, std::index_sequence<>>
    : aux::type_list<>
{ };

template<template<class...> class TIsConstructible, class T>
using ctor_impl_t =
    typename ctor_impl<
        TIsConstructible
      , T
      , std::make_index_sequence<BOOST_DI_CFG_CTOR_LIMIT_SIZE>
    >::type;

template<class...>
struct ctor;

template<class T>
struct ctor<T, aux::type_list<>>
    : aux::pair<uniform, ctor_impl_t<aux::is_braces_constructible, T>>
{ };

template<class T, class... TArgs>
struct ctor<T, aux::type_list<TArgs...>>
    : aux::pair<direct, aux::type_list<TArgs...>>
{ };

} // type_traits

template<class T>
struct ctor_traits
    : type_traits::ctor<T, type_traits::ctor_impl_t<std::is_constructible, T>>
{ };

namespace type_traits {

template<
    class T
  , class = typename BOOST_DI_CAT(has_, BOOST_DI_INJECTOR)<T>::type
> struct ctor_traits;

template<
    class T
  , class = typename BOOST_DI_CAT(has_, BOOST_DI_INJECTOR)<di::ctor_traits<T>>::type
> struct ctor_traits_impl;

template<class T>
struct ctor_traits<T, std::true_type>
    : aux::pair<direct, typename T::BOOST_DI_INJECTOR>
{ };

template<class T>
struct ctor_traits<T, std::false_type>
    : ctor_traits_impl<T>
{ };

template<class T>
struct ctor_traits_impl<T, std::true_type>
    : aux::pair<direct, typename di::ctor_traits<T>::BOOST_DI_INJECTOR>
{ };

template<class T>
struct ctor_traits_impl<T, std::false_type>
    : di::ctor_traits<T>
{ };

}}}} // boost::di::v1::type_traits

#if __has_include(<string>)
    #include <string>

    namespace boost { namespace di { inline namespace v1 {
        template<
            class T
          , class Traits
          , class TAllocator
        > struct ctor_traits<std::basic_string<T, Traits, TAllocator>> {
            BOOST_DI_INJECT_TRAITS();
        };
    }}} // boost::di::v1
#endif

namespace std {
    template<class>
    class initializer_list;

    #if defined(BOOST_DI_MSVC)
        template<class>
        class function;
    #endif
} // std

namespace boost { namespace di { inline namespace v1 {
    template<class T>
    struct ctor_traits<std::initializer_list<T>> {
        BOOST_DI_INJECT_TRAITS();
    };

    #if defined(BOOST_DI_MSVC)
        template<class T>
        struct ctor_traits<std::function<T>> {
            BOOST_DI_INJECT_TRAITS();
        };
    #endif
}}} // boost::di::v1

#endif

#ifndef BOOST_DI_CONCEPTS_SCOPABLE_HPP
#define BOOST_DI_CONCEPTS_SCOPABLE_HPP

namespace boost { namespace di { inline namespace v1 { namespace concepts {

template<class T>
struct provider {
    template<class TMemory = type_traits::heap>
    std::conditional_t<std::is_same<TMemory, type_traits::stack>::value, T, T*>
    try_get(const TMemory& = {}) const;

    template<class TMemory = type_traits::heap>
    T* get(const TMemory& = {}) const {
        return nullptr;
    }
};

std::false_type scopable_impl(...);

template<class T>
auto scopable_impl(T&&) -> aux::is_valid_expr<
    decltype(std::declval<typename T::template scope<_, _>>().template create<_>(provider<_>{}))
  , decltype(std::declval<typename T::template scope<_, _>>().template try_create<_>(provider<_>{}))
>;

template<class T>
struct scopable__ {
    using type = decltype(scopable_impl(std::declval<T>()));
};

template<class T>
using scopable = typename scopable__<T>::type;

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_CORE_DEPENDENCY_HPP
#define BOOST_DI_CORE_DEPENDENCY_HPP

#if defined(BOOST_DI_MSVC) || __has_include(<string>)
    #include <string>
#endif

namespace boost { namespace di { inline namespace v1 { namespace core {

BOOST_DI_HAS_METHOD(configure, configure);
BOOST_DI_HAS_TYPE(deps);

template<class T, class U = std::remove_reference_t<T>>
struct is_injector :
    std::integral_constant<bool, has_deps<U>::value || has_configure<U>::value>{};

template<class, class>
struct dependency_concept { };

template<class T, class TDependency>
struct dependency_impl
    : aux::pair<T, TDependency>
{ };

template<class... Ts, class TName, class TDependency>
struct dependency_impl<
    dependency_concept<aux::type_list<Ts...>, TName>
  , TDependency
> : aux::pair<dependency_concept<Ts, TName>, TDependency>...
{ };

struct override { };

struct dependency_base { };

template<
    class TScope
  , class TExpected
  , class TGiven = TExpected
  , class TName = no_name
  , class TPriority = aux::none_type
> struct dependency
    : private dependency_base
    , TScope::template scope<TExpected, TGiven>
    , dependency_impl<
          dependency_concept<TExpected, TName>
        , dependency<TScope, TExpected, TGiven, TName, TPriority>
      > {
private:
    template<class T>
    using is_not_narrowed = std::integral_constant<bool,
        (std::is_arithmetic<T>::value && std::is_same<TExpected, T>::value) || !std::is_arithmetic<T>::value
    >;

    template<class T>
    using externable = std::integral_constant<bool,
        !is_injector<T>::value &&
        std::is_same<TScope, scopes::deduce>::value &&
        std::is_same<TExpected, TGiven>::value &&
        is_not_narrowed<T>::value
    >;

    template<class T>
    struct str_traits {
        using type = T;
    };

    #if defined(BOOST_DI_MSVC) || __has_include(<string>)
        template<int N>
        struct str_traits<const char(&)[N]> {
            using type = std::string;
        };
    #endif

    template<class T>
    struct str_traits<std::shared_ptr<T>&> {
        using type = std::shared_ptr<T>;
    };

public:
    using creator = typename TScope::template scope<TExpected, TGiven>;
    using scope = TScope;
    using expected = TExpected;
    using given = std::remove_reference_t<TGiven>;
    using name = TName;
    using priority = TPriority;

    dependency() noexcept { }

    template<class T>
    explicit dependency(T&& object) noexcept
        : creator(static_cast<T&&>(object))
    { }

    template<
        class TScope_
      , class TExpected_
      , class TGiven_
      , class TName_
      , class TPriority_
    > dependency(const dependency<TScope_, TExpected_, TGiven_, TName_, TPriority_>& other) noexcept
        : creator(other)
    { }

    template<class T> // no requirements
    auto named(const T&) const noexcept {
        return dependency<TScope, TExpected, TGiven, T>{*this};
    }

    template<class T, BOOST_DI_REQUIRES(concepts::scopable<T>::value)>
    auto in(const T&) const noexcept {
        return dependency<T, TExpected, TGiven, TName>{};
    }

    template<class T, BOOST_DI_REQUIRES(externable<T>::value)>
    auto to(T&& object) const noexcept {
        using dependency = dependency<
            scopes::external, TExpected, typename str_traits<T>::type, TName
        >;
        return dependency{static_cast<T&&>(object)};
    }

    template<class T, BOOST_DI_REQUIRES(has_configure<T>::value)>
    auto to(const T& object) const noexcept {
        using dependency = dependency<
            scopes::exposed<TScope>, TExpected, decltype(std::declval<T>().configure()), TName
        >;
        return dependency{object.configure()};
    }

    template<class T, BOOST_DI_REQUIRES(has_deps<T>::value)>
    auto to(const T& object) const noexcept {
        using dependency = dependency<
            scopes::exposed<TScope>, TExpected, T, TName
        >;
        return dependency{object};
    }

    auto operator[](const override&) const noexcept {
        return dependency<TScope, TExpected, TGiven, TName, override>{*this};
    }

    #if defined(__cpp_variable_templates)
        const dependency& operator()() const noexcept {
            return *this;
        }
    #endif
};

template<class T>
struct is_dependency : std::is_base_of<dependency_base, T> { };

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CONCEPTS_CALLABLE_HPP
#define BOOST_DI_CONCEPTS_CALLABLE_HPP

namespace boost { namespace di { inline namespace v1 {

template<class...>
struct policy {
    struct is_not_callable { };
    struct is_not_configurable { };
};

namespace concepts {

struct arg {
    using type = void;
    using name = no_name;
    using is_root = std::false_type;

    template<class, class, class>
    struct resolve;
};

struct ctor { };

std::false_type callable_impl(...);

template<class T, class TArg>
auto callable_impl(T&& t, TArg&& arg) -> aux::is_valid_expr<
    decltype(t(arg))
>;

template<class T, class TArg, class TDependency, class... TCtor>
auto callable_impl(T&& t, TArg&& arg, TDependency&& dep, TCtor&&... ctor) -> aux::is_valid_expr<
    decltype(t(arg, dep, ctor...))
>;

template<class...>
struct is_callable_impl;

template<class T, class... Ts>
struct is_callable_impl<T, Ts...> {
    using callable_with_arg = decltype(callable_impl(std::declval<T>(), arg{}));
    using callable_with_arg_and_dep =
        decltype(callable_impl(std::declval<T>(), arg{}, core::dependency<scopes::deduce, T>{}, ctor{}));

    using type = std::conditional_t<
        callable_with_arg::value || callable_with_arg_and_dep::value
      , typename is_callable_impl<Ts...>::type
      , typename policy<T>::is_not_callable
    >;
};

template<>
struct is_callable_impl<>
    : std::true_type
{ };

template<class... Ts>
struct is_callable
    : is_callable_impl<Ts...>
{ };

template<class... Ts>
struct is_callable<core::pool<aux::type_list<Ts...>>>
    : is_callable_impl<Ts...>
{ };

template<>
struct is_callable<void> { // auto
    using type = policy<>::is_not_configurable;
};

template<class... Ts>
using callable = typename is_callable<Ts...>::type;

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_CONCEPTS_CREATABLE_HPP
#define BOOST_DI_CONCEPTS_CREATABLE_HPP

#define BOOST_DI_CONCEPTS_CREATABLE_ATTR \
    BOOST_DI_ATTR_ERROR("creatable constraint not satisfied")

namespace boost { namespace di { inline namespace v1 {

template<class T>
struct abstract_type {
struct is_not_bound {
    operator T*() const {
        using constraint_not_satisfied = is_not_bound;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "type not bound, did you forget to add: 'di::bind<interface, implementation>'?");
};

struct is_not_fully_implemented {
    operator T*() const {
        using constraint_not_satisfied = is_not_fully_implemented;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "type not implemented, did you forget to implement all interface methods?");
};

template<class TName>
struct named {
struct is_not_bound {
    operator T*() const {
        using constraint_not_satisfied = is_not_bound;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "type not bound, did you forget to add: 'di::bind<interface, implementation>.named(name)'?");
};

struct is_not_fully_implemented {
    operator T*() const {
        using constraint_not_satisfied = is_not_fully_implemented;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "type not implemented, did you forget to implement all interface methods?");
};

};};

template<class TParent>
struct when_creating {
    template<class T>
    struct type {
        template<class TName>
        struct named {
            static inline T
            error(_ = "reference type not bound, did you forget to add `auto value = ...; di::bind<T>.named(name).to(value)`");
        };

        static inline T
        error(_ = "reference type not bound, did you forget to add `auto value = ...; di::bind<T>.to(value)`");
    };
};

template<class TParent, class TName = no_name>
struct in_type {
    template<class T>
    using is_not_same = std::enable_if_t<!aux::is_same_or_base_of<T, TParent>::value>;

    template<class T, class = is_not_same<T>>
    operator T() {
        return {};
    }

    template<class T, class = is_not_same<T>>
    operator T&() const {
        using constraint_not_satisfied = typename when_creating<TParent>::template type<T&>::template named<TName>;
        return
            constraint_not_satisfied{}.error();
    }
};

template<class TParent>
struct in_type<TParent, no_name> {
    template<class T>
    using is_not_same = std::enable_if_t<!aux::is_same_or_base_of<T, TParent>::value>;

    template<class T, class = is_not_same<T>>
    operator T() {
        return {};
    }

    template<class T, class = is_not_same<T>>
    operator T&() const {
        using constraint_not_satisfied = typename when_creating<TParent>::template type<T&>;
        return
            constraint_not_satisfied{}.error();
    }
};

template<class...>
struct type;

template<class T, class>
struct in {
    using type = in_type<T>;
};

template<class TParent, class TName, class T>
struct in<TParent, detail::named_type<TName, T>> {
    using type = in_type<TParent, TName>;
};

template<class T, class TInitialization, class... TArgs, class... TCtor>
struct type<T, type_traits::direct, aux::pair<TInitialization, aux::type_list<TArgs...>>, TCtor...> {
    operator T*() const {
        return impl<T>();
    }

    template<class T_>
    auto impl() const {
        return new T{typename in<T_, TArgs>::type{}...};
    }
};

template<class T, class TInitialization, class... TArgs, class... TCtor>
struct type<T, type_traits::uniform, aux::pair<TInitialization, aux::type_list<TArgs...>>, TCtor...> {
    operator T*() const {
        return impl<T>(aux::is_braces_constructible<T, TCtor...>{});
    }

    template<class T_>
    auto impl(const std::true_type&) const {
        return new T_{typename in<T_, TArgs>::type{}...};
    }

    template<class T_>
    auto impl(const std::false_type&) const {
        return nullptr;
    }
};

template<class T>
struct type<T> {
template<class To>
struct is_not_convertible_to {
    operator To() const {
        using constraint_not_satisfied = is_not_convertible_to;
        return
            constraint_not_satisfied{}.error();
    }

    static inline To
    error(_ = "wrapper is not convertible to requested type, did you mistake the scope?");
};};

template<class T>
struct number_of_constructor_arguments_doesnt_match_for {
template<int Given> struct given {
template<int Expected> struct expected {
    operator T*() const {
        using constraint_not_satisfied = expected;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "verify BOOST_DI_INJECT_TRAITS or di::ctor_traits");
};};};

template<class T>
struct number_of_constructor_arguments_is_out_of_range_for {
template<int TMax> struct max {
    operator T*() const {
        using constraint_not_satisfied = max;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T*
    error(_ = "increase BOOST_DI_CFG_CTOR_LIMIT_SIZE value or reduce number of constructor parameters");
};};

namespace concepts {

template<class>
struct ctor_size;

template<class TInit, class... TCtor>
struct ctor_size<aux::pair<TInit, aux::type_list<TCtor...>>>
    : std::integral_constant<int, sizeof...(TCtor)>
{ };

template<class...>
struct creatable_error_impl;

template<class T>
using ctor_size_t = ctor_size<
    typename type_traits::ctor<
        aux::decay_t<T>
      , type_traits::ctor_impl_t<std::is_constructible, aux::decay_t<T>>
    >::type
>;

#define BOOST_DI_NAMED_ERROR(name, type, error_type, error) \
    std::conditional_t< \
        std::is_same<TName, no_name>::value \
      , typename error_type<aux::decay_t<type>>::error \
      , typename error_type<aux::decay_t<type>>::template named<TName>::error \
    >

template<class TInitialization, class TName, class I, class T, class... TCtor>
struct creatable_error_impl<TInitialization, TName, I, T, aux::type_list<TCtor...>>
    : std::conditional_t<
          std::is_abstract<aux::decay_t<T>>::value
        , std::conditional_t<
              std::is_same<I, T>::value
            , BOOST_DI_NAMED_ERROR(TName, T, abstract_type, is_not_bound)
            , BOOST_DI_NAMED_ERROR(TName, T, abstract_type, is_not_fully_implemented)
          >
        , std::conditional_t<
              ctor_size_t<T>::value == sizeof...(TCtor)
            , std::conditional_t<
                  !sizeof...(TCtor)
                , typename number_of_constructor_arguments_is_out_of_range_for<aux::decay_t<T>>::
                      template max<BOOST_DI_CFG_CTOR_LIMIT_SIZE>
                , type<
                      aux::decay_t<T>
                    , TInitialization
                    , typename type_traits::ctor_traits<aux::decay_t<T>>::type, TCtor...
                  >
              >
            , typename number_of_constructor_arguments_doesnt_match_for<aux::decay_t<T>>::
                  template given<sizeof...(TCtor)>::
                  template expected<ctor_size_t<T>::value>
          >
      >
{ };

#undef BOOST_DI_NAMED_ERROR

template<class TInit, class T, class... TArgs>
struct creatable {
    static constexpr auto value = std::is_constructible<T, TArgs...>::value;
};

template<class T, class... TArgs>
struct creatable<type_traits::uniform, T, TArgs...> {
    static constexpr auto value = aux::is_braces_constructible<T, TArgs...>::value;
};

template<class TInitialization, class TName, class I, class T, class... TArgs>
T creatable_error() {
    return creatable_error_impl<TInitialization, TName, I, T, aux::type_list<TArgs...>>{};
}

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_PROVIDERS_STACK_OVER_HEAP_HPP
#define BOOST_DI_PROVIDERS_STACK_OVER_HEAP_HPP

namespace boost { namespace di { inline namespace v1 { namespace providers {

class stack_over_heap {
public:
    template<class TInitialization, class TMemory, class T, class... TArgs>
    struct is_creatable {
        static constexpr auto value =
            concepts::creatable<TInitialization, T, TArgs...>::value;
    };

    template<class, class T, class... TArgs>
    auto get(const type_traits::direct&
           , const type_traits::heap&
           , TArgs&&... args) {
        return new T(static_cast<TArgs&&>(args)...);
    }

    template<class, class T, class... TArgs>
    auto get(const type_traits::uniform&
           , const type_traits::heap&
           , TArgs&&... args) {
        return new T{static_cast<TArgs&&>(args)...};
    }

    template<class, class T, class... TArgs>
    auto get(const type_traits::direct&
           , const type_traits::stack&
           , TArgs&&... args) const noexcept {
        return T(static_cast<TArgs&&>(args)...);
    }

    template<class, class T, class... TArgs>
    auto get(const type_traits::uniform&
           , const type_traits::stack&
           , TArgs&&... args) const noexcept {
        return T{static_cast<TArgs&&>(args)...};
    }
};

}}}} // boost::di::v1::providers

#endif

#ifndef BOOST_DI_CONFIG_HPP
#define BOOST_DI_CONFIG_HPP

#if defined(BOOST_DI_CFG)
    class BOOST_DI_CFG;
#else
    #define BOOST_DI_CFG boost::di::config
#endif

namespace boost { namespace di { inline namespace v1 {

template<class... TPolicies, BOOST_DI_REQUIRES_MSG(concepts::callable<TPolicies...>)>
inline auto make_policies(const TPolicies&... args) noexcept {
    return core::pool_t<TPolicies...>(args...);
}

class config {
public:
    template<class T>
    static auto provider(const T&) noexcept {
        return providers::stack_over_heap{};
    }

    template<class T>
    static auto policies(const T&) noexcept {
        return make_policies();
    }
};

}}} // boost::di::v1

#endif

#ifndef BOOST_DI_CORE_TRANSFORM_HPP
#define BOOST_DI_CORE_TRANSFORM_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

template<class T, class = void>
struct get_deps {
    using type = typename T::deps;
};

template<class T>
struct get_deps<T, std::enable_if_t<has_configure<T>::value>> {
    using result_type = typename aux::function_traits<
        decltype(&T::configure)
    >::result_type;

    using type = typename result_type::deps;
};

template<
    class T
  , class = typename is_injector<T>::type
  , class = typename is_dependency<T>::type
> struct add_type_list;

template<class T, class TAny>
struct add_type_list<T, std::true_type, TAny> {
    using type = typename get_deps<T>::type;
};

template<class T>
struct add_type_list<T, std::false_type, std::true_type> {
    using type = aux::type_list<T>;
};

template<class T>
struct add_type_list<T, std::false_type, std::false_type> {
    using type = aux::type_list<dependency<scopes::exposed<>, T>>;
};

#if defined(BOOST_DI_MSVC)
    template<class... Ts>
    struct transform : aux::join_t<typename add_type_list<Ts>::type...> { };

    template<class... Ts>
    using transform_t = typename transform<Ts...>::type;
#else
    template<class... Ts>
    using transform_t = aux::join_t<typename add_type_list<Ts>::type...>;
#endif

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CONCEPTS_BOUNDABLE_HPP
#define BOOST_DI_CONCEPTS_BOUNDABLE_HPP

namespace boost { namespace di { inline namespace v1 {

template<class...>
struct bound_type {
    struct is_bound_more_than_once { };
    struct is_neither_a_dependency_nor_an_injector { };

    template<class>
    struct is_not_base_of { };

    template<class>
    struct is_not_convertible_to { };
};

namespace concepts {

template<class... TDeps>
using is_supported =
    std::is_same<
       aux::bool_list<aux::always<TDeps>::value...>
     , aux::bool_list<(core::is_injector<TDeps>::value || core::is_dependency<TDeps>::value)...>
    >;

template<class...>
struct get_not_supported;

template<class T>
struct get_not_supported<T> {
    using type = T;
};

template<class T, class... TDeps>
struct get_not_supported<T, TDeps...>
    : std::conditional<
          core::is_injector<T>::value || core::is_dependency<T>::value
        , typename get_not_supported<TDeps...>::type
        , T
      >
{ };

template<class>
struct is_unique;

template<class T>
struct unique_dependency {
    using type = aux::pair<
        aux::pair<typename T::expected, typename T::name>
      , typename T::priority
    >;
};

template<class... TDeps>
struct is_unique<aux::type_list<TDeps...>>
    : aux::is_unique<typename unique_dependency<TDeps>::type...>
{ };

template<class>
struct get_is_unique_error_impl
    : std::true_type
{ };

template<class T, class TName, class TPriority>
struct get_is_unique_error_impl<aux::not_unique<aux::pair<aux::pair<T, TName>, TPriority>>> {
    using type = typename bound_type<T, TName>::is_bound_more_than_once;
};

template<class T>
struct get_is_unique_error_impl<aux::not_unique<T>> {
    using type = typename bound_type<T>::is_bound_more_than_once;
};

template<class>
struct get_is_unique_error;

template<class... TDeps>
struct get_is_unique_error<aux::type_list<TDeps...>>
    : get_is_unique_error_impl<typename aux::is_unique<typename unique_dependency<TDeps>::type...>::type>
{ };

template<class... TDeps>
using get_bindings_error =
    std::conditional_t<
        is_supported<TDeps...>::value
      , typename get_is_unique_error<core::transform_t<TDeps...>>::type
      , typename bound_type<typename get_not_supported<TDeps...>::type>::
            is_neither_a_dependency_nor_an_injector
    >;

template<class... Ts>
using get_any_of_error =
    std::conditional_t<
        std::is_same<
            aux::bool_list<aux::always<Ts>::value...>
          , aux::bool_list<std::is_same<std::true_type, Ts>::value...>
        >::value
      , std::true_type
      , aux::type_list<Ts...>
    >;

template<class I, class T> // expected -> given
auto boundable_impl(I&&, T&&) ->
    std::conditional_t<
        std::is_base_of<I, T>::value || std::is_convertible<T, I>::value
      , std::true_type
      , std::conditional_t<
            std::is_base_of<I, T>::value
          , typename bound_type<T>::template is_not_convertible_to<I>
          , typename bound_type<T>::template is_not_base_of<I>
        >
    >;

template<class... TDeps> // bindings
auto boundable_impl(aux::type_list<TDeps...>&&) -> get_bindings_error<TDeps...>;

template<class T, class... Ts> // any_of
auto boundable_impl(aux::type_list<Ts...>&&, T&&) ->
    get_any_of_error<decltype(boundable_impl(std::declval<Ts>(), std::declval<T>()))...>;

template<class... TDeps> // injector
auto boundable_impl(aux::type<TDeps...>&&) ->
    typename get_is_unique_error_impl<typename aux::is_unique<TDeps...>::type>::type;

std::true_type boundable_impl(...);

template<class... Ts>
struct boundable__ {
    using type = decltype(boundable_impl(std::declval<Ts>()...));
};

template<class... Ts>
using boundable = typename boundable__<Ts...>::type;

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_BINDINGS_HPP
#define BOOST_DI_BINDINGS_HPP

namespace boost { namespace di { inline namespace v1 {

namespace detail {
template<class... Ts, BOOST_DI_REQUIRES(aux::is_unique<Ts...>::value)>
auto any_of() {
    return aux::type_list<Ts...>{};
}
} // namespace detail

template<class T1, class T2, class... Ts>
using any_of = decltype(detail::any_of<T1, T2, Ts...>());

template<
    class TExpected
  , class TGiven = TExpected
  , BOOST_DI_REQUIRES_MSG(concepts::boundable<TExpected, TGiven>)
>
#if defined(__cpp_variable_templates)
    core::dependency<scopes::deduce, TExpected, TGiven> bind{};
#else
    struct bind : core::dependency<scopes::deduce, TExpected, TGiven> {};
#endif

static constexpr BOOST_DI_UNUSED core::override override{};
static constexpr BOOST_DI_UNUSED scopes::deduce deduce{};
static constexpr BOOST_DI_UNUSED scopes::unique unique{};
static constexpr BOOST_DI_UNUSED scopes::singleton singleton{};

}}} // boost::di::v1

#endif

#ifndef BOOST_DI_CORE_BINDER_HPP
#define BOOST_DI_CORE_BINDER_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

class binder {
    template<class TDefault, class>
    static TDefault resolve_impl(...) noexcept {
        return {};
    }

    template<class, class TConcept, class TDependency>
    static decltype(auto)
    resolve_impl(aux::pair<TConcept, TDependency>* dep) noexcept {
        return static_cast<TDependency&>(*dep);
    }

    template<
        class
      , class TConcept
      , class TScope
      , class TExpected
      , class TGiven
      , class TName
    > static decltype(auto)
    resolve_impl(aux::pair<TConcept
               , dependency<TScope, TExpected, TGiven, TName, override>>* dep) noexcept {
        return static_cast<dependency<TScope, TExpected, TGiven, TName, override>&>(*dep);
    }

public:
    template<
        class T
      , class TName = no_name
      , class TDefault = dependency<scopes::deduce, aux::decay_t<T>>
      , class TDeps = void
    > static decltype(auto) resolve(TDeps* deps) noexcept {
        using dependency = dependency_concept<aux::decay_t<T>, TName>;
        return resolve_impl<TDefault, dependency>(deps);
    }
};

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CORE_ANY_TYPE_HPP
#define BOOST_DI_CORE_ANY_TYPE_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

template<class T, class TParent>
using is_not_same_t = std::enable_if_t<!aux::is_same_or_base_of<T, TParent>::value>;

template<class T, class TInjector>
struct is_referable_impl {
    static constexpr auto value =
        std::remove_reference_t<decltype(binder::resolve<T>((TInjector*)nullptr))>::template
            is_referable<T>::value;
};

template<class T, class TInjector>
using is_referable_t = std::enable_if_t<is_referable_impl<T, TInjector>::value>;

template<class T, class TInjector, class TError>
struct is_creatable_impl {
    static constexpr auto value = TInjector::template is_creatable<T>::value;
};

template<class T, class TInjector>
struct is_creatable_impl<T, TInjector, std::false_type> {
    static constexpr auto value = true;
};

template<class T, class TInjector, class TError>
using is_creatable_t = std::enable_if_t<is_creatable_impl<T, TInjector, TError>::value>;

template<class TParent, class TInjector, class TError = std::false_type>
struct any_type {
    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_creatable_t<T, TInjector, TError>
    > operator T() {
        return injector_.create_impl(aux::type<T>{});
    }

    const TInjector& injector_;
};

template<class TParent, class TInjector, class TError = std::false_type>
struct any_type_ref {
    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_creatable_t<T, TInjector, TError>
    > operator T() {
        return injector_.create_impl(aux::type<T>{});
    }

    #if defined(BOOST_DI_GCC)
        template<class T
               , class = is_not_same_t<T, TParent>
               , class = is_referable_t<T&&, TInjector>
               , class = is_creatable_t<T&&, TInjector, TError>
        > operator T&&() const {
            return injector_.create_impl(aux::type<T&&>{});
        }
    #endif

    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_referable_t<T&, TInjector>
           , class = is_creatable_t<T&, TInjector, TError>
    > operator T&() const {
        return injector_.create_impl(aux::type<T&>{});
    }

    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_referable_t<const T&, TInjector>
           , class = is_creatable_t<const T&, TInjector, TError>
    > operator const T&() const {
        return injector_.create_impl(aux::type<const T&>{});
    }

    const TInjector& injector_;
};

namespace successful {

template<class TParent, class TInjector>
struct any_type {
    template<class T, class = is_not_same_t<T, TParent>>
    operator T() {
        return injector_.create_successful_impl(aux::type<T>{});
    }

    const TInjector& injector_;
};

template<class TParent, class TInjector>
struct any_type_ref {
    template<class T, class = is_not_same_t<T, TParent>>
    operator T() {
        return injector_.create_successful_impl(aux::type<T>{});
    }

    #if defined(BOOST_DI_GCC)
        template<class T
               , class = is_not_same_t<T, TParent>
               , class = is_referable_t<T&&, TInjector>
        > operator T&&() const {
            return injector_.create_successful_impl(aux::type<T&&>{});
        }
    #endif

    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_referable_t<T&, TInjector>
    > operator T&() const {
        return injector_.create_successful_impl(aux::type<T&>{});
    }

    template<class T
           , class = is_not_same_t<T, TParent>
           , class = is_referable_t<const T&, TInjector>
    > operator const T&() const {
        return injector_.create_successful_impl(aux::type<const T&>{});
    }

    const TInjector& injector_;
};

} // successful

template<class TParent>
struct any_type_fwd {
    template<class T, class = is_not_same_t<T, TParent>>
    operator T();
};

template<class TParent>
struct any_type_ref_fwd {
    template<class T, class = is_not_same_t<T, TParent>>
    operator T();

    template<class T, class = is_not_same_t<T, TParent>>
    operator T&() const;

    #if defined(BOOST_DI_GCC)
        template<class T, class = is_not_same_t<T, TParent>>
        operator T&&() const;
    #endif

    template<class T, class = is_not_same_t<T, TParent>>
    operator const T&() const;
};

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CORE_COPYABLE_HPP
#define BOOST_DI_CORE_COPYABLE_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

template<class>
struct copyable_impl;

template<class T>
struct to_be_copied : std::conditional<
    std::is_default_constructible<typename T::creator>::value
  , aux::type_list<>
  , aux::type_list<T>
> { };

template<class... TDeps>
struct copyable_impl<aux::type_list<TDeps...>>
    : aux::join<typename to_be_copied<TDeps>::type...>
{ };

template<class TDeps>
using copyable = typename copyable_impl<TDeps>::type;

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CORE_POLICY_HPP
#define BOOST_DI_CORE_POLICY_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

BOOST_DI_HAS_METHOD(call_operator, operator());

template<
    class T
  , class TName
  , class TIsRoot
  , class TDeps
  , class TIgnore = std::false_type
> struct arg_wrapper {
    using type BOOST_DI_UNUSED = T;
    using name BOOST_DI_UNUSED = TName;
    using is_root BOOST_DI_UNUSED = TIsRoot;
    using ignore = TIgnore;

    template<class T_, class TName_, class TDefault_>
    using resolve = decltype(core::binder::resolve<T_, TName_, TDefault_>((TDeps*)0));
};

template<class T>
struct allow_void : T { };

template<>
struct allow_void<void> : std::true_type { };

class policy {
    template<class TArg, class TPolicy, class TPolicies, class TDependency, class TCtor>
    static void call_impl(const TPolicies& policies, TDependency& dependency, const TCtor& ctor) noexcept {
        call_impl__<TArg>(static_cast<const TPolicy&>(policies), dependency, ctor);
    }

    template<class TArg, class TPolicy, class TDependency, class TCtor>
    static void call_impl__(const TPolicy& policy, TDependency& dependency, const TCtor& ctor) noexcept {
        call_impl_args<TArg>(policy, dependency, ctor);
    }

    template<class TArg, class TDependency, class TPolicy, class TInitialization, class... TCtor
           , BOOST_DI_REQUIRES(!has_call_operator<TPolicy, TArg, TDependency&, TCtor...>::value)
    > static void call_impl_args(const TPolicy& policy
                               , TDependency&
                               , const aux::pair<TInitialization, aux::type_list<TCtor...>>&) noexcept {
        (policy)(TArg{});
    }

    template<class TArg, class TDependency, class TPolicy, class TInitialization, class... TCtor
           , BOOST_DI_REQUIRES(has_call_operator<TPolicy, TArg, TDependency&, TCtor...>::value)>
    static void call_impl_args(const TPolicy& policy
                             , TDependency& dependency
                             , const aux::pair<TInitialization, aux::type_list<TCtor...>>&) noexcept {
        (policy)(TArg{}, dependency, aux::type<TCtor>{}...);
    }

    template<class, class, class, class, class = void>
    struct try_call_impl;

    template<class TArg, class TPolicy, class TDependency, class TInitialization, class... TCtor>
    struct try_call_impl<TArg, TPolicy, TDependency, aux::pair<TInitialization, aux::type_list<TCtor...>>
                       , BOOST_DI_REQUIRES_T(!has_call_operator<TPolicy, TArg, TDependency, TCtor...>::value)>
        : allow_void<decltype((std::declval<TPolicy>())(std::declval<TArg>()))>
    { };

    template<class TArg, class TPolicy, class TDependency, class TInitialization, class... TCtor>
    struct try_call_impl<TArg, TPolicy, TDependency, aux::pair<TInitialization, aux::type_list<TCtor...>>
                       , BOOST_DI_REQUIRES_T(has_call_operator<TPolicy, TArg, TDependency, TCtor...>::value)>
        : allow_void<decltype((std::declval<TPolicy>())(std::declval<TArg>(), std::declval<TDependency>(), aux::type<TCtor>{}...))>
    { };

public:
    template<class, class, class, class>
    struct try_call;

    template<class TArg, class TDependency, class TCtor, class... TPolicies>
    struct try_call<TArg, pool_t<TPolicies...>, TDependency, TCtor>
        : std::is_same<
            aux::bool_list<aux::always<TPolicies>::value...>
          , aux::bool_list<try_call_impl<TArg, TPolicies, TDependency, TCtor>::value...>
        >
    { };

    template<class TArg, class TDependency, class TCtor, class... TPolicies>
    static void call(BOOST_DI_UNUSED const pool_t<TPolicies...>& policies
                   , BOOST_DI_UNUSED TDependency& dependency
                   , BOOST_DI_UNUSED const TCtor& ctor) noexcept {
        int _[]{0, (call_impl<TArg, TPolicies>(policies, dependency, ctor), 0)...}; (void)_;
    }
};

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CORE_PROVIDER_HPP
#define BOOST_DI_CORE_PROVIDER_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

template<class, class, class, class>
struct try_provider;

template<
    class TGiven
  , class TInjector
  , class TProvider
  , class TInitialization
  , class... TCtor
> struct try_provider<TGiven, aux::pair<TInitialization, aux::type_list<TCtor...>>, TInjector, TProvider> {
    template<class TMemory>
    struct is_creatable {
        static constexpr auto value =
            TProvider::template is_creatable<
                TInitialization
              , TMemory
              , TGiven
              , typename TInjector::template try_create<TCtor>::type...
            >::value;
    };

    template<class TMemory = type_traits::heap>
    auto get(const TMemory& memory = {}) const -> std::enable_if_t<
        is_creatable<TMemory>::value
      , std::conditional_t<std::is_same<TMemory, type_traits::stack>::value, TGiven, TGiven*>
    >;
};

template<class, class, class, class, class>
struct provider;

template<
    class TExpected
  , class TGiven
  , class TName
  , class TInjector
  , class TInitialization
  , class... TCtor
> struct provider<TExpected, TGiven, TName, aux::pair<TInitialization, aux::type_list<TCtor...>>, TInjector> {
    using provider_t = decltype(TInjector::config::provider(std::declval<TInjector>()));

    template<class TMemory, class... TArgs>
    struct is_creatable {
        static constexpr auto value =
            provider_t::template is_creatable<TInitialization, TMemory, TGiven, TArgs...>::value;
    };

    template<class TMemory = type_traits::heap>
    auto get(const TMemory& memory = {}) const {
        return get_impl(memory, injector_.create_impl(aux::type<TCtor>{})...);
    }

    template<class TMemory, class... TArgs, BOOST_DI_REQUIRES(is_creatable<TMemory, TArgs...>::value)>
    auto get_impl(const TMemory& memory, TArgs&&... args) const {
        return TInjector::config::provider(injector_).template get<TExpected, TGiven>(
            TInitialization{}
          , memory
          , static_cast<TArgs&&>(args)...
        );
    }

    template<class TMemory, class... TArgs, BOOST_DI_REQUIRES(!is_creatable<TMemory, TArgs...>::value)>
    auto get_impl(const TMemory&, TArgs&&...) const {
        return concepts::creatable_error<TInitialization, TName, TExpected*, TGiven*, TArgs...>();
    }

    const TInjector& injector_;
};

namespace successful {

template<class, class, class, class>
struct provider;

template<
    class TExpected
  , class TGiven
  , class TInjector
  , class TInitialization
  , class... TCtor
> struct provider<TExpected, TGiven, aux::pair<TInitialization, aux::type_list<TCtor...>>, TInjector> {
    template<class TMemory = type_traits::heap>
    auto get(const TMemory& memory = {}) const {
        return TInjector::config::provider(injector_).template get<TExpected, TGiven>(
            TInitialization{}
          , memory
          , injector_.create_successful_impl(aux::type<TCtor>{})...
        );
    }

    const TInjector& injector_;
};

} // successful

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CORE_WRAPPER_HPP
#define BOOST_DI_CORE_WRAPPER_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

namespace successful {

template<class T, class TWrapper>
struct wrapper {
    using element_type = T;

    inline operator T() const noexcept {
        return wrapper_;
    }

    inline operator T() noexcept {
        return wrapper_;
    }

    TWrapper wrapper_;
};

} // successful

template<class T, class TWrapper, class = void>
struct wrapper_impl {
    using element_type = T;

    inline operator T() const noexcept {
        return wrapper_;
    }

    inline operator T() noexcept {
        return wrapper_;
    }

    TWrapper wrapper_;
};

template<class T, class TWrapper>
struct wrapper_impl<T, TWrapper, BOOST_DI_REQUIRES_T(!std::is_convertible<TWrapper, T>::value)> {
    using element_type = T;

    inline operator T() const noexcept {
        return typename type<TWrapper>::template is_not_convertible_to<T>{};
    }

    inline operator T() noexcept {
        return typename type<TWrapper>::template is_not_convertible_to<T>{};
    }

    TWrapper wrapper_;
};

template<class T, class TWrapper>
using wrapper = wrapper_impl<T, TWrapper>;

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_TYPE_TRAITS_REFERABLE_TRAITS_HPP
#define BOOST_DI_TYPE_TRAITS_REFERABLE_TRAITS_HPP

namespace boost { namespace di { inline namespace v1 { namespace type_traits {

template<class T, class>
struct referable_traits {
    using type = T;
};

template<class T, class TDependency>
struct referable_traits<T&, TDependency> {
    using type = std::conditional_t<TDependency::template is_referable<T&>::value, T&, T>;
};

template<class T, class TDependency>
struct referable_traits<const T&, TDependency> {
    using type = std::conditional_t<TDependency::template is_referable<const T&>::value, const T&, T>;
};

#if defined(BOOST_DI_MSVC)
    template<class T, class TDependency>
    struct referable_traits<T&&, TDependency> {
        using type = std::conditional_t<TDependency::template is_referable<T&&>::value, T&&, T>;
    };
#endif

template<class T, class TDependency>
using referable_traits_t = typename referable_traits<T, TDependency>::type;

}}}} // boost::di::v1::type_traits

#endif

#ifndef BOOST_DI_CORE_INJECTOR_HPP
#define BOOST_DI_CORE_INJECTOR_HPP

namespace boost { namespace di { inline namespace v1 { namespace core {

BOOST_DI_HAS_METHOD(call, call);

struct from_injector { };
struct from_deps { };
struct init { };
struct with_error { };

#if defined(BOOST_DI_MSVC)
    template<class T, class TInjector>
    inline auto build(const TInjector& injector) noexcept {
        return T{injector};
    }
#endif

template<class T>
inline decltype(auto) get_arg(const T& arg, const std::false_type&) noexcept {
    return arg;
}

template<class T>
inline decltype(auto) get_arg(const T& arg, const std::true_type&) noexcept {
    return arg.configure();
}

#define BOOST_DI_CORE_INJECTOR_POLICY(...) __VA_ARGS__ BOOST_DI_CORE_INJECTOR_POLICY_ELSE
#define BOOST_DI_CORE_INJECTOR_POLICY_ELSE(...)
template<class TConfig BOOST_DI_CORE_INJECTOR_POLICY(, class TPolicies = pool<>)(), class... TDeps>
class injector BOOST_DI_CORE_INJECTOR_POLICY()(<TConfig, pool<>, TDeps...>)
    : pool<transform_t<TDeps...>> {
    friend class binder;
    template<class> friend class pool;
    template<class> friend class scopes::exposed;
    template<class, class, class> friend struct any_type;
    template<class, class> friend struct successful::any_type;
    template<class, class, class> friend struct any_type_ref;
    template<class, class> friend struct successful::any_type_ref;
    template<class, class, class, class> friend struct try_provider;
    template<class, class, class, class, class> friend struct provider;
    template<class, class, class, class> friend struct successful::provider;

    using pool_t = pool<transform_t<TDeps...>>;
    using is_root_t = std::true_type;

public:
    using deps = transform_t<TDeps...>;
    using config = TConfig;

    template<class T, class TName = no_name, class TIsRoot = std::false_type>
    struct is_creatable {
        using TDependency = std::remove_reference_t<decltype(binder::resolve<T, TName>((injector*)0))>;
        using TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type;

        static constexpr auto value = std::is_convertible<
            decltype(
                std::declval<TDependency>().template try_create<T>(
                    try_provider<
                        typename TDependency::given
                      , TCtor
                      , injector
                      , decltype(TConfig::provider(std::declval<injector>()))
                    >{}
                )
            ), T>::value BOOST_DI_CORE_INJECTOR_POLICY(&&
            policy::template try_call<
                arg_wrapper<type_traits::referable_traits_t<T, TDependency>, TName, TIsRoot, pool_t>
              , TPolicies
              , TDependency
              , TCtor
            >::value)();
    };

    template<class... TArgs>
    explicit injector(const init&, const TArgs&... args) noexcept
        : injector{from_deps{}, get_arg(args, has_configure<decltype(args)>{})...}
    { }

    template<class TConfig_, class TPolicies_, class... TDeps_>
    explicit injector(const injector<TConfig_, TPolicies_, TDeps_...>& other) noexcept
        : injector{from_injector{}, other, deps{}}
    { }

    template<class T, BOOST_DI_REQUIRES(is_creatable<T, no_name, is_root_t>::value)>
    T create() const {
        return create_successful_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!is_creatable<T, no_name, is_root_t>::value)>
    BOOST_DI_CONCEPTS_CREATABLE_ATTR
    T create() const {
        return create_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!has_deps<T>::value)>
    operator T() const {
        return create<T>();
    }

private:
    template<class T>
    struct try_create {
        using type = std::conditional_t<is_creatable<T>::value, T, void>;
    };

    template<class TParent>
    struct try_create<any_type_fwd<TParent>> {
        using type = any_type<TParent, injector, with_error>;
    };

    template<class TParent>
    struct try_create<any_type_ref_fwd<TParent>> {
        using type = any_type_ref<TParent, injector, with_error>;
    };

    template<class TName, class T>
    struct try_create<detail::named_type<TName, T>> {
        using type = std::conditional_t<is_creatable<T, TName>::value, T, void>;
    };

    template<class... TArgs>
    explicit injector(const from_deps&, const TArgs&... args) noexcept
        : pool_t{copyable<deps>{}, core::pool_t<TArgs...>{args...}}
    { }

    template<class TInjector, class... TArgs>
    explicit injector(const from_injector&, const TInjector& injector, const aux::type_list<TArgs...>&) noexcept
    #if defined(BOOST_DI_MSVC)
        : pool_t{copyable<deps>{}, pool_t{build<TArgs>(injector)...}}
    #else
        : pool_t{copyable<deps>{}, pool_t{TArgs{injector}...}}
    #endif
    { }

    template<class TIsRoot = std::false_type, class T>
    auto create_impl(const aux::type<T>&) const {
        return create_impl__<TIsRoot, T>();
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_impl(const aux::type<detail::named_type<TName, T>>&) const {
        return create_impl__<TIsRoot, T, TName>();
    }

    template<class TIsRoot = std::false_type, class T, class TName = no_name>
    auto create_impl__() const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = core::provider<expected_t, given_t, TName, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        BOOST_DI_CORE_INJECTOR_POLICY(
            policy::template call<arg_wrapper<create_t, TName, TIsRoot, pool_t, std::true_type>>(
                TConfig::policies(*this), dependency, ctor_t{}
            );
        )()
        return wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class T>
    auto create_successful_impl(const aux::type<T>&) const {
        return create_successful_impl__<TIsRoot, T>();
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return successful::any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return successful::any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_successful_impl(const aux::type<detail::named_type<TName, T>>&) const {
        return create_successful_impl__<TIsRoot, T, TName>();
    }

    template<class TIsRoot = std::false_type, class T, class TName = no_name>
    auto create_successful_impl__() const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        BOOST_DI_CORE_INJECTOR_POLICY(
            policy::template call<arg_wrapper<create_t, TName, TIsRoot, pool_t, std::true_type>>(
                TConfig::policies(*this), dependency, ctor_t{}
            );
        )()
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }
};
#undef BOOST_DI_CORE_INJECTOR_POLICY
#undef BOOST_DI_CORE_INJECTOR_POLICY_ELSE

#define BOOST_DI_CORE_INJECTOR_POLICY(...) BOOST_DI_CORE_INJECTOR_POLICY_ELSE
#define BOOST_DI_CORE_INJECTOR_POLICY_ELSE(...) __VA_ARGS__
template<class TConfig BOOST_DI_CORE_INJECTOR_POLICY(, class TPolicies = pool<>)(), class... TDeps>
class injector BOOST_DI_CORE_INJECTOR_POLICY()(<TConfig, pool<>, TDeps...>)
    : pool<transform_t<TDeps...>> {
    friend class binder;
    template<class> friend class pool;
    template<class> friend class scopes::exposed;
    template<class, class, class> friend struct any_type;
    template<class, class> friend struct successful::any_type;
    template<class, class, class> friend struct any_type_ref;
    template<class, class> friend struct successful::any_type_ref;
    template<class, class, class, class> friend struct try_provider;
    template<class, class, class, class, class> friend struct provider;
    template<class, class, class, class> friend struct successful::provider;

    using pool_t = pool<transform_t<TDeps...>>;
    using is_root_t = std::true_type;

public:
    using deps = transform_t<TDeps...>;
    using config = TConfig;

    template<class T, class TName = no_name, class TIsRoot = std::false_type>
    struct is_creatable {
        using TDependency = std::remove_reference_t<decltype(binder::resolve<T, TName>((injector*)0))>;
        using TCtor = typename type_traits::ctor_traits<typename TDependency::given>::type;

        static constexpr auto value = std::is_convertible<
            decltype(
                std::declval<TDependency>().template try_create<T>(
                    try_provider<
                        typename TDependency::given
                      , TCtor
                      , injector
                      , decltype(TConfig::provider(std::declval<injector>()))
                    >{}
                )
            ), T>::value BOOST_DI_CORE_INJECTOR_POLICY(&&
            policy::template try_call<
                arg_wrapper<type_traits::referable_traits_t<T, TDependency>, TName, TIsRoot, pool_t>
              , TPolicies
              , TDependency
              , TCtor
            >::value)();
    };

    template<class... TArgs>
    explicit injector(const init&, const TArgs&... args) noexcept
        : injector{from_deps{}, get_arg(args, has_configure<decltype(args)>{})...}
    { }

    template<class TConfig_, class TPolicies_, class... TDeps_>
    explicit injector(const injector<TConfig_, TPolicies_, TDeps_...>& other) noexcept
        : injector{from_injector{}, other, deps{}}
    { }

    template<class T, BOOST_DI_REQUIRES(is_creatable<T, no_name, is_root_t>::value)>
    T create() const {
        return create_successful_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!is_creatable<T, no_name, is_root_t>::value)>
    BOOST_DI_CONCEPTS_CREATABLE_ATTR
    T create() const {
        return create_impl<is_root_t>(aux::type<T>{});
    }

    template<class T, BOOST_DI_REQUIRES(!has_deps<T>::value)>
    operator T() const {
        return create<T>();
    }

private:
    template<class T>
    struct try_create {
        using type = std::conditional_t<is_creatable<T>::value, T, void>;
    };

    template<class TParent>
    struct try_create<any_type_fwd<TParent>> {
        using type = any_type<TParent, injector, with_error>;
    };

    template<class TParent>
    struct try_create<any_type_ref_fwd<TParent>> {
        using type = any_type_ref<TParent, injector, with_error>;
    };

    template<class TName, class T>
    struct try_create<detail::named_type<TName, T>> {
        using type = std::conditional_t<is_creatable<T, TName>::value, T, void>;
    };

    template<class... TArgs>
    explicit injector(const from_deps&, const TArgs&... args) noexcept
        : pool_t{copyable<deps>{}, core::pool_t<TArgs...>{args...}}
    { }

    template<class TInjector, class... TArgs>
    explicit injector(const from_injector&, const TInjector& injector, const aux::type_list<TArgs...>&) noexcept
    #if defined(BOOST_DI_MSVC)
        : pool_t{copyable<deps>{}, pool_t{build<TArgs>(injector)...}}
    #else
        : pool_t{copyable<deps>{}, pool_t{TArgs{injector}...}}
    #endif
    { }

    template<class TIsRoot = std::false_type, class T>
    auto create_impl(const aux::type<T>&) const {
        return create_impl__<TIsRoot, T>();
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_impl(const aux::type<detail::named_type<TName, T>>&) const {
        return create_impl__<TIsRoot, T, TName>();
    }

    template<class TIsRoot = std::false_type, class T, class TName = no_name>
    auto create_impl__() const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = core::provider<expected_t, given_t, TName, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        BOOST_DI_CORE_INJECTOR_POLICY(
            policy::template call<arg_wrapper<create_t, TName, TIsRoot, pool_t, std::true_type>>(
                TConfig::policies(*this), dependency, ctor_t{}
            );
        )()
        return wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }

    template<class TIsRoot = std::false_type, class T>
    auto create_successful_impl(const aux::type<T>&) const {
        return create_successful_impl__<TIsRoot, T>();
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_fwd<TParent>>&) const {
        return successful::any_type<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class TParent>
    auto create_successful_impl(const aux::type<any_type_ref_fwd<TParent>>&) const {
        return successful::any_type_ref<TParent, injector>{*this};
    }

    template<class TIsRoot = std::false_type, class T, class TName>
    auto create_successful_impl(const aux::type<detail::named_type<TName, T>>&) const {
        return create_successful_impl__<TIsRoot, T, TName>();
    }

    template<class TIsRoot = std::false_type, class T, class TName = no_name>
    auto create_successful_impl__() const {
        auto&& dependency = binder::resolve<T, TName>((injector*)this);
        using dependency_t = std::remove_reference_t<decltype(dependency)>;
        using expected_t = typename dependency_t::expected;
        using given_t = typename dependency_t::given;
        using ctor_t = typename type_traits::ctor_traits<given_t>::type;
        using provider_t = successful::provider<expected_t, given_t, ctor_t, injector>;
        using wrapper_t = decltype(dependency.template create<T>(provider_t{*this}));
        using create_t = type_traits::referable_traits_t<T, dependency_t>;
        BOOST_DI_CORE_INJECTOR_POLICY(
            policy::template call<arg_wrapper<create_t, TName, TIsRoot, pool_t, std::true_type>>(
                TConfig::policies(*this), dependency, ctor_t{}
            );
        )()
        return successful::wrapper<create_t, wrapper_t>{dependency.template create<T>(provider_t{*this})};
    }
};
#undef BOOST_DI_CORE_INJECTOR_POLICY
#undef BOOST_DI_CORE_INJECTOR_POLICY_ELSE

}}}} // boost::di::v1::core

#endif

#ifndef BOOST_DI_CONCEPTS_PROVIDABLE_HPP
#define BOOST_DI_CONCEPTS_PROVIDABLE_HPP

namespace boost { namespace di { inline namespace v1 {

template<class>
struct provider {
    struct is_not_providable { };
};

namespace concepts {

template<class T>
typename provider<T>::is_not_providable providable_impl(...);

template<class T>
auto providable_impl(T&& t) -> aux::is_valid_expr<
    decltype(t.template get<_, _>(type_traits::direct{}, type_traits::heap{}))
  , decltype(t.template get<_, _>(type_traits::direct{}, type_traits::heap{}, int{}))
  , decltype(t.template get<_, _>(type_traits::uniform{}, type_traits::stack{}))
  , decltype(t.template get<_, _>(type_traits::uniform{}, type_traits::stack{}, int{}))
  , decltype(T::template is_creatable<type_traits::direct, type_traits::heap, _>::value)
  , decltype(T::template is_creatable<type_traits::uniform, type_traits::stack, _, int>::value)
>;

template<class T>
struct providable__ {
    using type = decltype(providable_impl<T>(std::declval<T>()));
};

template<class T>
using providable = typename providable__<T>::type;

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_CONCEPTS_CONFIGURABLE_HPP
#define BOOST_DI_CONCEPTS_CONFIGURABLE_HPP

namespace boost { namespace di { inline namespace v1 {

template<class>
struct config_type {
    struct is_not_configurable { };
};

namespace concepts {

std::false_type configurable_impl(...);

template<class T>
auto configurable_impl(T&& t) -> aux::is_valid_expr<
    decltype(T::provider(static_cast<const T&>(t)))
  , decltype(T::policies(static_cast<const T&>(t)))
>;

template<class T1, class T2>
struct get_configurable_error
    : aux::type_list<T1, T2>
{ };

template<class T>
struct get_configurable_error<std::true_type, T> {
    using type = T;
};

template<class T>
struct get_configurable_error<T, std::true_type> {
    using type = T;
};

template<>
struct get_configurable_error<std::true_type, std::true_type>
    : std::true_type
{ };

template<class T>
auto is_configurable(const std::true_type&) {
    return typename get_configurable_error<
        decltype(providable<decltype(T::provider(std::declval<T>()))>())
      , decltype(callable<decltype(T::policies(std::declval<T>()))>())
    >::type{};
}

template<class T>
auto is_configurable(const std::false_type&) {
    return typename config_type<T>::is_not_configurable{};
}

template<class T>
struct configurable__ {
    using type = decltype(is_configurable<T>(decltype(configurable_impl(std::declval<T>())){}));
};

template<class T>
using configurable = typename configurable__<T>::type;

}}}} // boost::di::v1::concepts

#endif

#ifndef BOOST_DI_INJECTOR_HPP
#define BOOST_DI_INJECTOR_HPP

namespace boost { namespace di { inline namespace v1 { namespace detail {

template<class>
void create(const std::true_type&) { }

template<class>
BOOST_DI_CONCEPTS_CREATABLE_ATTR
void
    create
(const std::false_type&) { }

} // namespace detail

template<class... T>
class injector : public
     BOOST_DI_REQUIRES_MSG_T(concepts::boundable<aux::type<T...>>
                           , core::injector<::BOOST_DI_CFG, core::pool<>, T...>) {
public:
    template<
        class TConfig
      , class TPolicies
      , class... TDeps
        #if defined(BOOST_DI_GCC)
          , BOOST_DI_REQUIRES_MSG(concepts::boundable<aux::type<T...>>)
        #endif
    > injector(const core::injector<TConfig, TPolicies, TDeps...>& injector) noexcept // non explicit
        : core::injector<::BOOST_DI_CFG, core::pool<>, T...>(injector) {
            using injector_t = core::injector<TConfig, TPolicies, TDeps...>;
            int _[]{0, (
                detail::create<T>(
                    std::integral_constant<bool,
                        injector_t::template is_creatable<T>::value ||
                        injector_t::template is_creatable<T*>::value
                    >{}
                )
            , 0)...}; (void)_;
    }
};

}}} // boost::di::v1

#endif

#ifndef BOOST_DI_MAKE_INJECTOR_HPP
#define BOOST_DI_MAKE_INJECTOR_HPP

namespace boost { namespace di { inline namespace v1 {

template<
     class TConfig = ::BOOST_DI_CFG
   , class... TDeps
   , BOOST_DI_REQUIRES_MSG(concepts::boundable<aux::type_list<TDeps...>>)
   , BOOST_DI_REQUIRES_MSG(concepts::configurable<TConfig>)
> inline auto make_injector(const TDeps&... args) noexcept {
    return core::injector<TConfig, decltype(((TConfig*)0)->policies(0)), TDeps...>{core::init{}, args...};
}

}}} // boost::di::v1

#endif

#ifndef BOOS_DI_POLICIES_CONSTRUCTIBLE_HPP
#define BOOS_DI_POLICIES_CONSTRUCTIBLE_HPP

namespace boost { namespace di { inline namespace v1 { namespace policies {

template<class T>
struct type {
template<class TPolicy>
struct not_allowed_by {
    operator T() const {
        using constraint_not_satisfied = not_allowed_by;
        return
            constraint_not_satisfied{}.error();
    }

    static inline T
    error(_ = "type disabled by constructible policy, added by BOOST_DI_CFG or make_injector<CONFIG>!");
};};

struct type_op {};

template<class T, class = void>
struct apply_impl {
    template<class>
    struct apply : T { };
};

template<template<class...> class T, class... Ts>
struct apply_impl<T<Ts...>, std::enable_if_t<!std::is_base_of<type_op, T<Ts...>>::value>> {
    template<class TOp, class>
    struct apply_placeholder_impl {
        using type = TOp;
    };

    template<class TOp>
    struct apply_placeholder_impl<_, TOp> {
        using type = TOp;
    };

    template<template<class...> class TExpr, class TOp, class... TArgs>
    struct apply_placeholder {
        using type = TExpr<typename apply_placeholder_impl<TArgs, TOp>::type...>;
    };

    template<class TArg>
    struct apply : apply_placeholder<T, typename TArg::type, Ts...>::type
    { };
};

template<class T>
struct apply_impl<T, std::enable_if_t<std::is_base_of<type_op, T>::value>> {
    template<class TArg>
    struct apply : T::template apply<TArg>::type { };
};

template<class T>
struct not_ : type_op {
    template<class TArg>
    struct apply : std::integral_constant<bool,
        !apply_impl<T>::template apply<TArg>::value
    > { };
};

template<class... Ts>
struct and_ : type_op {
    template<class TArg>
    struct apply : std::is_same<
        aux::bool_list<apply_impl<Ts>::template apply<TArg>::value...>
      , aux::bool_list<aux::always<Ts>::value...>
    > { };
};

template<class... Ts>
struct or_ : type_op {
    template<class TArg>
    struct apply : std::integral_constant<bool,
        !std::is_same<
            aux::bool_list<apply_impl<Ts>::template apply<TArg>::value...>
          , aux::bool_list<aux::never<Ts>::value...>
        >::value
    > { };
};

template<class T>
struct always : type_op {
    template<class TArg>
    struct apply : apply_impl<T>::template apply<TArg>::type { };
};

template<class T>
struct is_bound : type_op {
    struct not_resolved { };

    template<class TArg>
    struct apply : std::integral_constant<bool,
        !std::is_same<
            typename TArg::template resolve<
                std::conditional_t<
                    std::is_same<T, _>::value
                  , typename TArg::type
                  , T
                >
              , typename TArg::name
              , not_resolved
            >
          , not_resolved
         >::value>
    { };
};

struct is_root : type_op {
    template<class TArg>
    struct apply : TArg::is_root { };
};

namespace operators {

template<class X, class Y>
inline auto operator||(const X&, const Y&) {
    return or_<X, Y>{};
}

template<class X, class Y>
inline auto operator&&(const X&, const Y&) {
    return and_<X, Y>{};
}

template<class T>
inline auto operator!(const T&) {
    return not_<T>{};
}

} // operators

template<class T>
struct constructible_impl {
    template<class TArg, BOOST_DI_REQUIRES(T::template apply<TArg>::value)>
    std::true_type operator()(const TArg&) const {
        return {};
    }

    template<class TArg, BOOST_DI_REQUIRES(!T::template apply<TArg>::value)>
    std::false_type operator()(const TArg&) const {
        dump_error<typename TArg::type>(typename TArg::ignore{});
        return {};
    }

    template<class T_>
    void dump_error(const std::true_type&) const {
        void(static_cast<T_>(typename type<T_>::template not_allowed_by<T>{}));
    }

    template<class>
    void dump_error(const std::false_type&) const { }
};

template<class T = aux::never<_>, BOOST_DI_REQUIRES(std::is_base_of<type_op, T>::value)>
inline auto constructible(const T& = {}) {
    return constructible_impl<T>{};
}

template<class T = aux::never<_>, BOOST_DI_REQUIRES(!std::is_base_of<type_op, T>::value)>
inline auto constructible(const T& = {}) {
    return constructible_impl<or_<T>>{};
}

}}}} // boost::di::v1::policies

#endif

#ifndef BOOST_DI_PROVIDERS_HEAP_HPP
#define BOOST_DI_PROVIDERS_HEAP_HPP

namespace boost { namespace di { inline namespace v1 { namespace providers {

class heap {
public:
    template<class TInitialization, class TMemory, class T, class... TArgs>
    struct is_creatable {
        static constexpr auto value =
            concepts::creatable<TInitialization, T, TArgs...>::value;
    };

    template<class, class T, class TMemory, class... TArgs>
    auto get(const type_traits::direct&
           , const TMemory&
           , TArgs&&... args) const {
        return new T(static_cast<TArgs&&>(args)...);
    }

    template<class, class T, class TMemory, class... TArgs>
    auto get(const type_traits::uniform&
           , const TMemory&
           , TArgs&&... args) const {
        return new T{static_cast<TArgs&&>(args)...};
    }
};

}}}} // boost::di::v1::providers

#endif

#endif

#endif

