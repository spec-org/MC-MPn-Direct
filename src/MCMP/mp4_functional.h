#ifndef MP4_Functional_H_
#define MP4_Functional_H_

#include "mp_functional.h"
#include "cblas.h"

template <int CVMP4>
class MP4_Functional : public Standard_MP_Functional {
 public:
  MP4_Functional(Electron_Pair_List* electron_pair_list) : Standard_MP_Functional(CVMP4*(100 + CVMP4*(-135 + CVMP4*(68 - 9*CVMP4))) / 4, 3, "24"),
      mpn(electron_pair_list->size()),
      rv(mpn),
      wgt(mpn),
      r_r(mpn),
      r_w(mpn),
      w_w(mpn),
      en_r(mpn),
      en_w(mpn),
      ij_(mpn * mpn),
      ik_(mpn * mpn),
      il_(mpn * mpn),
      jk_(mpn * mpn),
      jl_(mpn * mpn),
      kl_(mpn * mpn),
      ext_ptr(8),
      ext_data(8,
      std::vector<double>(mpn*mpn)),
      i_kl(mpn * mpn),
      j_kl(mpn * mpn),
      ij_rk(mpn * mpn),
      ij_wk(mpn * mpn),
      ij_rl(mpn * mpn),
      ij_wl(mpn * mpn),
      T_r(mpn * mpn),
      T_w(mpn * mpn),
      Av(mpn * mpn) {}
  void energy(double& emp, std::vector<double>& control, OVPS_Host&, Electron_Pair_List*, Tau*) override;

 private:
  void contract(std::vector<double>& result, const std::vector<double>& A, CBLAS_TRANSPOSE A_trans, const std::vector<double>& B, const std::vector<double>& v);
  void mcmp4_ij_helper(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ik, const std::vector<double>& jk,
      const std::vector<double>& il, const std::vector<double>& jl);
  void mcmp4_ij_helper_t1(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ik_1, const std::vector<double>& ik_2, const std::vector<double>& ik_3,
      const std::vector<double>& jk,
      const std::vector<double>& il,
      const std::vector<double>& jl_1, const std::vector<double>& jl_2, const std::vector<double>& jl_3);
  void mcmp4_ij_helper_t2(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ik_1, const std::vector<double>& ik_2,
      const std::vector<double>& jk_1, const std::vector<double>& jk_2,
      const std::vector<double>& il_1, const std::vector<double>& il_2,
      const std::vector<double>& jl_1, const std::vector<double>& jl_2);
  void mcmp4_ij_helper_t3(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ik,
      const std::vector<double>& jk_1, const std::vector<double>& jk_2, const std::vector<double>& jk_3,
      const std::vector<double>& il_1, const std::vector<double>& il_2, const std::vector<double>& il_3,
      const std::vector<double>& jl);
  void mcmp4_energy_ij_fast(double& emp4, std::vector<double>& control, const OVPS_Host& ovps);
  void mcmp4_energy_ik_fast(double& emp4, std::vector<double>& control, const OVPS_Host& ovps);

  void mcmp4_il_helper(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ij, const std::vector<double>& jl,
      const std::vector<double>& ik, const std::vector<double>& kl);
  void mcmp4_il_helper_t1(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ij_1, const std::vector<double>& ij_2, const std::vector<double>& ij_3,
      const std::vector<double>& jl,
      const std::vector<double>& ik,
      const std::vector<double>& kl_1, const std::vector<double>& kl_2, const std::vector<double>& kl_3);
  void mcmp4_il_helper_t2(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ij_1, const std::vector<double>& ij_2,
      const std::vector<double>& jl_1, const std::vector<double>& jl_2,
      const std::vector<double>& ik_1, const std::vector<double>& ik_2,
      const std::vector<double>& kl_1, const std::vector<double>& kl_2);
  void mcmp4_il_helper_t3(double constant,
      double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& ij,
      const std::vector<double>& jl_1, const std::vector<double>& jl_2, const std::vector<double>& jl_3,
      const std::vector<double>& ik_1, const std::vector<double>& ik_2, const std::vector<double>& ik_3,
      const std::vector<double>& kl);
  void mcmp4_energy_il_fast(double& emp4, std::vector<double>& control, const OVPS_Host& ovps);

  std::array<double, 4> contract_jk(int walker,
      const std::vector<double>& T,
      const std::vector<double>& jk, const std::vector<double>& ik, const std::vector<double>& ij);
  void mcmp4_energy_ijkl_helper(double& emp4, std::vector<double>& control, int offset,
      const std::vector<double>& constants,
      const std::vector<const std::vector<double>*>& ij,
      const std::vector<const std::vector<double>*>& ik,
      const std::vector<double>& il,
      const std::vector<const std::vector<double>*>& jk,
      const std::vector<double>& jl,
      const std::vector<double>& kl);
  void mcmp4_energy_ijkl_t1(double& emp4, std::vector<double>& control,
      const std::vector<double> constants,
      const std::vector<const std::vector<double>*> ij,
      const std::vector<const std::vector<double>*> ik,
      const std::vector<double>& il_1, const std::vector<double>& il_2,
      const std::vector<const std::vector<double>*> jk_1, const std::vector<const std::vector<double>*> jk_2,
      const std::vector<double>& jl,
      const std::vector<double>& kl);
  void mcmp4_energy_ijkl_t2(double& emp4, std::vector<double>& control,
      const std::vector<double> constants,
      const std::vector<const std::vector<double>*> ij,
      const std::vector<const std::vector<double>*> ik_1, const std::vector<const std::vector<double>*> ik_2,
      const std::vector<double>& il,
      const std::vector<const std::vector<double>*> jk,
      const std::vector<double>& jl_1, const std::vector<double>& jl_2,
      const std::vector<double>& kl);
  void mcmp4_energy_ijkl_t3(double& emp4, std::vector<double>& control,
      const std::vector<double> constants,
      const std::vector<const std::vector<double>*> ij_1, const std::vector<const std::vector<double>*> ij_2,
      const std::vector<const std::vector<double>*> ik,
      const std::vector<double>& il,
      const std::vector<const std::vector<double>*> jk,
      const std::vector<double>& jl,
      const std::vector<double>& kl_1, const std::vector<double>& kl_2);
  void mcmp4_energy_ijkl_fast(double& emp4, std::vector<double>& control, const OVPS_Host& ovps);

  int mpn;
  std::vector<double> rv;
  std::vector<double> wgt;
  std::vector<double> r_r;
  std::vector<double> r_w;
  std::vector<double> w_w;
  std::vector<double> en_r;
  std::vector<double> en_w;

  std::vector<double> ij_;
  std::vector<double> ik_;
  std::vector<double> il_;
  std::vector<double> jk_;
  std::vector<double> jl_;
  std::vector<double> kl_;

  std::vector<const std::vector<double>*> ext_ptr;
  std::vector<std::vector<double>> ext_data;

  std::vector<double> i_kl;
  std::vector<double> j_kl;
  std::vector<double> ij_rk;
  std::vector<double> ij_wk;
  std::vector<double> ij_rl;
  std::vector<double> ij_wl;
  std::vector<double> T_r;
  std::vector<double> T_w;
  std::vector<double> Av;
};

template class MP4_Functional<0>;
template class MP4_Functional<1>;
template class MP4_Functional<2>;
template class MP4_Functional<3>;
template class MP4_Functional<4>;

MP_Functional* create_MP4_Functional(int cv_level, Electron_Pair_List* electron_pair_list);
#endif  // MP4_Functional_H_