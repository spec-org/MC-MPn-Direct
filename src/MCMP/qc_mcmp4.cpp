//
// Created by aedoran on 6/13/18.
//
#include "../qc_monte.h"
#include "qc_mcmp4.h"
#include "cblas.h"

template <int CVMP4>
void MCMP4<CVMP4>::contract(std::vector<double>& result,
    const std::vector<double>& A, CBLAS_TRANSPOSE A_trans, 
    const std::vector<double>& B, const std::vector<double>& v) {
  Ddgmm(DDGMM_SIDE_LEFT,
      mpn, mpn,
      B.data(), mpn,
      v.data(), 1,
      Av.data(), mpn);
  cblas_dgemm(CblasColMajor,
      A_trans, CblasNoTrans,
      mpn, mpn, mpn,
      1.0,
      A.data(), mpn,
      Av.data(), mpn,
      0.0,
      result.data(), mpn);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_ij_helper(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ik, const std::vector<double>& jk,
    const std::vector<double>& il, const std::vector<double>& jl) {
  // ij_rk = ik . diagonal( rv ) . kj
  contract(ij_rk, jk, CblasTrans, ik, rv);
  contract(ij_rl, jl, CblasTrans, il, rv);

  // ij_rl = il . diagonal( rv ) . lj
  if (CVMP4 >= 3) {
    contract(ij_wk, jk, CblasTrans, ik, wgt);
    contract(ij_wl, jl, CblasTrans, il, wgt);
  }

  // i_kl = ik * il
  std::transform(ik.begin(), ik.end(), il.begin(), i_kl.begin(), std::multiplies<>());
  std::transform(jk.begin(), jk.end(), jl.begin(), j_kl.begin(), std::multiplies<>());

  // T_r = i_kl . diagonal(rv * rv) . kl_j
  contract(T_r, j_kl, CblasTrans, i_kl, r_r);

  // ij_WlWk = ij_rk * ij_rl - T_r
  std::transform(std::begin(ij_rk), std::end(ij_rk), std::begin(ij_rl), std::begin(T_w), std::multiplies<>());
  std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
  cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

  cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
  emp4 += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;                   // r r r r
  if (CVMP4 >= 1) {
    control[0 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r r r
  }

  if (CVMP4 >= 2) {
    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[3 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w r r
  }

  if (CVMP4 >= 3) {
    // T_r = i_kl . diagonal(wgt * rv) . kl_j
    contract(T_r, j_kl, CblasTrans, i_kl, r_w);

    // T_w = ij_rk * ij_wl - T_r
    std::transform(std::begin(ij_rk), std::end(ij_rk), std::begin(ij_wl), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[6 + offset] += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;    // r r r w
    control[9 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r r w

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[12 + offset] += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;    // r w r w
    control[15 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w r w

    // T_w = ij_rk * ij_wl - T_r
    std::transform(std::begin(ij_wk), std::end(ij_wk), std::begin(ij_rl), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[18 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r w r

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[21 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w w r

    // T_r = i_kl . diagonal(wgt * wgt) . kl_j
    contract(T_r, j_kl, CblasTrans, i_kl, w_w);

    // T_w = ij_wk * ij_wl - T_r
    std::transform(std::begin(ij_wk), std::end(ij_wk), std::begin(ij_wl), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[24 + offset] += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;    // r r w w
    control[27 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r w w

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[30 + offset] += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;   // r w w w
    control[33 + offset] += std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w w w
  }
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_ij_helper_t1(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ik_1, const std::vector<double>& ik_2, const std::vector<double>& ik_3,
    const std::vector<double>& jk,
    const std::vector<double>& il,
    const std::vector<double>& jl_1, const std::vector<double>& jl_2, const std::vector<double>& jl_3
) {
  std::transform(ik_1.begin(), ik_1.end(), ik_2.begin(), ik_.begin(), std::multiplies<>());
  std::transform(ik_.begin(), ik_.end(), ik_3.begin(), ik_.begin(), std::multiplies<>());

  std::transform(jl_1.begin(), jl_1.end(), jl_2.begin(), jl_.begin(), std::multiplies<>());
  std::transform(jl_.begin(), jl_.end(), jl_3.begin(), jl_.begin(), std::multiplies<>());
  mcmp4_ij_helper(constant, emp4, control, offset, ik_, jk, il, jl_);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_ij_helper_t2(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ik_1, const std::vector<double>& ik_2,
    const std::vector<double>& jk_1, const std::vector<double>& jk_2,
    const std::vector<double>& il_1, const std::vector<double>& il_2,
    const std::vector<double>& jl_1, const std::vector<double>& jl_2
) {
  std::transform(ik_1.begin(), ik_1.end(), ik_2.begin(), ik_.begin(), std::multiplies<>());
  std::transform(jk_1.begin(), jk_1.end(), jk_2.begin(), jk_.begin(), std::multiplies<>());
  std::transform(il_1.begin(), il_1.end(), il_2.begin(), il_.begin(), std::multiplies<>());
  std::transform(jl_1.begin(), jl_1.end(), jl_2.begin(), jl_.begin(), std::multiplies<>());
  mcmp4_ij_helper(constant, emp4, control, offset, ik_, jk_, il_, jl_);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_ij_helper_t3(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ik,
    const std::vector<double>& jk_1, const std::vector<double>& jk_2, const std::vector<double>& jk_3,
    const std::vector<double>& il_1, const std::vector<double>& il_2, const std::vector<double>& il_3,
    const std::vector<double>& jl
) {
  std::transform(jk_1.begin(), jk_1.end(), jk_2.begin(), jk_.begin(), std::multiplies<>());
  std::transform(jk_.begin(), jk_.end(), jk_3.begin(), jk_.begin(), std::multiplies<>());

  std::transform(il_1.begin(), il_1.end(), il_2.begin(), il_.begin(), std::multiplies<>());
  std::transform(il_.begin(), il_.end(), il_3.begin(), il_.begin(), std::multiplies<>());
  mcmp4_ij_helper(constant, emp4, control, offset, ik, jk_, il_, jl);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ij_fast(double& emp4, std::vector<double>& control, const OVPs& ovps) {
  constexpr int offset = 0 + CVMP4*(2 + CVMP4*(8 + CVMP4*(-5 + CVMP4))) / 2;
  mcmp4_ij_helper_t1(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t1(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t1(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t1(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t1(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t1(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t1(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t1(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22);

  mcmp4_ij_helper_t2(1, emp4, control, offset,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][1].s_22, ovps.v_set[1][1].s_11,
      ovps.v_set[2][0].s_21, ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22);
  mcmp4_ij_helper_t2(1, emp4, control, offset,
      ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.o_set[1][1].s_22, ovps.o_set[1][1].s_11,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_11, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t2(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_21, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t2(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_11, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t2(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t2(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t2(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_21, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t2(-2, emp4, control, offset,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][1].s_22, ovps.v_set[1][1].s_11,
      ovps.v_set[2][0].s_22, ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22);
  mcmp4_ij_helper_t2(-2, emp4, control, offset,
      ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[1][1].s_22, ovps.o_set[1][1].s_11,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_11, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_11, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_22);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_21, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_21);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t2(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_21, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12);
  mcmp4_ij_helper_t2(-16, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11);

  mcmp4_ij_helper_t3(2, emp4, control, offset,
      ovps.v_set[1][0].s_22, ovps.o_set[1][1].s_11, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_22,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][0].s_12, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t3(2, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][1].s_21, ovps.v_set[1][1].s_11, ovps.v_set[1][1].s_22,
      ovps.v_set[2][0].s_11, ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11);
  mcmp4_ij_helper_t3(8, emp4, control, offset,
      ovps.v_set[1][0].s_11, ovps.o_set[1][1].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_22,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][0].s_22, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t3(8, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][1].s_21, ovps.v_set[1][1].s_12, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_12, ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11);
  mcmp4_ij_helper_t3(-4, emp4, control, offset,
      ovps.v_set[1][0].s_12, ovps.o_set[1][1].s_11, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_22,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][0].s_22, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t3(-4, emp4, control, offset,
      ovps.v_set[1][0].s_21, ovps.o_set[1][1].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_22,
      ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][0].s_12, ovps.v_set[2][1].s_11);
  mcmp4_ij_helper_t3(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][1].s_21, ovps.v_set[1][1].s_11, ovps.v_set[1][1].s_22,
      ovps.v_set[2][0].s_12, ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][1].s_11);
  mcmp4_ij_helper_t3(-4, emp4, control, offset,
      ovps.o_set[1][0].s_22, ovps.o_set[1][1].s_21, ovps.v_set[1][1].s_12, ovps.v_set[1][1].s_21,
      ovps.v_set[2][0].s_11, ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][1].s_11);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ik_fast(double& emp4, std::vector<double>& control, const OVPs& ovps) {
  constexpr int offset = 1 + CVMP4*(2 + CVMP4*(8 + CVMP4*(-5 + CVMP4))) / 2;
  mcmp4_ij_helper_t1(  4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_21,
      ovps.v_set[2][0].s_22, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t1(  4, emp4, control, offset,
      ovps.v_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t1(  4, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_21, ovps.v_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t1( -2, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_21,
      ovps.v_set[2][0].s_21, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_12);
  mcmp4_ij_helper_t1( -2, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_21, ovps.v_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t1( -8, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_21,
      ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t1( -8, emp4, control, offset,
      ovps.v_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21,
      ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t1(  4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_21,
      ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_12);

  mcmp4_ij_helper_t2(  1, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.v_set[2][0].s_21, ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22);
  mcmp4_ij_helper_t2(  1, emp4, control, offset,
    ovps.v_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_22, ovps.v_set[1][1].s_11,
    ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2(  2, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t2(  2, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2(  2, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_ij_helper_t2(  2, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2(  8, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t2(  8, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t2(  8, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t2(  8, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_ij_helper_t2( -2, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.o_set[0][0].s_11, ovps.o_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.v_set[2][0].s_22, ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22);
  mcmp4_ij_helper_t2( -2, emp4, control, offset,
    ovps.v_set[0][0].s_22, ovps.v_set[0][0].s_11, ovps.v_set[1][1].s_22, ovps.v_set[1][1].s_11,
    ovps.o_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_ij_helper_t2( -4, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_ij_helper_t2(-16, emp4, control, offset,
    ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_21,
    ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);

  mcmp4_ij_helper_t3(  4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.o_set[1][1].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
      ovps.v_set[2][0].s_11, ovps.v_set[2][0].s_22, ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11);
  mcmp4_ij_helper_t3(  4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.o_set[1][1].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
      ovps.v_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11);
  mcmp4_ij_helper_t3(  4, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_22, ovps.v_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t3(  4, emp4, control, offset,
      ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_21, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_22, ovps.v_set[2][0].s_12, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t3( -2, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.o_set[1][1].s_22, ovps.v_set[1][1].s_21, ovps.o_set[1][1].s_11,
      ovps.v_set[2][0].s_11, ovps.v_set[2][0].s_22, ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11);
  mcmp4_ij_helper_t3( -2, emp4, control, offset,
      ovps.v_set[0][0].s_22, ovps.v_set[1][1].s_11, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_22, ovps.v_set[2][0].s_12, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11);
  mcmp4_ij_helper_t3( -8, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.o_set[1][1].s_22, ovps.v_set[1][1].s_22, ovps.o_set[1][1].s_11,
      ovps.v_set[2][0].s_12, ovps.v_set[2][0].s_21, ovps.o_set[2][0].s_12, ovps.o_set[2][2].s_11);
  mcmp4_ij_helper_t3( -8, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.v_set[1][1].s_21, ovps.v_set[1][1].s_12, ovps.o_set[1][1].s_21,
      ovps.o_set[2][0].s_22, ovps.v_set[2][0].s_22, ovps.o_set[2][0].s_11, ovps.v_set[2][2].s_11);
}

template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_il_helper(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ij, const std::vector<double>& jl,
    const std::vector<double>& ik, const std::vector<double>& kl) {
  std::transform(ij.begin(), ij.end(), ik.begin(), i_kl.begin(), std::multiplies<>());
  std::transform(jl.begin(), jl.end(), kl.begin(), j_kl.begin(), std::multiplies<>());

  // contract k and j
  contract(ij_rl, jl, CblasNoTrans, ij, rv);
  contract(ij_rk, kl, CblasNoTrans, ik, rv);

  if (CVMP4 >= 3) {
    contract(ij_wl, jl, CblasNoTrans, ij, wgt);
    contract(ij_wk, kl, CblasNoTrans, ik, wgt);
  }

  // build jrkr
  contract(T_r, j_kl, CblasNoTrans, i_kl, r_r);
  // combin ij_rk * ij_rl - T_r
  std::transform(std::begin(ij_rl), std::end(ij_rl), std::begin(ij_rk), std::begin(T_w), std::multiplies<>());
  std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
  cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

  cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
  emp4 += std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant; // r r r r
  if (CVMP4 >= 1) {
    control[0 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r r r
  }

  if (CVMP4 >= 2) {
    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[3 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r r w
    control[6 + offset] +=  std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;   // r r r w
  }

  if (CVMP4 >= 3) {
    contract(T_r, j_kl, CblasNoTrans, i_kl, r_w);
    // build jw kr
    std::transform(std::begin(ij_wl), std::end(ij_wl), std::begin(ij_rk), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[9 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w r r

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[12 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w r w
    control[15 + offset] +=  std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;   // r w r w

    // build jr kw
    std::transform(std::begin(ij_rl), std::end(ij_rl), std::begin(ij_wk), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[18 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r w r

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[21 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w r w w
    control[24 + offset] +=  std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;   // r r w w

    // build jw kw
    contract(T_r, j_kl, CblasNoTrans, i_kl, w_w);
    std::transform(std::begin(ij_wl), std::end(ij_wl), std::begin(ij_wk), std::begin(T_w), std::multiplies<>());
    std::transform(std::begin(T_w), std::end(T_w), std::begin(T_r), std::begin(T_w), std::minus<>());
    cblas_dscal(mpn, 0.0, T_w.data(), mpn+1);

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, rv.data(), 1, 0.0, en_r.data(), 1);
    control[27 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w w r

    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, T_w.data(), mpn, wgt.data(), 1, 0.0, en_r.data(), 1);
    control[30 + offset] +=  std::inner_product(wgt.begin(), wgt.end(), en_r.begin(), 0.0) * constant; // w w w w
    control[33 + offset] +=  std::inner_product(rv.begin(), rv.end(), en_r.begin(), 0.0) * constant;   // r w w w
  }
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_il_helper_t1(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ij_1, const std::vector<double>& ij_2, const std::vector<double>& ij_3,
    const std::vector<double>& jl,
    const std::vector<double>& ik,
    const std::vector<double>& kl_1, const std::vector<double>& kl_2, const std::vector<double>& kl_3
) {
  std::transform(ij_1.begin(), ij_1.end(), ij_2.begin(), ij_.begin(), std::multiplies<>());
  std::transform(ij_3.begin(), ij_3.end(), ij_.begin(), ij_.begin(), std::multiplies<>());
  std::transform(kl_1.begin(), kl_1.end(), kl_2.begin(), kl_.begin(), std::multiplies<>());
  std::transform(kl_3.begin(), kl_3.end(), kl_.begin(), kl_.begin(), std::multiplies<>());
  mcmp4_il_helper(constant, emp4, control, offset, ij_, jl, ik, kl_);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_il_helper_t2(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ij_1, const std::vector<double>& ij_2,
    const std::vector<double>& jl_1, const std::vector<double>& jl_2,
    const std::vector<double>& ik_1, const std::vector<double>& ik_2,
    const std::vector<double>& kl_1, const std::vector<double>& kl_2
) {
  std::transform(ij_1.begin(), ij_1.end(), ij_2.begin(), ij_.begin(), std::multiplies<>());
  std::transform(jl_1.begin(), jl_1.end(), jl_2.begin(), jl_.begin(), std::multiplies<>());
  std::transform(ik_1.begin(), ik_1.end(), ik_2.begin(), ik_.begin(), std::multiplies<>());
  std::transform(kl_1.begin(), kl_1.end(), kl_2.begin(), kl_.begin(), std::multiplies<>());
  mcmp4_il_helper(constant, emp4, control, offset, ij_, jl_, ik_, kl_);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_il_helper_t3(double constant,
    double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& ij,
    const std::vector<double>& jl_1, const std::vector<double>& jl_2, const std::vector<double>& jl_3,
    const std::vector<double>& ik_1, const std::vector<double>& ik_2, const std::vector<double>& ik_3,
    const std::vector<double>& kl) {
  std::transform(jl_1.begin(), jl_1.end(), jl_2.begin(), jl_.begin(), std::multiplies<>());
  std::transform(jl_3.begin(), jl_3.end(), jl_.begin(), jl_.begin(), std::multiplies<>());
  std::transform(ik_1.begin(), ik_1.end(), ik_2.begin(), ik_.begin(), std::multiplies<>());
  std::transform(ik_3.begin(), ik_3.end(), ik_.begin(), ik_.begin(), std::multiplies<>());
  mcmp4_il_helper(constant, emp4, control, offset, ij, jl_, ik_, kl);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_il_fast(double& emp4, std::vector<double>& control, const OVPs& ovps) {
  constexpr int offset = 2 + CVMP4*(2 + CVMP4*(8 + CVMP4*(-5 + CVMP4))) / 2;
  mcmp4_il_helper_t1( 2, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12,
      ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t1( 2, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[0][0].s_21, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t1( 8, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12,
      ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t1( 8, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_11, ovps.v_set[0][0].s_22, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t1( -4, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12,
      ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t1( -4, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12,
      ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t1( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_11, ovps.v_set[0][0].s_22, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t1( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.v_set[0][0].s_21, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22, ovps.v_set[2][2].s_11);

  mcmp4_il_helper_t2( 1, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22,
      ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( 1, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.v_set[0][0].s_21, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22);
  mcmp4_il_helper_t2( 2, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t2( 2, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( 2, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t2( 2, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( 8, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t2( 8, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t2( 8, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t2( 8, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t2( -2, emp4, control, offset,
      ovps.o_set[0][0].s_11, ovps.o_set[0][0].s_22, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22,
      ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( -2, emp4, control, offset,
      ovps.v_set[0][0].s_11, ovps.v_set[0][0].s_22, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_11, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_il_helper_t2( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_il_helper_t2(-16, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);

  mcmp4_il_helper_t3( 2, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t3( 2, emp4, control, offset,
      ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11);
  mcmp4_il_helper_t3( 8, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t3( 8, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11);
  mcmp4_il_helper_t3( -4, emp4, control, offset,
      ovps.o_set[0][0].s_21, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_11, ovps.v_set[1][0].s_22, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t3( -4, emp4, control, offset,
      ovps.o_set[0][0].s_22, ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22, ovps.v_set[2][1].s_12,
      ovps.o_set[1][0].s_12, ovps.v_set[1][0].s_12, ovps.v_set[1][0].s_21, ovps.v_set[2][2].s_11);
  mcmp4_il_helper_t3( -4, emp4, control, offset,
      ovps.v_set[0][0].s_12, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_22, ovps.o_set[2][2].s_11);
  mcmp4_il_helper_t3( -4, emp4, control, offset,
      ovps.v_set[0][0].s_22, ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[1][0].s_11, ovps.o_set[1][0].s_22, ovps.v_set[1][0].s_12, ovps.o_set[2][2].s_11);
}

template <int CVMP4>
std::array<double, 4> MCMP4<CVMP4>::contract_jk(
    int walker,
    const std::vector<double>& T, 
    const std::vector<double>& jk, const std::vector<double>& ik, const std::vector<double>& ij) {
  std::array<double, 4> out;
  std::transform(T.begin(), T.end(), jk.begin(), Av.begin(), std::multiplies<>());

  std::transform(rv.begin(), rv.end(), ik.begin() + walker * mpn, ij_rk.begin(), std::multiplies<>());
  cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, Av.data(), mpn, ij_rk.data(), 1, 0.0, en_r.data(), 1); // kr ?l

  std::transform(rv.begin(), rv.end(), ij.begin() + walker * mpn, ij_rk.begin(), std::multiplies<>());
  out[0] = std::inner_product(en_r.begin(), en_r.end(), ij_rk.begin(), 0.0); // jr kr l?
  if (CVMP4 >= 2) {
    std::transform(wgt.begin(), wgt.end(), ik.begin() + walker * mpn, ij_wk.begin(), std::multiplies<>());
    cblas_dgemv(CblasColMajor, CblasTrans, mpn, mpn, 1.0, Av.data(), mpn, ij_wk.data(), 1, 0.0, en_w.data(), 1); // kw ?l
    out[1] = std::inner_product(en_w.begin(), en_w.end(), ij_rk.begin(), 0.0); // jr kw l?
  }

  if (CVMP4 >= 3) {
    std::transform(wgt.begin(), wgt.end(), ij.begin() + walker * mpn, ij_wk.begin(), std::multiplies<>());
    out[2] = std::inner_product(en_r.begin(), en_r.end(), ij_wk.begin(), 0.0); // jw kr l?
    out[3] = std::inner_product(en_w.begin(), en_w.end(), ij_wk.begin(), 0.0); // jw kw l?
  }

  return out;
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ijkl_helper(double& emp4, std::vector<double>& control, int offset,
    const std::vector<double>& constants,
    const std::vector<const std::vector<double>*>& ij,
    const std::vector<const std::vector<double>*>& ik,
    const std::vector<double>& il,
    const std::vector<const std::vector<double>*>& jk,
    const std::vector<double>& jl,
    const std::vector<double>& kl) {
  std::array<double, 4> contracted_jk;
  double jr_kr_lr, jr_kr_lw, jr_kw_lr, jr_kw_lw, jw_kr_lr, jw_kr_lw, jw_kw_lr, jw_kw_lw;
  for (int i = 0; i < mpn; ++i) {
    std::transform(rv.begin(), rv.end(), il.begin() + i * mpn, en_r.begin(), std::multiplies<>());
    contract(T_r, kl, CblasTrans, jl, en_r);

    if (CVMP4 >= 4) {
      std::transform(wgt.begin(), wgt.end(), il.begin() + i * mpn, en_w.begin(), std::multiplies<>());
      contract(T_w, kl, CblasTrans, jl, en_w);
    }

    jr_kr_lr = 0; jr_kr_lw = 0; jr_kw_lr = 0; jr_kw_lw = 0;
    jw_kr_lr = 0; jw_kr_lw = 0; jw_kw_lr = 0; jw_kw_lw = 0;
    for (auto eqn = 0ull; eqn < constants.size(); ++eqn) {
      contracted_jk = contract_jk(i, T_r, *(jk[eqn]), *(ik[eqn]), *(ij[eqn]));
      jr_kr_lr += constants[eqn] * contracted_jk[0] ;
      if (CVMP4 >= 2) {
        jr_kw_lr += constants[eqn] * contracted_jk[1];
      }
      if (CVMP4 >= 3) {
        jw_kr_lr += constants[eqn] * contracted_jk[2];
        jw_kw_lr += constants[eqn] * contracted_jk[3];
      }

      if (CVMP4 >= 4) {
        contracted_jk = contract_jk(i, T_w, *(jk[eqn]), *(ik[eqn]), *(ij[eqn]));
        jr_kr_lw += constants[eqn] * contracted_jk[0];
        jr_kw_lw += constants[eqn] * contracted_jk[1];
        jw_kr_lw += constants[eqn] * contracted_jk[2];
        jw_kw_lw += constants[eqn] * contracted_jk[3];
      }
    }
    emp4        += jr_kr_lr * rv[i];
    if (CVMP4 >= 1) {
      control[offset + 0] += jr_kr_lr * wgt[i];
    }
    if (CVMP4 >= 2) {
      control[offset + 3] += jr_kw_lr * wgt[i];
    }
    if (CVMP4 >= 3) {
      control[offset + 6] += jw_kr_lr * wgt[i];
      control[offset + 9] += jw_kw_lr * wgt[i];
    }
    if (CVMP4 >= 4) {
      control[offset + 12] += jr_kr_lw * rv[i];
      control[offset + 15] += jw_kr_lw * rv[i];
      control[offset + 18] += jr_kw_lw * rv[i];
      control[offset + 21] += jw_kw_lw * rv[i];
      control[offset + 24] += jr_kr_lw * wgt[i];
      control[offset + 27] += jw_kr_lw * wgt[i];
      control[offset + 30] += jr_kw_lw * wgt[i];
      control[offset + 33] += jw_kw_lw * wgt[i];
    }
  }
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ijkl_t1(double& emp4, std::vector<double>& control,
    const std::vector<double> constants,
    const std::vector<const std::vector<double>*> ij,
    const std::vector<const std::vector<double>*> ik,
    const std::vector<double>& il_1, const std::vector<double>& il_2,
    const std::vector<const std::vector<double>*> jk_1, const std::vector<const std::vector<double>*> jk_2,
    const std::vector<double>& jl,
    const std::vector<double>& kl) {
  std::transform(il_1.begin(), il_1.end(), il_2.begin(), il_.begin(), std::multiplies<>());

  for (auto i = 0ull; i < jk_1.size(); ++i) {
    std::transform(jk_1[i]->begin(), jk_1[i]->end(), jk_2[i]->begin(), ext_data[i].begin(), std::multiplies<>());
    ext_ptr[i] = &ext_data[i];
  }

  mcmp4_energy_ijkl_helper(emp4, control, 0, constants, ij, ik, il_, ext_ptr, jl, kl);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ijkl_t2(double& emp4, std::vector<double>& control,
    const std::vector<double> constants,
    const std::vector<const std::vector<double>*> ij,
    const std::vector<const std::vector<double>*> ik_1, const std::vector<const std::vector<double>*> ik_2,
    const std::vector<double>& il,
    const std::vector<const std::vector<double>*> jk,
    const std::vector<double>& jl_1, const std::vector<double>& jl_2,
    const std::vector<double>& kl) {

  for (auto i = 0ull; i < ik_1.size(); ++i) {
    std::transform(ik_1[i]->begin(), ik_1[i]->end(), ik_2[i]->begin(), ext_data[i].begin(), std::multiplies<>());
    ext_ptr[i] = &ext_data[i];
  }

  std::transform(jl_1.begin(), jl_1.end(), jl_2.begin(), jl_.begin(), std::multiplies<>());

  mcmp4_energy_ijkl_helper(emp4, control, 1, constants, ij, ext_ptr, il, jk, jl_, kl);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ijkl_t3(double& emp4, std::vector<double>& control,
    const std::vector<double> constants,
    const std::vector<const std::vector<double>*> ij_1, const std::vector<const std::vector<double>*> ij_2,
    const std::vector<const std::vector<double>*> ik,
    const std::vector<double>& il,
    const std::vector<const std::vector<double>*> jk,
    const std::vector<double>& jl,
    const std::vector<double>& kl_1, const std::vector<double>& kl_2) {
  for (auto i = 0ull; i < ij_1.size(); ++i) {
    std::transform(ij_1[i]->begin(), ij_1[i]->end(), ij_2[i]->begin(), ext_data[i].begin(), std::multiplies<>());
    ext_ptr[i] = &ext_data[i];
  }

  std::transform(kl_1.begin(), kl_1.end(), kl_2.begin(), jl_.begin(), std::multiplies<>());

  mcmp4_energy_ijkl_helper(emp4, control, 2, constants, ext_ptr, ik, il, jk, jl, jl_);
}
template <int CVMP4>
void MCMP4<CVMP4>::mcmp4_energy_ijkl_fast(double& emp4, std::vector<double>& control, const OVPs& ovps) {
  mcmp4_energy_ijkl_t1(emp4, control,
      { -2, -2, -2, 4, 4, 4, 4, -8},
      {&ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_11},
      ovps.o_set[2][0].s_11, ovps.o_set[2][0].s_22,
      {&ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22},
      {&ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_12, ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      {  2, 2, -2, -4, -4, 4},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_11},
      {&ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t1(emp4, control,
      { -2, 2, 2, 4, -4, -4},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11},
      {&ovps.o_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_12, ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      {  2, -2, -4, -4, 4, 8},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_21},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_11},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22},
      {&ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t1(emp4, control,
      { -2, 2, 4, -4, -4, 8},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_11},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_21},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_21,
      {&ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22},
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_12, ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      { -4, -4, 4, 8, 8, -8},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_11},
      {&ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      {  4, -4, -4, -8, 8, 8},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_12,
      {&ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11},
      {&ovps.o_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_11, ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      {  2, 2, -2, -4, -4, 4},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12},
      {&ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_11},
      {&ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      { -2, 2, 2, 4, -4, -4},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_11, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_12},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12, ovps.v_set[2][0].s_22,
      {&ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11},
      {&ovps.o_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_11, ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t1(emp4, control,
      { -2, -2, -2, 4, 4, 4, 4, -8},
      {&ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_21},
      {&ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11},
      ovps.v_set[2][0].s_11, ovps.v_set[2][0].s_22,
      {&ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_22},
      {&ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.o_set[2][2].s_11);

  mcmp4_energy_ijkl_t2(emp4, control,
      {2, 2, 2, -4, -4, -4, -4, 8},
      {&ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_12},
      {&ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22},
      ovps.v_set[2][0].s_12 ,
      {&ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_11},
      ovps.o_set[2][1].s_11, ovps.o_set[2][1].s_22,
      ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {-4, 4, 4, 8, -8, -8},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_11,
      ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t2(emp4, control,
      {-2, -2, 2, 4, 4, -4},
      {&ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.v_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_11,
      ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {2, -2, -2, -4, 4, 4},
      {&ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_11, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_12},
      ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_21,
      ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t2(emp4, control,
      {-2, 2, 4, 4, -4, -8},
      {&ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_11},
      {&ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22},
      ovps.v_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_21},
      ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_21,
      ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {2, -2, -2, -4, 4, 4},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11},
      ovps.o_set[2][0].s_12 ,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_12,
      ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {4, 4, -4, -8, -8, 8},
      {&ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_12,
      ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {2, -2, -4, 4, 4, -8},
      {&ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_12},
      {&ovps.v_set[1][0].s_11, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_11},
      ovps.o_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {-2, -2, 2, 4, 4, -4},
      {&ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_21, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12},
      ovps.o_set[2][1].s_12, ovps.v_set[2][1].s_22,
      ovps.o_set[2][2].s_11);
  mcmp4_energy_ijkl_t2(emp4, control,
      {2, 2, 2, -4, -4, -4, -4, 8},
      {&ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_12},
      {&ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      {&ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_22},
      ovps.o_set[2][0].s_12 ,
      {&ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_11},
      ovps.v_set[2][1].s_11, ovps.v_set[2][1].s_22,
      ovps.o_set[2][2].s_11);


  mcmp4_energy_ijkl_t3(emp4, control,
      { -2, -2, -2, 4, 4, 4, 4, -8},
      {&ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22},
      {&ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_11},
      ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.o_set[1][1].s_22, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_12},
      ovps.v_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.o_set[2][2].s_22);
  mcmp4_energy_ijkl_t3(emp4, control,
      { -4, -4, 4, 8, 8, -8},
      {&ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_11},
      {&ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t3(emp4, control,
      { -4, -8, 8, -4, 4, 8},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11},
      {&ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.v_set[2][0].s_12,
      {&ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_11);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 2, 2, -2, -4, -4, 4},
      {&ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_11},
      {&ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 2, 4, -4, 2, -2, -4},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11},
      {&ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_11, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21},
      ovps.v_set[2][0].s_12,
      {&ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_21);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 2, 2, -2, -4, -4, 4},
      {&ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_11},
      {&ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.v_set[2][1].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 2, 4, -4, 2, -2, -4},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_11},
      {&ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12, &ovps.v_set[0][0].s_22},
      {&ovps.o_set[1][0].s_12, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_12, &ovps.o_set[1][0].s_22, &ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_22},
      ovps.v_set[2][0].s_11,
      {&ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_22, &ovps.v_set[1][1].s_22},
      ovps.o_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_12);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 2, -2, -4, -4, 4, 8},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_11, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.o_set[0][0].s_22},
      {&ovps.v_set[0][0].s_11, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_21, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_12, &ovps.o_set[1][0].s_21, &ovps.v_set[1][0].s_12},
      ovps.o_set[2][0].s_12,
      {&ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_11, &ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.o_set[1][1].s_11},
      ovps.v_set[2][1].s_11, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_energy_ijkl_t3(emp4, control,
      { -2, -4, 8, 2, -4, 4},
      {&ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_11, &ovps.o_set[0][0].s_12, &ovps.o_set[0][0].s_21},
      {&ovps.o_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.o_set[0][0].s_12},
      {&ovps.v_set[1][0].s_22, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_11, &ovps.o_set[1][0].s_21, &ovps.o_set[1][0].s_21, &ovps.v_set[1][0].s_22},
      ovps.v_set[2][0].s_11,
      {&ovps.o_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_12, &ovps.o_set[1][1].s_21},
      ovps.o_set[2][1].s_12, ovps.o_set[2][2].s_11, ovps.v_set[2][2].s_22);
  mcmp4_energy_ijkl_t3(emp4, control,
      { 4, -2, -2, 4, -2, 4, 4, -8},
      {&ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_21, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22, &ovps.o_set[0][0].s_22},
      {&ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_12, &ovps.v_set[0][0].s_22, &ovps.v_set[0][0].s_22},
      {&ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.v_set[1][0].s_12, &ovps.v_set[1][0].s_21, &ovps.v_set[1][0].s_22, &ovps.v_set[1][0].s_11, &ovps.v_set[1][0].s_12},
      ovps.o_set[2][0].s_12,
      {&ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_22, &ovps.v_set[1][1].s_21, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_11, &ovps.v_set[1][1].s_12, &ovps.v_set[1][1].s_11},
      ovps.o_set[2][1].s_11, ovps.v_set[2][2].s_11, ovps.v_set[2][2].s_22);
}

template <int CVMP4>
void MCMP4<CVMP4>::energy(double& emp, std::vector<double>& control, OVPs& ovps, Electron_Pair_List* electron_pair_list, Tau* tau) {
  double en4 = 0.0;
  std::vector<double> ctrl(control.size(), 0.0);

  std::copy(electron_pair_list->rv.begin(), electron_pair_list->rv.end(), rv.begin());
  std::transform(electron_pair_list->wgt.begin(), electron_pair_list->wgt.end(), wgt.begin(), [](double x){return 1.0/x;});
  std::transform(rv.begin(), rv.end(), rv.begin(), r_r.begin(), std::multiplies<>());
  std::transform(rv.begin(), rv.end(), wgt.begin(), r_w.begin(), std::multiplies<>());
  std::transform(wgt.begin(), wgt.end(), wgt.begin(), w_w.begin(), std::multiplies<>());

  mcmp4_energy_ij_fast(en4, ctrl, ovps);
  mcmp4_energy_ik_fast(en4, ctrl, ovps);
  mcmp4_energy_il_fast(en4, ctrl, ovps);
  mcmp4_energy_ijkl_fast(en4, ctrl, ovps);

  auto nsamp_tauwgt = tau->get_wgt(3);
  nsamp_tauwgt /= static_cast<double>(mpn);
  nsamp_tauwgt /= static_cast<double>(mpn - 1);
  nsamp_tauwgt /= static_cast<double>(mpn - 2);
  nsamp_tauwgt /= static_cast<double>(mpn - 3);
  emp = emp + en4 * nsamp_tauwgt;
  if (CVMP4 >= 1) {
    std::transform(ctrl.begin(), ctrl.end(), control.begin(), control.begin(), [&](double c, double total) { return total + c * nsamp_tauwgt; });
  }
}

MCMP* create_MCMP4(int cv_level, Electron_Pair_List* electron_pair_list) {
  MCMP* mcmp = nullptr;
  if (cv_level == 0) {
    mcmp = new MCMP4<0>(electron_pair_list);
  } else if (cv_level == 1) {
    mcmp = new MCMP4<1>(electron_pair_list);
  } else if (cv_level == 2) {
    mcmp = new MCMP4<2>(electron_pair_list);
  } else if (cv_level == 3) {
    mcmp = new MCMP4<3>(electron_pair_list);
  } else if (cv_level == 4) {
    mcmp = new MCMP4<4>(electron_pair_list);
  }
  
  if (mcmp == nullptr) {
    std::cerr << "MCMP4 not supported with cv level " << cv_level << "\n";
    exit(0);
  }
  return mcmp;
}
