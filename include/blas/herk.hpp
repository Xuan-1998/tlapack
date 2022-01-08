// Copyright (c) 2017-2021, University of Tennessee. All rights reserved.
// Copyright (c) 2021, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

#ifndef BLAS_HERK_HH
#define BLAS_HERK_HH

#include "blas/utils.hpp"
#include "blas/syrk.hpp"

namespace blas {

/**
 * Hermitian rank-k update:
 * \[
 *     C = \alpha A A^H + \beta C,
 * \]
 * or
 * \[
 *     C = \alpha A^H A + \beta C,
 * \]
 * where alpha and beta are real scalars, C is an n-by-n Hermitian matrix,
 * and A is an n-by-k or k-by-n matrix.
 *
 * Generic implementation for arbitrary data types.
 *
 * @param[in] layout
 *     Matrix storage, Layout::ColMajor or Layout::RowMajor.
 *
 * @param[in] uplo
 *     What part of the matrix C is referenced,
 *     the opposite triangle being assumed from symmetry:
 *     - Uplo::Lower: only the lower triangular part of C is referenced.
 *     - Uplo::Upper: only the upper triangular part of C is referenced.
 *
 * @param[in] trans
 *     The operation to be performed:
 *     - Op::NoTrans:   $C = \alpha A A^H + \beta C$.
 *     - Op::ConjTrans: $C = \alpha A^H A + \beta C$.
 *     - In the real    case, Op::Trans is interpreted as Op::ConjTrans.
 *       In the complex case, Op::Trans is illegal (see @ref syrk instead).
 *
 * @param[in] n
 *     Number of rows and columns of the matrix C. n >= 0.
 *
 * @param[in] k
 *     - If trans = NoTrans: number of columns of the matrix A. k >= 0.
 *     - Otherwise:          number of rows    of the matrix A. k >= 0.
 *
 * @param[in] alpha
 *     Scalar alpha. If alpha is zero, A is not accessed.
 *
 * @param[in] A
 *     - If trans = NoTrans:
 *     the n-by-k matrix A, stored in an lda-by-k array [RowMajor: n-by-lda].
 *     - Otherwise:
 *     the k-by-n matrix A, stored in an lda-by-n array [RowMajor: k-by-lda].
 *
 * @param[in] lda
 *     Leading dimension of A.
 *     If trans = NoTrans: lda >= max(1, n) [RowMajor: lda >= max(1, k)],
 *     If Otherwise:       lda >= max(1, k) [RowMajor: lda >= max(1, n)].
 *
 * @param[in] beta
 *     Scalar beta. If beta is zero, C need not be set on input.
 *
 * @param[in] C
 *     The n-by-n Hermitian matrix C,
 *     stored in an lda-by-n array [RowMajor: n-by-lda].
 *
 * @param[in] ldc
 *     Leading dimension of C. ldc >= max(1, n).
 *
 * @ingroup herk
 */
template<
    class matrixA_t, class matrixC_t, 
    class alpha_t, class beta_t,
    enable_if_t<(
    /* Requires: */
        !is_complex<alpha_t>::value &&
        !is_complex<beta_t> ::value
    ), int > = 0
>
void herk(
    blas::Uplo uplo,
    blas::Op trans,
    const alpha_t& alpha, const matrixA_t& A,
    const beta_t& beta, matrixC_t& C )
{
    // data traits
    using TA    = type_t< matrixA_t >;
    using idx_t = size_type< matrixC_t >;

    // constants
    const idx_t n = (trans == Op::NoTrans) ? nrows(A) : ncols(A);
    const idx_t k = (trans == Op::NoTrans) ? ncols(A) : nrows(A);

    // check arguments
    blas_error_if( uplo != Uplo::Lower &&
                   uplo != Uplo::Upper &&
                   uplo != Uplo::General );
    blas_error_if( trans != Op::NoTrans &&
                   trans != Op::ConjTrans );
    blas_error_if( nrows(C) == ncols(C) &&
                   nrows(C) == n );

    if (trans == Op::NoTrans) {
        if (uplo != Uplo::Lower) {
        // uplo == Uplo::Upper or uplo == Uplo::General
            for(idx_t j = 0; j < n; ++j) {

                for(idx_t i = 0; i < j; ++i)
                    C(i,j) *= beta;
                C(j,j) = beta * real( C(j,j) );

                for(idx_t l = 0; l < k; ++l) {

                    auto alphaConjAjl = alpha*conj( A(j,l) );

                    for(idx_t i = 0; i < j; ++i)
                        C(i,j) += A(i,l)*alphaConjAjl;
                    C(j,j) += real( A(j,l) * alphaConjAjl );
                }
            }
        }
        else { // uplo == Uplo::Lower
            for(idx_t j = 0; j < n; ++j) {

                C(j,j) = beta * real( C(j,j) );
                for(idx_t i = j+1; i < n; ++i)
                    C(i,j) *= beta;

                for(idx_t l = 0; l < k; ++l) {

                    auto alphaConjAjl = alpha*conj( A(j,l) );

                    C(j,j) += real( A(j,l) * alphaConjAjl );
                    for(idx_t i = j+1; i < n; ++i) {
                        C(i,j) += A(i,l) * alphaConjAjl;
                    }
                }
            }
        }
    }
    else { // trans == Op::ConjTrans
        if (uplo != Uplo::Lower) {
        // uplo == Uplo::Upper or uplo == Uplo::General
            for(idx_t j = 0; j < n; ++j) {
                for(idx_t i = 0; i < j; ++i) {
                    TA sum = 0;
                    for(idx_t l = 0; l < k; ++l)
                        sum += conj( A(l,i) ) * A(l,j);
                    C(i,j) = alpha*sum + beta*C(i,j);
                }
                real_type<TA> sum = 0;
                for(idx_t l = 0; l < k; ++l)
                    sum += real(A(l,j)) * real(A(l,j))
                         + imag(A(l,j)) * imag(A(l,j));
                C(j,j) = alpha*sum + beta*real( C(j,j) );
            }
        }
        else {
            // uplo == Uplo::Lower
            for(idx_t j = 0; j < n; ++j) {
                for(idx_t i = j+1; i < n; ++i) {
                    TA sum = 0;
                    for(idx_t l = 0; l < k; ++l)
                        sum += conj( A(l,i) ) * A(l,j);
                    C(i,j) = alpha*sum + beta*C(i,j);
                }
                real_type<TA> sum = 0;
                for(idx_t l = 0; l < k; ++l)
                    sum += real(A(l,j)) * real(A(l,j))
                         + imag(A(l,j)) * imag(A(l,j));
                C(j,j) = alpha*sum + beta*real( C(j,j) );
            }
        }
    }

    if (uplo == Uplo::General) {
        for(idx_t j = 0; j < n; ++j) {
            for(idx_t i = j+1; i < n; ++i)
                C(i,j) = conj( C(j,i) );
        }
    }
}

template< typename TA, typename TC >
void herk(
    blas::Layout layout,
    blas::Uplo uplo,
    blas::Op trans,
    blas::idx_t n, blas::idx_t k,
    real_type<TA, TC> alpha,  // note: real
    TA const *A_, blas::idx_t lda,
    real_type<TA, TC> beta,  // note: real
    TC       *C_, blas::idx_t ldc )
{
    typedef blas::real_type<TA, TC> real_t;
    using blas::internal::colmajor_matrix;

    // constants
    const TC szero( 0 );
    const real_t zero( 0 );
    const real_t one( 1 );

    // check arguments
    blas_error_if( layout != Layout::ColMajor &&
                   layout != Layout::RowMajor );
    blas_error_if( uplo != Uplo::Lower &&
                   uplo != Uplo::Upper &&
                   uplo != Uplo::General );
    blas_error_if( trans != Op::NoTrans &&
                   trans != Op::Trans &&
                   trans != Op::ConjTrans );
    blas_error_if( is_complex<TA>::value && trans == Op::Trans );
    blas_error_if( n < 0 );
    blas_error_if( k < 0 );
    blas_error_if( lda < (
        (layout == Layout::RowMajor)
            ? ((trans == Op::NoTrans) ? k : n)
            : ((trans == Op::NoTrans) ? n : k)
        )
    );
    blas_error_if( ldc < n );

    // quick return
    if (n == 0)
        return;

    // This algorithm only works with Op::NoTrans or Op::ConjTrans
    if(trans == Op::ConjTrans) trans = Op::Trans;

    // adapt if row major
    if (layout == Layout::RowMajor) {
        if (uplo == Uplo::Lower)
            uplo = Uplo::Upper;
        else if (uplo == Uplo::Upper)
            uplo = Uplo::Lower;
        trans = (trans == Op::NoTrans)
            ? Op::ConjTrans
            : Op::NoTrans;
        alpha = conj(alpha);
    }
        
    // Matrix views
    const auto A = (trans == Op::NoTrans)
                 ? colmajor_matrix<TA>( (TA*)A_, n, k, lda )
                 : colmajor_matrix<TA>( (TA*)A_, k, n, lda );
    auto C = colmajor_matrix<TC>( C_, n, n, ldc );

    // alpha == zero
    if (alpha == zero) {
        if (beta == zero) {
            if (uplo != Uplo::Upper) {
                for(idx_t j = 0; j < n; ++j) {
                    for(idx_t i = 0; i <= j; ++i)
                        C(i,j) = szero;
                }
            }
            else if (uplo != Uplo::Lower) {
                for(idx_t j = 0; j < n; ++j) {
                    for(idx_t i = j; i < n; ++i)
                        C(i,j) = szero;
                }
            }
            else {
                for(idx_t j = 0; j < n; ++j) {
                    for(idx_t i = 0; i < n; ++i)
                        C(i,j) = szero;
                }
            }
        } else if (beta != one) {
            if (uplo != Uplo::Upper) {
                for(idx_t j = 0; j < n; ++j) {
                    for(idx_t i = 0; i < j; ++i)
                        C(i,j) *= beta;
                    C(j,j) = beta * real( C(j,j) );
                }
            }
            else if (uplo != Uplo::Lower) {
                for(idx_t j = 0; j < n; ++j) {
                    C(j,j) = beta * real( C(j,j) );
                    for(idx_t i = j+1; i < n; ++i)
                        C(i,j) *= beta;
                }
            }
            else {
                for(idx_t j = 0; j < n; ++j) {
                    for(idx_t i = 0; i < j; ++i)
                        C(i,j) *= beta;
                    C(j,j) = beta * real( C(j,j) );
                    for(idx_t i = j+1; i < n; ++i)
                        C(i,j) *= beta;
                }
            }
        }
        return;
    }

    herk( uplo, trans, alpha, A, beta, C );
}

}  // namespace blas

#endif        //  #ifndef BLAS_HERK_HH
