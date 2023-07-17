/// @file ungl2.hpp
/// @author Yuxin Cai, University of Colorado Denver, USA
/// @note Adapted from @see
/// https://github.com/Reference-LAPACK/lapack/blob/master/SRC/zungl2.f
//
// Copyright (c) 2021-2023, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

#ifndef TLAPACK_UNGL2_HH
#define TLAPACK_UNGL2_HH

#include "tlapack/base/utils.hpp"
#include "tlapack/lapack/larf.hpp"

namespace tlapack {

/** Worspace query of ungl2()
 *
 * @param[in] Q k-by-n matrix.
 *
 * @param[in] tauw Complex vector of length min(m,n).
 *      tauw(j) must contain the scalar factor of the elementary
 *      reflector H(j), as returned by gelq2.
 *
 * @param[in] opts Options.
 *
 * @return WorkInfo The amount workspace required.
 *
 * @ingroup workspace_query
 */
template <TLAPACK_SMATRIX matrix_t, TLAPACK_VECTOR vector_t>
inline constexpr WorkInfo ungl2_worksize(const matrix_t& Q,
                                         const vector_t& tauw,
                                         const WorkspaceOpts& opts = {})
{
    using idx_t = size_type<matrix_t>;
    using range = pair<idx_t, idx_t>;

    // constants
    const idx_t k = nrows(Q);

    if (k > 1) {
        auto C = rows(Q, range{1, k});
        return larf_worksize(RIGHT_SIDE, FORWARD, ROWWISE_STORAGE, row(Q, 0),
                             tauw[0], C, opts);
    }
    return WorkInfo{};
}

/**
 * Generates all or part of the unitary matrix Q from an LQ factorization
 * determined by gelq2 (unblocked algorithm).
 *
 * The matrix Q is defined as the first k rows of a product of k elementary
 * reflectors of order n
 * \[
 *          Q = H(k)**H ... H(2)**H H(1)**H
 * \]
 * as returned by gelq2 and k <= n.
 *
 * @return  0 if success
 *
 * @param[in,out] Q k-by-n matrix.
 *      On entry, the i-th row must contain the vector which defines
 *      the elementary reflector H(j), for j = 1,2,...,k, as returned
 *      by gelq2 in the first k rows of its array argument A.
 *      On exit, the k by n matrix Q.
 *
 * @param[in] tauw Complex vector of length min(m,n).
 *      tauw(j) must contain the scalar factor of the elementary
 *      reflector H(j), as returned by gelq2.
 *
 * @param[in] opts Options.
 *      @c opts.work is used if whenever it has sufficient size.
 *      The sufficient size can be obtained through a workspace query.
 *
 * @ingroup computational
 */
template <TLAPACK_SMATRIX matrix_t, TLAPACK_VECTOR vector_t>
int ungl2(matrix_t& Q, const vector_t& tauw, const WorkspaceOpts& opts = {})
{
    using idx_t = size_type<matrix_t>;
    using T = type_t<matrix_t>;
    using range = pair<idx_t, idx_t>;
    using real_t = real_type<T>;

    // constants
    const idx_t k = nrows(Q);
    const idx_t n = ncols(Q);
    const idx_t m =
        size(tauw);  // maximum number of Householder reflectors to use
    const idx_t t =
        min(k, m);  // desired number of Householder reflectors to use

    // check arguments
    tlapack_check_false((idx_t)size(tauw) < min(m, n));

    // Allocates workspace
    VectorOfBytes localworkdata;
    Workspace work = [&]() {
        WorkInfo workinfo = ungl2_worksize(Q, tauw, opts);
        return alloc_workspace(localworkdata, workinfo, opts.work);
    }();

    // Options to forward
    const auto& larfOpts = WorkspaceOpts{work};

    // Initialise columns t:k-1 to rows of the unit matrix
    if (k > m) {
        for (idx_t j = 0; j < n; ++j) {
            for (idx_t i = t; i < k; ++i)
                Q(i, j) = real_t(0);
            if (j < k && j > t - 1) Q(j, j) = real_t(1);
        }
    }

    for (idx_t j = t - 1; j != idx_t(-1); --j) {
        // Apply H(j)**H to Q(j:k,j:n) from the right
        if (j + 1 < n) {
            auto w = slice(Q, j, range(j, n));

            // Apply to the Q11 below the w
            if ((k > m && j + 1 == t) || (j + 1 < t)) {
                // When k > m, we need to start from the (t-1)th row of w and
                // apply to Q(j+1:k,j:n) This procedure is only used once when
                // both conditions are satisfied

                auto Q11 = slice(Q, range(j + 1, k), range(j, n));
                larf(Side::Right, FORWARD, ROWWISE_STORAGE, w, conj(tauw[j]),
                     Q11, larfOpts);
            }

            scal(-conj(tauw[j]), w);
        }

        Q(j, j) = real_t(1.) - conj(tauw[j]);

        // Set Q(j,0:j-1) to zero
        for (idx_t l = 0; l < j; l++)
            Q(j, l) = real_t(0);
    }

    return 0;
}
}  // namespace tlapack
#endif  // TLAPACK_UNGL2_HH
