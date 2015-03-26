//
// Copyright (c) 2014 Krzysztof Jusiak (krzysztof at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_DI_SCOPES_EXPOSED_HPP
#define BOOST_DI_SCOPES_EXPOSED_HPP

#include <memory>
#include <type_traits>
#include "boost/di/scopes/deduce.hpp"

namespace boost { namespace di { namespace scopes {

template<class TScope = scopes::deduce>
class exposed {
public:
    static constexpr auto priority = false;

    template<class TExpected, class TGiven>
    class scope {
        template<class T, class = void>
        struct iprovider {
            virtual ~iprovider() noexcept = default;
            virtual T* get(const type_traits::heap& = {}) const noexcept = 0;
            virtual T  get(const type_traits::stack&) const noexcept = 0;
        };

        template<class T>
        struct iprovider<T, std::enable_if_t<!std::is_copy_constructible<T>{}>> {
            virtual ~iprovider() noexcept = default;
            virtual T* get(const type_traits::heap& = {}) const noexcept = 0;
        };

        template<class TInjector>
        class provider_impl : public iprovider<TExpected> {
        public:
            explicit provider_impl(const TInjector& injector)
                : injector_(injector)
            { }

            TExpected* get(const type_traits::heap&) const noexcept { // non override
                return injector_.template create_impl<TExpected*>();
            }

            std::conditional_t<std::is_copy_constructible<TExpected>{}, TExpected, void>
            get(const type_traits::stack&) const noexcept { // non override
                return injector_.template create_impl<TExpected>();
            }

        private:
            TInjector injector_;
        };

    public:
        template<class TInjector>
        explicit scope(const TInjector& injector) noexcept
            : provider_{std::make_shared<provider_impl<TInjector>>(injector)}
        { }

        template<class T, class TProvider>
        T create_(const TProvider&);

        template<class T, class TProvider>
        auto create(const TProvider&) {
            return scope_.template create<T>(*provider_);
        }

    private:
        std::shared_ptr<iprovider<TExpected>> provider_;
        typename TScope::template scope<TExpected, TGiven> scope_;
    };
};

}}} // boost::di::scopes

#endif

