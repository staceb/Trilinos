#pragma once
#ifndef __ICHOL_LEFT_BLOCKED_HPP__
#define __ICHOL_LEFT_BLOCKED_HPP__

/// \file ichol_left_blocked.hpp
/// \brief Blocked incomplete Chloesky factorization.
/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "partition.hpp"

#include "scale.hpp"
#include "dot.hpp"

#include "gemv.hpp"
#include "gemm.hpp"

#include "trsv.hpp"
#include "trsm.hpp"

#include "ichol_left_unblocked.hpp"

namespace Example { 

  using namespace std;
  
  // use Lower Triangular part only
  template<typename CrsMatViewType>
  KOKKOS_INLINE_FUNCTION 
  int 
  ichol_left_blocked_lower(const CrsMatViewType A,
                           const typename CrsMatViewType::ordinal_type mb) {
    typedef typename CrsMatViewType::value_type   value_type;
    typedef typename CrsMatViewType::ordinal_type ordinal_type;

    // if succeed, return 0 
    int r_val = 0;

    CrsMatViewType ATL, ATR,      A00, A01, A02,
      /**/         ABL, ABR,      A10, A11, A12,
      /**/                        A20, A21, A22;
    
    Part_2x2(A,  ATL, ATR,
             /**/ABL, ABR, 
             0, 0, Partition::TopLeft);
    
    while (ATL.NumRows() < A.NumRows()) {
      Part_2x2_to_3x3(ATL, ATR, /**/  A00, A01, A02,
                      /*******/ /**/  A10, A11, A12,
                      ABL, ABR, /**/  A20, A21, A22,  
                      mb, mb, Partition::BottomRight);
      // -----------------------------------------------------
      CrsMatViewType AB0, AB1;
      
      Merge_2x1(A11,
                A21, AB1);
      
      Merge_2x1(A10,
                A20, AB0);

      // sparse gemm
      gemm_nt_t(-1.0, AB0, A10, /**/ 1.0, AB1);

      // cholesky on diagonal block
      r_val = ichol_left_unblocked_lower(A11);
      if (r_val)
        break;

      // trsm
      r_val = trsm_r_l_t(Diag::NonUnit, 1.0, A11, A21);
      if (r_val)
        break;

      // -----------------------------------------------------
      Merge_3x3_to_2x2(A00, A01, A02, /**/ ATL, ATR,
                       A10, A11, A12, /**/ /******/
                       A20, A21, A22, /**/ ABL, ABR,
                       Partition::TopLeft);
    }

    return r_val;
  }

}

#endif