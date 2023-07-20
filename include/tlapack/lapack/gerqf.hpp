/// @file gerqf.hpp
/// @author Thijs Steel, KU Leuven, Belgium
/// @note Adapted from @see
/// https://github.com/Reference-LAPACK/lapack/blob/master/SRC/zgerqf.f
//
// Copyright (c) 2021-2023, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

#ifndef TLAPACK_GERQF_HH
#define TLAPACK_GERQF_HH

#include "tlapack/base/utils.hpp"
#include "tlapack/lapack/gerq2.hpp"
#include "tlapack/lapack/larfb.hpp"
#include "tlapack/lapack/larft.hpp"

namespace tlapack {
/**
 * Options struct for gerqf
 */
template <TLAPACK_INDEX idx_t = size_t>
struct GerqfOpts {
    idx_t nb = 32;  ///< Block size
};

/** Worspace query of gerqf()
 *
 * @param[in] A m-by-n matrix.
 *
 * @param tau min(n,m) vector.
 *
 * @param[in] opts Options.
 *
 * @return WorkInfo The amount workspace required.
 *
 * @ingroup workspace_query
 */
template <class T, TLAPACK_SMATRIX A_t, TLAPACK_SVECTOR tau_t>
inline constexpr WorkInfo gerqf_worksize(
    const A_t& A, const tau_t& tau, const GerqfOpts<size_type<A_t>>& opts = {})
{
    using idx_t = size_type<A_t>;
    using range = pair<idx_t, idx_t>;
    using matrixT_t = matrix_type<A_t, tau_t>;

    // constants
    const idx_t m = nrows(A);
    const idx_t n = ncols(A);
    const idx_t k = min(m, n);
    const idx_t nb = min(opts.nb, k);

    auto A11 = rows(A, range(0, nb));
    auto tauw1 = slice(tau, range(0, nb));
    WorkInfo workinfo = gerq2_worksize<T>(A11, tauw1).transpose();

    if (m > nb) {
        auto TT1 = slice(A, range(0, nb), range(0, nb));
        auto A12 = slice(A, range(nb, m), range(0, n));
        workinfo.minMax(larfb_worksize<T>(Side::Right, Op::NoTrans,
                                          Direction::Backward, StoreV::Rowwise,
                                          A11, TT1, A12));
        if constexpr (is_same_v<T, type_t<matrixT_t>>)
            workinfo += WorkInfo(nb, nb);
    }

    return workinfo;
}

/** Computes an RQ factorization of an m-by-n matrix A using
 *  a blocked algorithm.
 *
 * The matrix Q is represented as a product of elementary reflectors
 * \[
 *          Q = H_1' H_2' ... H_k',
 * \]
 * where k = min(m,n). Each H_i has the form
 * \[
 *          H_i = I - tau * v * v',
 * \]
 * where tau is a scalar, and v is a vector with
 * \[
 *          v[n-k+i+1:n] = 0; v[n-k+i-1] = 1,
 * \]
 * with v[1] through v[n-k+i-1] stored on exit below the diagonal
 * in the ith column of A, and tau in tau[i].
 *
 * @return  0 if success
 *
 * @param[in,out] A m-by-n matrix.
 *      On entry, the m by n matrix A.
 *      On exit, if m <= n, the upper triangle of the subarray
 *      A(0:m,n-m:n) contains the m by m upper triangular matrix R;
 *      if m >= n, the elements on and above the (m-n)-th subdiagonal
 *      contain the m by n upper trapezoidal matrix R; the remaining
 *      elements, with the array TAU, represent the unitary matrix
 *      Q as a product of elementary reflectors.
 *
 * @param[out] tau Real vector of length min(m,n).
 *      The scalar factors of the elementary reflectors.
 *
 * @param[in] opts Options.
 *      - @c opts.work is used if whenever it has sufficient size.
 *        The sufficient size can be obtained through a workspace query.
 *
 * @ingroup computational
 */
template <TLAPACK_SMATRIX A_t, TLAPACK_SVECTOR tau_t>
int gerqf(A_t& A, tau_t& tau, const GerqfOpts<size_type<A_t>>& opts = {})
{
    Create<A_t> new_matrix;
    using T = type_t<A_t>;

    using idx_t = size_type<A_t>;
    using range = pair<idx_t, idx_t>;

    // constants
    const idx_t m = nrows(A);
    const idx_t n = ncols(A);
    const idx_t k = min(m, n);
    const idx_t nb = min(opts.nb, k);

    // check arguments
    tlapack_check((idx_t)size(tau) >= k);

    // Allocate or get workspace
    WorkInfo workinfo = gerqf_worksize<T>(A, tau, opts);
    std::vector<T> work_;
    auto work = new_matrix(work_, workinfo.m, workinfo.n);

    auto workt = transpose_view(work);
    auto TT = (m > nb) ? slice(work, range{workinfo.m - nb, workinfo.m},
                               range{workinfo.n - nb, workinfo.n})
                       : slice(work, range{0, 0}, range{0, 0});

    // Main computational loop
    for (idx_t j2 = 0; j2 < k; j2 += nb) {
        const idx_t ib = min(nb, k - j2);
        const idx_t j = m - j2 - ib;

        // Compute the RQ factorization of the current block A(j:j+ib,0:n-j2)
        auto A11 = slice(A, range(j, j + ib), range(0, n - j2));
        auto tauw1 = slice(tau, range(k - j2 - ib, k - j2));

        gerq2(A11, tauw1, workt);

        if (j > 0) {
            // Form the triangular factor of the block reflector
            auto TT1 = slice(TT, range(0, ib), range(0, ib));
            larft(Direction::Backward, StoreV::Rowwise, A11, tauw1, TT1);

            // Apply H to A(0:j,0:n-j2) from the right
            auto A12 = slice(A, range(0, j), range(0, n - j2));
            larfb(Side::Right, Op::NoTrans, Direction::Backward,
                  StoreV::Rowwise, A11, TT1, A12, work);
        }
    }

    return 0;
}
}  // namespace tlapack
#endif  // TLAPACK_GERQF_HH
