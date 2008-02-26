/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2008 Chris Kenyon
 Copyright (C) 2008 Roland Lichters
 Copyright (C) 2008 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/termstructures/credit/defaultdensitystructure.hpp>
#include <ql/math/integrals/gaussianquadratures.hpp>
#include <boost/bind.hpp>

using namespace boost;

namespace QuantLib {

    namespace {

        template <class F>
        struct remapper {
            F f;
            Time T;
            remapper(const F& f, Time T) : f(f), T(T) {}
            // This remaps [-1,1] to [0,T]. No differential included.
            Real operator()(Real x) const {
                return f((x+1.0)*T/2.0);
            }
        };

        template <class F>
        remapper<F> remap(const F& f, Time T) {
            return remapper<F>(f,T);
        }

    }

    DefaultDensityStructure::DefaultDensityStructure(const DayCounter& dc)
    : DefaultProbabilityTermStructure(dc) {}

    DefaultDensityStructure::DefaultDensityStructure(const Date& referenceDate,
                                                     const Calendar& cal,
                                                     const DayCounter& dc)
    : DefaultProbabilityTermStructure(referenceDate, cal, dc) {}

    DefaultDensityStructure::DefaultDensityStructure(Natural settlementDays,
                                                     const Calendar& cal,
                                                     const DayCounter& dc)
    : DefaultProbabilityTermStructure(settlementDays, cal, dc) {}


    Probability DefaultDensityStructure::survivalProbabilityImpl(Time t) const {
        static GaussChebyshevIntegration integral(48);
        // this stores the address of the method to integrate (so that
        // we don't have to insert its full expression inside the
        // integral below--it's long enough already)
        Real (DefaultProbabilityTermStructure::*f)(Time,bool) const =
            &DefaultProbabilityTermStructure::defaultDensity;
        // the Gauss-Chebyshev quadratures integrate over [-1,1],
        // hence the remapping (and the Jacobian term t/2)
        Probability P = 1.0 - integral(remap(bind(f,this,_1,true), t)) * t/2.0;
        QL_ENSURE(P >= 0.0, "negative survival probability");
        return P;
    }

    Real DefaultDensityStructure::hazardRateImpl(Time t) const {
        Probability S = survivalProbabilityImpl(t);
        return S == 0.0 ? 0.0 : defaultDensityImpl(t)/S;
    }

}
