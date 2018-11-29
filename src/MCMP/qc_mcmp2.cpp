//
// Created by aedoran on 6/1/18.
//

#ifdef HAVE_CONFIG_H_
#include "config.h"
#endif

#include <algorithm>
#include <functional>

#include "../qc_monte.h"

void MP::mcmp2_energy_fast(double& emp2, std::vector<double>& control) {
  /* This functions computes the second-order MP2 energy for a single MC steps
   * The traces of the HF greens's functions are computed in-place in this function.
   * If higher order corrections are desired, the mcmp2_energy fucntions is more appropriate as it does not compute the HF green's function traces
   *
   * The wavefunction psi is obtained from the basis varaible declared in the MP class
   * The imaginary tiem varaiables are obtained from tau_values declared in the MP class
   *
   * Input Parameters:
   *   emp2: reference to double. stores the energy calcaulted by this function
   *   control: reference to vector<double>. stores the control varaites to the mp2 energy for calcaulted by this function.
   */
  int im, am;
  double icount2 = static_cast<double>(el_pair_list.size()) * static_cast<double>(el_pair_list.size() - 1);

  double o_13, o_14, o_23, o_24;
  double v_13, v_14, v_23, v_24;

  auto tau_values = tau.get_exp_tau(0, 0);

  emp2 = 0.0;
#if MP2CV >= 1
  std::fill(control.begin(), control.end(), 0.0);
#endif

  std::array<double, 2> en;
  std::array<double, 2> en_rj{0, 0};
#if MP2CV >= 2
  std::array<double, 2> en_wj{0, 0};
#endif

  for (auto it = 0; it < el_pair_list.size(); ++it) {
    en_rj.fill(0.0);
#if MP2CV >= 2
    en_wj.fill(0.0);
#endif
    std::transform(basis.h_basis.psi1 + it * (ivir2-iocc1), basis.h_basis.psi1 + (it+1) * (ivir2-iocc1), tau_values.begin() + iocc1, basis.h_basis.psiTau1, std::multiplies<>());
    std::transform(basis.h_basis.psi2 + it * (ivir2-iocc1), basis.h_basis.psi2 + (it+1) * (ivir2-iocc1), tau_values.begin() + iocc1, basis.h_basis.psiTau2, std::multiplies<>());
    for (auto jt = it + 1; jt != el_pair_list.size(); jt++) {
      o_13 = 0.0;
      o_14 = 0.0;
      o_23 = 0.0;
      o_24 = 0.0;
      v_13 = 0.0;
      v_14 = 0.0;
      v_23 = 0.0;
      v_24 = 0.0;

      // compute the traces of the the HF green's functions
      for (im = 0; im < iocc2-iocc1; im++) {
        o_13 = o_13 + basis.h_basis.psiTau1[im] * basis.h_basis.psi1[jt * (ivir2-iocc1) + (im)];
        o_14 = o_14 + basis.h_basis.psiTau1[im] * basis.h_basis.psi2[jt * (ivir2-iocc1) + (im)];
        o_23 = o_23 + basis.h_basis.psiTau2[im] * basis.h_basis.psi1[jt * (ivir2-iocc1) + (im)];
        o_24 = o_24 + basis.h_basis.psiTau2[im] * basis.h_basis.psi2[jt * (ivir2-iocc1) + (im)];
      }
      for (am = iocc2-iocc1; am < ivir2-iocc1; am++) {
        v_13 = v_13 + basis.h_basis.psiTau1[am] * basis.h_basis.psi1[jt * (ivir2-iocc1) + (am)];
        v_14 = v_14 + basis.h_basis.psiTau1[am] * basis.h_basis.psi2[jt * (ivir2-iocc1) + (am)];
        v_23 = v_23 + basis.h_basis.psiTau2[am] * basis.h_basis.psi1[jt * (ivir2-iocc1) + (am)];
        v_24 = v_24 + basis.h_basis.psiTau2[am] * basis.h_basis.psi2[jt * (ivir2-iocc1) + (am)];
      }

      // compute the energy
      en[0] = (o_13 * o_24 * v_13 * v_24);
      en[1] = (o_14 * o_23 * v_13 * v_24);

      en[0] = en[0] + (o_14 * o_23 * v_14 * v_23);
      en[1] = en[1] + (o_13 * o_24 * v_14 * v_23);

      std::transform(en.begin(), en.end(), en_rj.begin(), en_rj.begin(), [&](double x, double y) {return y + x * el_pair_list[jt].rv;});
#if MP2CV >= 2
      std::transform(en.begin(), en.end(), en_wj.begin(), en_wj.begin(), [&](double x, double y) {return y + x / el_pair_list[jt].wgt;});
#endif
    }
    emp2 += (en_rj[1] - 2.0 * en_rj[0]) * el_pair_list[it].rv;
#if MP2CV >= 1
    std::transform(en_rj.begin(), en_rj.end(), control.begin()+0, control.begin()+0, [&](double x, double y) { return y + x / el_pair_list[it].wgt; });
#endif
#if MP2CV >= 2
    std::transform(en_wj.begin(), en_wj.end(), control.begin()+2, control.begin()+2, [&](double x, double y) { return y + x * el_pair_list[it].rv; });
    std::transform(en_wj.begin(), en_wj.end(), control.begin()+4, control.begin()+4, [&](double x, double y) { return y + x / el_pair_list[it].wgt; });
#endif
  }

  auto tau_wgt = tau.get_wgt(1);
  tau_wgt /= static_cast<double>(el_pair_list.size());
  tau_wgt /= static_cast<double>(el_pair_list.size() - 1);
  emp2 = emp2 * tau_wgt;
#if MP2CV >= 1
  std::transform(control.begin(), control.end(), control.begin(), [&](double c) { return c * tau_wgt; });
#endif
}

void MP::mcmp2_energy(double& emp2, std::vector<double>& control) {
  emp2 = 0.0;
#if MP2CV >= 1
  std::fill(control.begin(), control.end(), 0.0);
#endif

  std::array<double, 2> en;
  std::array<double, 2> en_rj{0, 0};
#if MP2CV >= 2
  std::array<double, 2> en_wj{0, 0};
#endif

  for (auto it = 0; it != iops.iopns[KEYS::MC_NPAIR]; it++) {
    en_rj.fill(0.0);
#if MP2CV >= 2
    en_wj.fill(0.0);
#endif

    for (auto jt = it + 1; jt !=  iops.iopns[KEYS::MC_NPAIR]; jt++) {
      auto ijIndex = it * iops.iopns[KEYS::MC_NPAIR] + jt;
      en[0] = (ovps.o_set[0][0].s_11[ijIndex] * ovps.o_set[0][0].s_22[ijIndex] * ovps.v_set[0][0].s_11[ijIndex] * ovps.v_set[0][0].s_22[ijIndex]);
      en[1] = (ovps.o_set[0][0].s_12[ijIndex] * ovps.o_set[0][0].s_21[ijIndex] * ovps.v_set[0][0].s_11[ijIndex] * ovps.v_set[0][0].s_22[ijIndex]);

      en[0] = en[0] + (ovps.o_set[0][0].s_12[ijIndex] * ovps.o_set[0][0].s_21[ijIndex] * ovps.v_set[0][0].s_12[ijIndex] * ovps.v_set[0][0].s_21[ijIndex]);
      en[1] = en[1] + (ovps.o_set[0][0].s_11[ijIndex] * ovps.o_set[0][0].s_22[ijIndex] * ovps.v_set[0][0].s_12[ijIndex] * ovps.v_set[0][0].s_21[ijIndex]);

      std::transform(en.begin(), en.end(), en_rj.begin(), en_rj.begin(), [&](double x, double y) {return y + x * el_pair_list[jt].rv;});
#if MP2CV >= 2
      std::transform(en.begin(), en.end(), en_wj.begin(), en_wj.begin(), [&](double x, double y) {return y + x / el_pair_list[jt].wgt;});
#endif
    }
    emp2 += (en_rj[1] - 2.0 * en_rj[0]) * el_pair_list[it].rv;
#if MP2CV >= 1
    std::transform(en_rj.begin(), en_rj.end(), control.begin()+0, control.begin()+0, [&](double x, double y) { return y + x / el_pair_list[it].wgt; });
#endif
#if MP2CV >= 2
    std::transform(en_wj.begin(), en_wj.end(), control.begin()+2, control.begin()+2, [&](double x, double y) { return y + x * el_pair_list[it].rv; });
    std::transform(en_wj.begin(), en_wj.end(), control.begin()+4, control.begin()+4, [&](double x, double y) { return y + x / el_pair_list[it].wgt; });
#endif
  }
  auto tau_wgt = tau.get_wgt(1);
  tau_wgt /= static_cast<double>(el_pair_list.size());
  tau_wgt /= static_cast<double>(el_pair_list.size() - 1);
  emp2 = emp2 * tau_wgt;
#if MP2CV >= 1
  std::transform(control.begin(), control.end(), control.begin(), [&](double c) { return c * tau_wgt; });
#endif
}

