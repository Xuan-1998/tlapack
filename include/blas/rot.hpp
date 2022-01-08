// Copyright (c) 2017-2021, University of Tennessee. All rights reserved.
// Copyright (c) 2021, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

#ifndef BLAS_ROT_HH
#define BLAS_ROT_HH

#include "blas/utils.hpp"

namespace blas {

/**
 * Apply plane rotation:
 * \[
 *       \begin{bmatrix} x^T   \\ y^T    \end{bmatrix}
 *     = \begin{bmatrix} c & s \\ -s & c \end{bmatrix}
 *       \begin{bmatrix} x^T   \\ y^T    \end{bmatrix}.
 * \]
 *
 * @see rotg to generate the rotation.
 *
 * Generic implementation for arbitrary data types.
 *
 * @param[in] n
 *     Number of elements in x and y. n >= 0.
 *
 * @param[in, out] x
 *     The n-element vector x, in an array of length (n-1)*abs(incx) + 1.
 *
 * @param[in] incx
 *     Stride between elements of x. incx must not be zero.
 *     If incx < 0, uses elements of x in reverse order: x(n-1), ..., x(0).
 *
 * @param[in, out] y
 *     The n-element vector y, in an array of length (n-1)*abs(incy) + 1.
 *
 * @param[in] incy
 *     Stride between elements of y. incy must not be zero.
 *     If incy < 0, uses elements of y in reverse order: y(n-1), ..., y(0).
 *
 * @param[in] c
 *     Cosine of rotation; real.
 *
 * @param[in] s
 *     Sine of rotation; complex.
 *
 * @ingroup rot
 */
template<
    class vectorX_t, class vectorY_t,
    class c_type, class s_type,
    enable_if_t<(
    /* Requires: */
        ! is_complex<c_type>::value
    ), int > = 0
>
void rot(
    vectorX_t& x, vectorY_t& y,
    const c_type& c, const s_type& s )
{
    using idx_t = size_type< vectorY_t >;

    // constants
    const idx_t n = size(x);

    // check arguments
    blas_error_if( size(y) != n );

    for (idx_t i = 0; i < n; ++i) {
        auto stmp = c*x(i) + s*y(i);
        y(i) = c*y(i) - conj(s)*x(i);
        x(i) = stmp;
    }
}

template< typename TX, typename TY >
void rot(
    blas::idx_t n,
    TX *x, blas::int_t incx,
    TY *y, blas::int_t incy,
    blas::real_type<TX, TY>   c,
    blas::scalar_type<TX, TY> s )
{
    typedef scalar_type<TX, TY> scalar_t;
    using internal::vector;

    // check arguments
    blas_error_if( incx == 0 );
    blas_error_if( incy == 0 );

    // quick return
    if ( n == 0 || (c == 1 && s == scalar_t(0)) )
        return;

    // Views
    auto _x = vector<TX>(
        &x[(incx > 0 ? 0 : (-n + 1)*incx)],
        n, incx );
    auto _y = vector<TY>(
        &y[(incy > 0 ? 0 : (-n + 1)*incy)],
        n, incy );

    rot( _x, _y, c, s );
}

}  // namespace blas

#endif        //  #ifndef BLAS_ROT_HH
