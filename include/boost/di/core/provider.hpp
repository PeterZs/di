//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
// // Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_CORE_PROVIDER_HPP
#define BOOST_DI_CORE_PROVIDER_HPP

#include "boost/di/aux_/utility.hpp"
#include "boost/di/aux_/type_traits.hpp"
#include "boost/di/core/any_type.hpp"
#include "boost/di/type_traits/memory_traits.hpp"
#include "boost/di/type_traits/ctor_traits.hpp"

namespace boost { namespace di { namespace core {

template<class...>
struct provider;

template<
    class TExpected
  , class TGiven
  , class TParent
  , class TInjector
  , class TInitialization
  , class... TArgs
> struct provider<
    TExpected
  , TGiven
  , TParent
  , aux::pair<TInitialization, aux::type_list<TArgs...>>
  , TInjector
> {
    template<class TMemory = type_traits::heap>
    auto get(const TMemory& memory = {}) const {
        auto&& config = ((TInjector&)injector_).config();
        return config.provider().template get<TExpected, TGiven>(
            TInitialization{}
          , memory
          , get_impl(aux::type<TArgs>{})...
        );
    }

    template<class T>
    auto get_impl(const aux::type<T>&) const {
        return injector_.template create_t<T>();
    }

    template<class... Ts>
    auto get_impl(const aux::type<any_type<Ts...>>&) const {
        return any_type<TParent, TInjector>{injector_};
    }

    template<class T, class TName>
    auto get_impl(const aux::type<type_traits::named<TName, T>>&) const {
        return injector_.template create_t<T, TName>();
    }

    const TInjector& injector_;
};

}}} // boost::di::core

#endif

