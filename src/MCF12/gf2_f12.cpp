#include <algorithm>
#include <iostream>
#include <numeric>

#include "gf2_f12.h"

GF2_F12_V::GF2_F12_V(IOPs& iops, Basis& basis, std::string extension) :
    MCGF(iops, basis, 0, extension, true),
    traces(basis.iocc1, basis.iocc2, basis.ivir1, basis.ivir2, iops.iopns[KEYS::ELECTRON_PAIRS], iops.iopns[KEYS::ELECTRONS]),
    x_traces(iops.iopns[KEYS::ELECTRON_PAIRS], iops.iopns[KEYS::ELECTRONS])
{
  correlation_factor = create_correlation_factor(iops);
  nsamp_pair = 1.0 / static_cast<double>(iops.iopns[KEYS::ELECTRON_PAIRS]);
  nsamp_one_1 = 1.0 / static_cast<double>(iops.iopns[KEYS::ELECTRONS]);
  nsamp_one_2 = nsamp_one_1 / static_cast<double>(iops.iopns[KEYS::ELECTRONS] - 1.0);
}

GF2_F12_V::~GF2_F12_V() {
  delete correlation_factor;
}

void GF2_F12_V::energy_f12(std::vector<std::vector<double>>& egf, std::unordered_map<int, Wavefunction>& wavefunctions, Electron_Pair_List* electron_pair_list, Electron_List* electron_list) {
  calculate_v(egf, wavefunctions, electron_pair_list, electron_list);
}

void GF2_F12_V::calculate_v(std::vector<std::vector<double>>& egf, std::unordered_map<int, Wavefunction>& wavefunctions, Electron_Pair_List* electron_pair_list, Electron_List* electron_list) {
  traces.update_v(wavefunctions);
  correlation_factor->update(electron_pair_list, electron_list);
  for (int band = 0; band < numBand; band++) {
    x_traces.set(band, offBand, wavefunctions);

    double v_1_pair_0_one_int = 0.0;
    for (int ip = 0; ip < electron_pair_list->size(); ++ip) {
      auto f_12 = correlation_factor->f12p[ip];
       v_1_pair_0_one_int += c1 * f_12 * x_traces.x11[ip] * traces.p22[ip] * electron_pair_list->rv[ip];
       v_1_pair_0_one_int += c1 * f_12 * traces.p11[ip] * x_traces.x22[ip] * electron_pair_list->rv[ip];
       v_1_pair_0_one_int += c2 * f_12 * x_traces.x12[ip] * traces.p12[ip] * electron_pair_list->rv[ip];
       v_1_pair_0_one_int += c2 * f_12 * traces.p12[ip] * x_traces.x12[ip] * electron_pair_list->rv[ip];
    }
    v_1_pair_0_one_int *= nsamp_pair;

    double v_1_pair_1_one_int = 0.0;
    for (int ip = 0; ip < electron_pair_list->size(); ++ip) {
      std::array<double, 4> t{0.0, 0.0, 0.0, 0.0};
      for (int io = 0; io < electron_list->size(); ++io) {
        t[0] += correlation_factor->f23[ip][io] * x_traces.x13[ip][io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
        t[1] += correlation_factor->f23[ip][io] * traces.p13[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];

        t[2] += correlation_factor->f23[ip][io] * traces.p23[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
        t[3] += correlation_factor->f23[ip][io] * x_traces.x23[ip][io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
      }
      v_1_pair_1_one_int += c1 * t[0] * traces.p22[ip] * electron_pair_list->rv[ip];
      v_1_pair_1_one_int += c1 * t[1] * x_traces.x22[ip] * electron_pair_list->rv[ip];
      
      v_1_pair_1_one_int += c2 * t[2] * x_traces.x12[ip] * electron_pair_list->rv[ip];
      v_1_pair_1_one_int += c2 * t[3] * traces.p12[ip] * electron_pair_list->rv[ip];
    }
    v_1_pair_1_one_int *= -2.0 * nsamp_pair * nsamp_one_1;

    std::array<double, 2> v_1_pair_2_one_int{0.0, 0.0};
    for (int ip = 0; ip < electron_pair_list->size(); ++ip) {
      for (int io = 0; io < electron_list->size(); ++io) {
        std::array<double, 8> t{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        for (int jo = 0; jo < electron_list->size(); ++jo) {
          if (jo != io) {
            t[0] += correlation_factor->f12o[io][jo] * traces.p23[ip * traces.electrons + jo] * traces.k23[ip * traces.electrons + jo] / electron_list->weight[jo];
            t[1] += correlation_factor->f12o[io][jo] * x_traces.x23[ip][jo] * traces.k23[ip * traces.electrons + jo] / electron_list->weight[jo];

            t[2] += correlation_factor->f12o[io][jo] * x_traces.x13[ip][jo] * traces.k23[ip * traces.electrons + jo] / electron_list->weight[jo];
            t[3] += correlation_factor->f12o[io][jo] * traces.p13[ip * traces.electrons + jo] * traces.k23[ip * traces.electrons + jo] / electron_list->weight[jo];

            t[4] += correlation_factor->f12o[io][jo] * traces.p23[ip * traces.electrons + jo] * traces.v23[ip * traces.electrons + jo] / electron_list->weight[jo];
            t[5] += correlation_factor->f12o[io][jo] * x_traces.x23[ip][jo] * traces.v23[ip * traces.electrons + jo] / electron_list->weight[jo];
               
            t[6] += correlation_factor->f12o[io][jo] * x_traces.x13[ip][jo] * traces.v23[ip * traces.electrons + jo] / electron_list->weight[jo];
            t[7] += correlation_factor->f12o[io][jo] * traces.p13[ip * traces.electrons + jo] * traces.v23[ip * traces.electrons + jo] / electron_list->weight[jo];
          }
        }
        v_1_pair_2_one_int[0] += t[0] * x_traces.x13[ip][io] * traces.k13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
        v_1_pair_2_one_int[0] += t[1] * traces.p13[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
                                     
        v_1_pair_2_one_int[1] += t[2] * traces.p23[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
        v_1_pair_2_one_int[1] += t[3] * x_traces.x23[ip][io] * traces.k13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
                                     
        v_1_pair_2_one_int[0] -= t[4] * x_traces.x13[ip][io] * traces.v13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
        v_1_pair_2_one_int[0] -= t[5] * traces.p13[ip * traces.electrons + io] * traces.v13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
                                    
        v_1_pair_2_one_int[1] -= t[6] * traces.p23[ip * traces.electrons + io] * traces.v13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
        v_1_pair_2_one_int[1] -= t[7] * x_traces.x23[ip][io] * traces.v13[ip * traces.electrons + io] * electron_pair_list->rv[ip] / electron_list->weight[io];
      }
    }
    v_1_pair_2_one_int[0] *= c1 * nsamp_pair * nsamp_one_2;
    v_1_pair_2_one_int[1] *= c2 * nsamp_pair * nsamp_one_2;

    egf[band][0] += v_1_pair_0_one_int + v_1_pair_2_one_int[0] + v_1_pair_2_one_int[1] + v_1_pair_1_one_int;
  }
}

void GF2_F12_V::core(OVPs& ovps, Electron_Pair_List* electron_pair_list) {}

void GF2_F12_V::energy_no_diff(std::vector<std::vector<double>>&, std::unordered_map<int, Wavefunction>&, Electron_Pair_List*, Tau*) {}

void GF2_F12_V::energy_diff(std::vector<std::vector<double>>&, std::unordered_map<int, Wavefunction>&, Electron_Pair_List*, Tau*) {}

GF2_F12_VBX::GF2_F12_VBX(IOPs& iops, Basis& basis) : GF2_F12_V(iops, basis, "f12_VBX") {
  nsamp_one_3 = nsamp_one_2 / static_cast<double>(iops.iopns[KEYS::ELECTRONS]-2);
  nsamp_one_4 = nsamp_one_3 / static_cast<double>(iops.iopns[KEYS::ELECTRONS]-3);
}

void GF2_F12_VBX::energy_f12(std::vector<std::vector<double>>& egf, std::unordered_map<int, Wavefunction>& wavefunctions, Electron_Pair_List* electron_pair_list, Electron_List* electron_list) {
  std::vector<std::vector<double>> e_v(egf.size(), std::vector<double>(egf[0].size(), 0.0));

  calculate_v(e_v, wavefunctions, electron_pair_list, electron_list);
  calculate_bx(egf, wavefunctions, electron_pair_list, electron_list);
  for (int band = 0; band < numBand; band++) {
    egf[band][0] += 2.0 * e_v[band][0];
  }
}

void GF2_F12_VBX::calculate_bx(std::vector<std::vector<double>>& egf, std::unordered_map<int, Wavefunction>& wavefunctions, const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  traces.update_bx(wavefunctions, electron_pair_list, electron_list);

  for (int band = 0; band < numBand; band++) {
    zero();
    x_traces.set(band, offBand, wavefunctions);
    x_traces.set_derivative_traces(band, offBand, wavefunctions, electron_pair_list, electron_list);
    calculate_bx_t_fa(electron_pair_list, electron_list);
    calculate_bx_t_fb(electron_pair_list, electron_list);

    calculate_bx_t_fc(electron_pair_list, electron_list);
    /* Due to the symetry of the integrals in calculate_bx_t_fd, calculate_bx_t_fd always evalates to zero analatically.
     *if (!correlation_factor->f12_d_is_zero()) {
     *  traces.update_bx_fd_traces(wavefunctions, electron_list);
     *  calculate_bx_t_fd(electron_pair_list, electron_list);
     *}
     */
    calculate_bx_k(electron_pair_list, electron_list);
    normalize();

    auto direct_contribution = 
          direct_1_pair_0_one_ints[0] + direct_1_pair_0_one_ints[1]
        + direct_0_pair_2_one_ints[0] + direct_0_pair_2_one_ints[1]
        - 2.0*direct_1_pair_1_one_ints[0] - 2.0*direct_1_pair_1_one_ints[1] + 2.0*direct_1_pair_1_one_ints[2]
        - 2.0*direct_0_pair_3_one_ints[0] - 2.0*direct_0_pair_3_one_ints[1]
        + direct_1_pair_2_one_ints[0] + direct_1_pair_2_one_ints[1] - direct_1_pair_2_one_ints[2] - direct_1_pair_2_one_ints[3] - 2.0*direct_1_pair_2_one_ints[4] - 2.0*direct_1_pair_2_one_ints[5]
        + direct_0_pair_4_one_ints[0] - direct_0_pair_4_one_ints[1] + direct_0_pair_4_one_ints[2] - direct_0_pair_4_one_ints[3]
        + 2.0*direct_1_pair_3_one_ints[0];
    auto xchang_contrubtion = 
          xchang_1_pair_0_one_ints[0] + xchang_1_pair_0_one_ints[1]
        + xchang_0_pair_2_one_ints[0] + xchang_0_pair_2_one_ints[1]
        - 2.0*xchang_1_pair_1_one_ints[0] - 2.0*xchang_1_pair_1_one_ints[1] + 2.0*xchang_1_pair_1_one_ints[2]
        - 2.0*xchang_0_pair_3_one_ints[0] - 2.0*xchang_0_pair_3_one_ints[1]
        + xchang_1_pair_2_one_ints[0] + xchang_1_pair_2_one_ints[1] - xchang_1_pair_2_one_ints[2] - xchang_1_pair_2_one_ints[3] - 2.0*xchang_1_pair_2_one_ints[4] - 2.0*xchang_1_pair_2_one_ints[5]
        + xchang_0_pair_4_one_ints[0] - xchang_0_pair_4_one_ints[1] + xchang_0_pair_4_one_ints[2] - xchang_0_pair_4_one_ints[3]
        + 2.0*xchang_1_pair_3_one_ints[0];
    egf[band][0] += c3 * direct_contribution + c4 * xchang_contrubtion;
  }
}

void GF2_F12_VBX::zero() {
  direct_1_pair_0_one_ints.fill(0.0);
  direct_0_pair_2_one_ints.fill(0.0);
  direct_1_pair_1_one_ints.fill(0.0);
  direct_0_pair_3_one_ints.fill(0.0);
  direct_1_pair_2_one_ints.fill(0.0);
  direct_0_pair_4_one_ints.fill(0.0);
  direct_1_pair_3_one_ints.fill(0.0);

  xchang_1_pair_0_one_ints.fill(0.0);
  xchang_0_pair_2_one_ints.fill(0.0);
  xchang_1_pair_1_one_ints.fill(0.0);
  xchang_0_pair_3_one_ints.fill(0.0);
  xchang_1_pair_2_one_ints.fill(0.0);
  xchang_0_pair_4_one_ints.fill(0.0);
  xchang_1_pair_3_one_ints.fill(0.0);
}

void GF2_F12_VBX::normalize() {
  for (auto &it : direct_1_pair_0_one_ints) {
    it *= nsamp_pair;
  }
  for (auto &it : xchang_1_pair_0_one_ints) {
    it *= nsamp_pair;
  }
  for (auto &it : direct_0_pair_2_one_ints) {
    it *= nsamp_one_2;
  }
  for (auto &it : xchang_0_pair_2_one_ints) {
    it *= nsamp_one_2;
  }
  for (auto &it : direct_1_pair_1_one_ints) {
    it *= nsamp_pair * nsamp_one_1;
  }
  for (auto &it : xchang_1_pair_1_one_ints) {
    it *= nsamp_pair * nsamp_one_1;
  }
  for (auto &it : direct_0_pair_3_one_ints) {
    it *= nsamp_one_3;
  }
  for (auto &it : xchang_0_pair_3_one_ints) {
    it *= nsamp_one_3;
  }
  for (auto &it : direct_1_pair_2_one_ints) {
    it *= nsamp_pair * nsamp_one_2;
  }
  for (auto &it : xchang_1_pair_2_one_ints) {
    it *= nsamp_pair * nsamp_one_2;
  }
  for (auto &it : direct_0_pair_4_one_ints) {
    it *= nsamp_one_4;
  }
  for (auto &it : xchang_0_pair_4_one_ints) {
    it *= nsamp_one_4;
  }
  for (auto &it : direct_1_pair_3_one_ints) {
    it *= nsamp_pair * nsamp_one_3;
  }
  for (auto &it : xchang_1_pair_3_one_ints) {
    it *= nsamp_pair * nsamp_one_3;
  }
}

void GF2_F12_VBX::calculate_bx_t_fa(const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  for(int ip = 0; ip < electron_pair_list->size();ip++) {
    auto f_a = correlation_factor->f12p_a[ip];
    auto f_12 = correlation_factor->f12p[ip];
    direct_1_pair_0_one_ints[0] += f_12 * f_a * x_traces.x11[ip] * traces.p22[ip] * electron_pair_list->rv[ip];
    direct_1_pair_0_one_ints[0] += f_12 * f_a * traces.p11[ip] * x_traces.x22[ip] * electron_pair_list->rv[ip];
    xchang_1_pair_0_one_ints[0] += f_12 * f_a * x_traces.x12[ip] * traces.p12[ip] * electron_pair_list->rv[ip];
    xchang_1_pair_0_one_ints[0] += f_12 * f_a * traces.p12[ip] * x_traces.x12[ip] * electron_pair_list->rv[ip];
  }

  for(int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 4> t{0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      t[0] += correlation_factor->f23[ip][io] * x_traces.x13[ip][io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
      t[1] += correlation_factor->f23[ip][io] * traces.p13[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
          
      t[2] += correlation_factor->f23[ip][io] * x_traces.x23[ip][io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
      t[3] += correlation_factor->f23[ip][io] * traces.p23[ip * traces.electrons + io] * traces.k13[ip * traces.electrons + io] / electron_list->weight[io];
    }
    auto f_a = correlation_factor->f12p_a[ip];
    direct_1_pair_1_one_ints[0] += t[0] * f_a * traces.p22[ip] * electron_pair_list->rv[ip];
    direct_1_pair_1_one_ints[0] += t[1] * f_a * x_traces.x22[ip] * electron_pair_list->rv[ip];
                                          
    xchang_1_pair_1_one_ints[0] += t[2] * f_a * traces.p12[ip] * electron_pair_list->rv[ip];
    xchang_1_pair_1_one_ints[0] += t[3] * f_a * x_traces.x12[ip] * electron_pair_list->rv[ip];
  }

  for(int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 8> t{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      std::array<double, 8> s{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      for(int jo = 0; jo < electron_list->size();++jo) {
        if (jo != io) {
          s[0] += correlation_factor->f12o[io][jo] * x_traces.x23[ip][jo] * traces.k23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
          s[1] += correlation_factor->f12o[io][jo] * traces.p23[ip * traces.electrons + jo]   * traces.k23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
                                                                                  
          s[2] += correlation_factor->f12o[io][jo] * x_traces.x23[ip][jo] * traces.v23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
          s[3] += correlation_factor->f12o[io][jo] * traces.p23[ip * traces.electrons + jo]   * traces.v23[ip * traces.electrons + jo] / (electron_list->weight[jo]);

          s[4] += correlation_factor->f12o[io][jo] * x_traces.x13[ip][jo] * traces.k23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
          s[5] += correlation_factor->f12o[io][jo] * traces.p13[ip * traces.electrons + jo]   * traces.k23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
             
          s[6] += correlation_factor->f12o[io][jo] * x_traces.x13[ip][jo] * traces.v23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
          s[7] += correlation_factor->f12o[io][jo] * traces.p13[ip * traces.electrons + jo]   * traces.v23[ip * traces.electrons + jo] / (electron_list->weight[jo]);
        }
      }
      t[0] += s[0] * traces.p13[ip * traces.electrons + io]   * traces.k13[ip * traces.electrons + io] / (electron_list->weight[io]);
      t[1] += s[1] * x_traces.x13[ip][io] * traces.k13[ip * traces.electrons + io] / (electron_list->weight[io]);
                                                              
      t[2] += s[2] * traces.p13[ip * traces.electrons + io]   * traces.v13[ip * traces.electrons + io] / (electron_list->weight[io]);
      t[3] += s[3] * x_traces.x13[ip][io] * traces.v13[ip * traces.electrons + io] / (electron_list->weight[io]);
                                                              
      t[4] += s[4] * traces.p23[ip * traces.electrons + io]   * traces.k13[ip * traces.electrons + io] / (electron_list->weight[io]);
      t[5] += s[5] * x_traces.x23[ip][io] * traces.k13[ip * traces.electrons + io] / (electron_list->weight[io]);
                                                                              
      t[6] += s[6] * traces.p23[ip * traces.electrons + io]   * traces.v13[ip * traces.electrons + io] / (electron_list->weight[io]);
      t[7] += s[7] * x_traces.x23[ip][io] * traces.v13[ip * traces.electrons + io] / (electron_list->weight[io]);
    }
    auto f_a = correlation_factor->f12p_a[ip];
    direct_1_pair_2_one_ints[0] += t[0] * f_a * electron_pair_list->rv[ip];
    direct_1_pair_2_one_ints[0] += t[1] * f_a * electron_pair_list->rv[ip];
                                                                      
    direct_1_pair_2_one_ints[2] += t[2] * f_a * electron_pair_list->rv[ip];
    direct_1_pair_2_one_ints[2] += t[3] * f_a * electron_pair_list->rv[ip];
                                                                      
    xchang_1_pair_2_one_ints[0] += t[4] * f_a * electron_pair_list->rv[ip];
    xchang_1_pair_2_one_ints[0] += t[5] * f_a * electron_pair_list->rv[ip];
                                                                          
    xchang_1_pair_2_one_ints[2] += t[6] * f_a * electron_pair_list->rv[ip];
    xchang_1_pair_2_one_ints[2] += t[7] * f_a * electron_pair_list->rv[ip];
  }
}

/*
double calculate_bx_t_fb_4e(
    std::vector<std::vector<double>>& T_jo_lo_1, std::vector<std::vector<double>>& T_jo_lo_2,
    std::vector<std::vector<double>>& T_ko_lo_1,
    std::vector<double>& weight,
) {
  std::vector<std::vector<double>> T_jo_lo(weight.size(), std::vector<double>(weight.size(), 0.0));
  for (int jo = 0; jo < weight.size(); ++jo) {
    for (int lo = 0; lo < weight.size(); ++lo) {
      T_jo_lo[jo][lo] = traces.op12[jo * traces.electrons + lo] * traces.ok12[jo * traces.electrons + lo];
    }
    T_jo_lo[jo][jo] = 0.0;
  }

  std::vector<std::vector<double>> T_ko_lo(weight.size(), std::vector<double>(weight.size(), 0.0));
  for (int ko = 0; ko < weight.size(); ++ko) {
    for (int lo = 0; lo < weight.size(); ++lo) {
      T_ko_lo[ko][lo] = correlation_factor->f12o[ko][lo];
    }
    T_ko_lo[ko][ko] = 0.0;
  }
  
  std::vector<std::vector<double>> T_jo_ko(weight.size(), std::vector<double>(weight.size(), 0.0));
  for (int jo = 0; jo < weight.size(); ++jo) {
    for (int ko = 0; ko < weight.size(); ++ko) {
      for (int lo = 0; lo < weight.size(); ++lo) {
        T_jo_ko[jo][ko] += T_jo_lo[jo][lo] * T_ko_lo[ko][lo] / weight[lo];
      }
    }
    T_jo_ko[jo][jo] = 0.0;
  }

  double en = 0.0;
  for (int io = 0; io < weight.size(); ++io) {
    double t_io = 0.0;
    for (int ko = 0; ko < weight.size(); ++ko) {
      if (ko != io) {
        double t_ko = 0.0;
        for (int jo = 0; jo < weight.size(); ++jo) {
          if (jo != io) {
            t_ko += correlation_factor->f12o_b[io][jo] * T_jo_ko[jo][ko] / weight[jo];
          }
        }
        t_io += t_ko * x_traces.ox12[io][ko] * traces.ok12[io * traces.electrons + ko] / weight[ko];
      }
    }
    en += t_io / weight[io];
  }

  for (int io = 0; io < weight.size(); ++io) {
    double t_io = 0.0;
    for (int jo = 0; jo < weight.size(); ++jo) {
      if (jo != io) {
        double t_jo = 0.0;
        for (int ko = 0; ko < weight.size(); ++ko) {
          if (ko != jo && ko != io) {
            t_jo += correlation_factor->f12o[ko][io] * x_traces.ox12[io][ko] * traces.ok12[io * traces.electrons + ko] / weight[ko];
          }
        }
        auto f_b = correlation_factor->f12o_b[io][jo];
        t_io += t_jo * f_b * traces.op12[jo * traces.electrons + io] * traces.ok12[jo * traces.electrons + io] / weight[jo] / (weight[io]);
      }
    }
    en -= t_io / weight[io] / (weight[io]);
  }
  return en;
}
*/

void GF2_F12_VBX::calculate_bx_t_fb(const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 4> t_io{0.0, 0.0, 0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        auto f_b = correlation_factor->f12o_b[io][jo];
        t_io[0] += f_b * correlation_factor->f12o[io][jo] * x_traces.ox11[jo] / electron_list->weight[jo];
        t_io[1] += f_b * correlation_factor->f12o[io][jo] * traces.op11[jo]   / electron_list->weight[jo];
        t_io[2] += f_b * correlation_factor->f12o[io][jo] * x_traces.ox12[io][jo] * traces.op12[io * traces.electrons + jo] / electron_list->weight[jo];
        t_io[3] += f_b * correlation_factor->f12o[io][jo] * traces.op12[io * traces.electrons + jo] * x_traces.ox12[io][jo] / electron_list->weight[jo];
      }
    }
    direct_0_pair_2_one_ints[0] += t_io[0] * traces.op11[io]   / electron_list->weight[io];
    direct_0_pair_2_one_ints[0] += t_io[1] * x_traces.ox11[io] / electron_list->weight[io];
    xchang_0_pair_2_one_ints[0] += t_io[2] / electron_list->weight[io];
    xchang_0_pair_2_one_ints[0] += t_io[3] / electron_list->weight[io];
  }

  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 4> t_io{0.0, 0.0, 0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        std::array<double, 4> t_jo{0.0, 0.0, 0.0, 0.0};
        for (int ko = 0; ko < electron_list->size(); ++ko) {
          if (ko != jo && ko != io) {
          t_jo[0] += correlation_factor->f12o[jo][ko] * x_traces.ox12[io][ko] * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
          t_jo[1] += correlation_factor->f12o[jo][ko] * traces.op12[io * traces.electrons + ko]   * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
          t_jo[2] += correlation_factor->f12o[jo][ko] * x_traces.ox12[jo][ko] * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
          t_jo[3] += correlation_factor->f12o[jo][ko] * traces.op12[jo * traces.electrons + ko]   * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
          }
        }
        auto f_b = correlation_factor->f12o_b[io][jo];
        t_io[0] += f_b * t_jo[0] * traces.op11[jo]       / electron_list->weight[jo];
        t_io[1] += f_b * t_jo[1] * x_traces.ox11[jo]     / electron_list->weight[jo];
        t_io[2] += f_b * t_jo[2] * traces.op12[io * traces.electrons + jo]   / electron_list->weight[jo];
        t_io[3] += f_b * t_jo[3] * x_traces.ox12[io][jo] / electron_list->weight[jo];
      }
    }
    direct_0_pair_3_one_ints[0] += t_io[0] / electron_list->weight[io];
    direct_0_pair_3_one_ints[0] += t_io[1] / electron_list->weight[io];
    xchang_0_pair_3_one_ints[0] += t_io[2] / electron_list->weight[io];
    xchang_0_pair_3_one_ints[0] += t_io[3] / electron_list->weight[io];
  }

  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 4> t_io{0.0, 0.0, 0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        std::array<double, 8> t_jo{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        for (int ko = 0; ko < electron_list->size(); ++ko) {
          if (ko != jo && ko != io) {
            std::array<double, 8> t_ko{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            for (int lo = 0; lo < electron_list->size(); ++lo) {
              if (lo != ko && lo != jo && lo != io) {
  t_ko[0] += correlation_factor->f12o[ko][lo] * traces.op12[jo * traces.electrons + lo]   * traces.ok12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
  t_ko[1] += correlation_factor->f12o[ko][lo] * x_traces.ox12[jo][lo] * traces.ok12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
 
  t_ko[2] += correlation_factor->f12o[ko][lo] * traces.op12[io * traces.electrons + lo]   * traces.ok12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
  t_ko[3] += correlation_factor->f12o[ko][lo] * x_traces.ox12[io][lo] * traces.ok12[jo * traces.electrons + lo] / (electron_list->weight[lo]);

  t_ko[4] += correlation_factor->f12o[ko][lo] * traces.op12[jo * traces.electrons + lo]   * traces.ov12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
  t_ko[5] += correlation_factor->f12o[ko][lo] * x_traces.ox12[jo][lo] * traces.ov12[jo * traces.electrons + lo] / (electron_list->weight[lo]);

  t_ko[6] += correlation_factor->f12o[ko][lo] * traces.op12[io * traces.electrons + lo]   * traces.ov12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
  t_ko[7] += correlation_factor->f12o[ko][lo] * x_traces.ox12[io][lo] * traces.ov12[jo * traces.electrons + lo] / (electron_list->weight[lo]);
              }
            }
  auto f_b = correlation_factor->f12o_b[io][jo];
  t_jo[0] += t_ko[0] * x_traces.ox12[io][ko] * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
  t_jo[1] += t_ko[1] * traces.op12[io * traces.electrons + ko]   * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
         
  t_jo[2] += t_ko[2] * x_traces.ox12[jo][ko] * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
  t_jo[3] += t_ko[3] * traces.op12[jo * traces.electrons + ko]   * traces.ok12[io * traces.electrons + ko] / electron_list->weight[ko];
         
  t_jo[4] += t_ko[4] * x_traces.ox12[io][ko] * traces.ov12[io * traces.electrons + ko] / electron_list->weight[ko];
  t_jo[5] += t_ko[5] * traces.op12[io * traces.electrons + ko]   * traces.ov12[io * traces.electrons + ko] / electron_list->weight[ko];
         
  t_jo[6] += t_ko[6] * x_traces.ox12[jo][ko] * traces.ov12[io * traces.electrons + ko] / electron_list->weight[ko];
  t_jo[7] += t_ko[7] * traces.op12[jo * traces.electrons + ko]   * traces.ov12[io * traces.electrons + ko] / electron_list->weight[ko];
          }
        }
  auto f_b = correlation_factor->f12o_b[io][jo];
  direct_0_pair_4_one_ints[0] += t_jo[0] * f_b / electron_list->weight[io] / electron_list->weight[jo];
  direct_0_pair_4_one_ints[0] += t_jo[1] * f_b / electron_list->weight[io] / electron_list->weight[jo];

  xchang_0_pair_4_one_ints[0] += t_jo[2] * f_b / electron_list->weight[io] / electron_list->weight[jo];
  xchang_0_pair_4_one_ints[0] += t_jo[3] * f_b / electron_list->weight[io] / electron_list->weight[jo];

  direct_0_pair_4_one_ints[1] += t_jo[4] * f_b / electron_list->weight[io] / electron_list->weight[jo];
  direct_0_pair_4_one_ints[1] += t_jo[5] * f_b / electron_list->weight[io] / electron_list->weight[jo];

  xchang_0_pair_4_one_ints[1] += t_jo[6] * f_b / electron_list->weight[io] / electron_list->weight[jo];
  xchang_0_pair_4_one_ints[1] += t_jo[7] * f_b / electron_list->weight[io] / electron_list->weight[jo];
      }
    }
  }
}

void GF2_F12_VBX::calculate_bx_t_fc(const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  for(int ip = 0; ip < electron_pair_list->size();ip++) {
    auto f_12 = correlation_factor->f12p[ip];
    auto f_c = correlation_factor->f12p_c[ip];
    direct_1_pair_0_one_ints[1] += f_12 * f_c * (x_traces.x11[ip] * traces.dp22[ip] - x_traces.dx11[ip] * traces.p22[ip]) * electron_pair_list->rv[ip];
    direct_1_pair_0_one_ints[1] += f_12 * f_c * (traces.p11[ip] * x_traces.dx22[ip] - traces.dp11[ip] * x_traces.x22[ip]) * electron_pair_list->rv[ip];
    xchang_1_pair_0_one_ints[1] += f_12 * f_c * (x_traces.dx12[ip] * traces.p12[ip] - x_traces.x12[ip] * traces.dp21[ip]) * electron_pair_list->rv[ip];
    xchang_1_pair_0_one_ints[1] += f_12 * f_c * (traces.dp12[ip] * x_traces.x12[ip] - traces.p12[ip] * x_traces.dx21[ip]) * electron_pair_list->rv[ip];
  }

  for(int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 4> t{0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      auto cf = correlation_factor->f23[ip][io] * correlation_factor->f12p_c[ip];
      auto wgt = electron_pair_list->rv[ip] / electron_list->weight[io];
      direct_1_pair_1_one_ints[1] += cf * (x_traces.x13[ip][io]  * traces.dp22[ip] - x_traces.dx31[ip][io] * traces.p22[ip] ) * traces.k13[ip * traces.electrons + io] * wgt;
      direct_1_pair_1_one_ints[1] += cf * (traces.p13[ip * traces.electrons + io]  * x_traces.dx22[ip] - traces.dp31[ip][io] * x_traces.x22[ip] ) * traces.k13[ip * traces.electrons + io] * wgt;
      xchang_1_pair_1_one_ints[1] += cf * (x_traces.dx32[ip][io] * traces.p12[ip]  - x_traces.x23[ip][io]  * traces.dp21[ip]) * traces.k13[ip * traces.electrons + io] * wgt;
      xchang_1_pair_1_one_ints[1] += cf * (traces.dp32[ip][io] * x_traces.x12[ip]  - traces.p23[ip * traces.electrons + io]  * x_traces.dx21[ip]) * traces.k13[ip * traces.electrons + io] * wgt;
    }
  }

  for (int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 4> t{0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      std::array<double, 8> s{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      for (int jo = 0; jo < electron_list->size(); ++jo) {
        if (jo != io) {
          auto cf = correlation_factor->f12p_c[ip] * correlation_factor->f12o[io][jo];
          auto wgt = electron_pair_list->rv[ip] / electron_list->weight[io] / (electron_list->weight[jo]);
direct_1_pair_2_one_ints[1] += cf * (x_traces.dx32[ip][jo] *  traces.p13[ip * traces.electrons + io] -  x_traces.x23[ip][jo] * traces.dp31[ip][io]) * traces.k13[ip * traces.electrons + io] * traces.k23[ip * traces.electrons + jo] * wgt;
direct_1_pair_2_one_ints[1] += cf * (traces.dp32[ip][jo] *  x_traces.x13[ip][io] -  traces.p23[ip * traces.electrons + jo] * x_traces.dx31[ip][io]) * traces.k13[ip * traces.electrons + io] * traces.k23[ip * traces.electrons + jo] * wgt;

direct_1_pair_2_one_ints[3] += cf * (x_traces.dx32[ip][jo] *  traces.p13[ip * traces.electrons + io] -  x_traces.x23[ip][jo] * traces.dp31[ip][io]) * traces.v13[ip * traces.electrons + io] * traces.v23[ip * traces.electrons + jo] * wgt;
direct_1_pair_2_one_ints[3] += cf * (traces.dp32[ip][jo] *  x_traces.x13[ip][io] -  traces.p23[ip * traces.electrons + jo] * x_traces.dx31[ip][io]) * traces.v13[ip * traces.electrons + io] * traces.v23[ip * traces.electrons + jo] * wgt;

xchang_1_pair_2_one_ints[1] += cf * ( x_traces.x13[ip][jo] * traces.dp32[ip][io] - x_traces.dx31[ip][jo] *  traces.p23[ip * traces.electrons + io]) * traces.k13[ip * traces.electrons + io] * traces.k23[ip * traces.electrons + jo] * wgt;
xchang_1_pair_2_one_ints[1] += cf * ( traces.p13[ip * traces.electrons + jo] * x_traces.dx32[ip][io] - traces.dp31[ip][jo] *  x_traces.x23[ip][io]) * traces.k13[ip * traces.electrons + io] * traces.k23[ip * traces.electrons + jo] * wgt;

xchang_1_pair_2_one_ints[3] += cf * ( x_traces.x13[ip][jo] * traces.dp32[ip][io] - x_traces.dx31[ip][jo] *  traces.p23[ip * traces.electrons + io]) * traces.v13[ip * traces.electrons + io] * traces.v23[ip * traces.electrons + jo] * wgt;
xchang_1_pair_2_one_ints[3] += cf * ( traces.p13[ip * traces.electrons + jo] * x_traces.dx32[ip][io] - traces.dp31[ip][jo] *  x_traces.x23[ip][io]) * traces.v13[ip * traces.electrons + io] * traces.v23[ip * traces.electrons + jo] * wgt;
        }
      }
    }
  }
}

void GF2_F12_VBX::calculate_bx_t_fd(const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 2> t_io{0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        auto f_d = correlation_factor->f12o_d[io][jo] * correlation_factor->f12o[io][jo];
        auto wgt = 1.0 / electron_list->weight[io] / (electron_list->weight[jo]);
        direct_0_pair_2_one_ints[1] += f_d * (x_traces.ox11[io] * traces.ds_p22[io][jo] - x_traces.ds_x11[io][jo] * traces.op11[jo]) * wgt;
        direct_0_pair_2_one_ints[1] += f_d * (traces.op11[io] * x_traces.ds_x22[io][jo] - traces.ds_p11[io][jo] * x_traces.ox11[jo]) * wgt;
        xchang_0_pair_2_one_ints[1] += f_d * (x_traces.ox12[io][jo] * traces.ds_p12[io][jo] - x_traces.ox12[io][jo] * traces.ds_p21[io][jo]) * wgt;
        xchang_0_pair_2_one_ints[1] += f_d * (traces.op12[io * traces.electrons + jo] * x_traces.ds_x12[io][jo] - traces.op12[io * traces.electrons + jo] * x_traces.ds_x21[io][jo]) * wgt;
      }
    }
  }

  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 4> t_io{0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        std::array<double, 4> t_jo{0.0, 0.0, 0.0, 0.0};
        for (int ko = 0; ko < electron_list->size(); ++ko) {
          if (ko != jo && ko != io) {
            auto wgt = 1.0 / electron_list->weight[io]/ electron_list->weight[jo] / electron_list->weight[ko];
            auto cf = correlation_factor->f12o[jo][ko] * correlation_factor->f12o_d[io][jo];
      direct_0_pair_3_one_ints[1] += cf * traces.ok12[io * traces.electrons + ko] * (x_traces.ox12[io][ko] * traces.ds_p22[io][jo]   - x_traces.ds_x31[io][jo][ko] * traces.op11[jo]      ) * wgt;
      direct_0_pair_3_one_ints[1] += cf * traces.ok12[io * traces.electrons + ko] * (traces.op12[io * traces.electrons + ko] * x_traces.ds_x22[io][jo]   - traces.ds_p31[io][jo][ko] * x_traces.ox11[jo]      ) * wgt;

      xchang_0_pair_3_one_ints[1] += cf * traces.ok12[io * traces.electrons + ko] * (x_traces.ds_x32[io][jo][ko] * traces.op12[io * traces.electrons + jo]     -  x_traces.ox12[jo][ko]      * traces.ds_p21[io][jo]) * wgt;
      xchang_0_pair_3_one_ints[1] += cf * traces.ok12[io * traces.electrons + ko] * (traces.ds_p32[io][jo][ko] * x_traces.ox12[io][jo]     -  traces.op12[jo * traces.electrons + ko]      * x_traces.ds_x21[io][jo]) * wgt;
          }
        }
      }
    }
  }

  for (int io = 0; io < electron_list->size(); ++io) {
    std::array<double, 4> t_io{0.0, 0.0, 0.0, 0.0};
    for (int jo = 0; jo < electron_list->size(); ++jo) {
      if (jo != io) {
        std::array<double, 4> t_jo{0.0, 0.0, 0.0, 0.0};
        for (int ko = 0; ko < electron_list->size(); ++ko) {
          if (ko != jo && ko != io) {
            std::array<double, 8> t_ko{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            for (int lo = 0; lo < electron_list->size(); ++lo) {
              if (lo != ko && lo != jo && lo != io) {
                auto cf = correlation_factor->f12o[ko][lo] * correlation_factor->f12o_d[io][jo]; 
                auto wgt = 1.0 / electron_list->weight[io]/ electron_list->weight[jo] / electron_list->weight[ko] / electron_list->weight[lo];
   direct_0_pair_4_one_ints[2] += cf * (x_traces.ds_x32[io][jo][lo] * traces.op12[io * traces.electrons + ko] - x_traces.ox12[jo][lo] * traces.ds_p31[io][jo][ko]) * traces.ok12[io * traces.electrons + ko] * traces.ok12[jo * traces.electrons + lo] * wgt;
   direct_0_pair_4_one_ints[2] += cf * (traces.ds_p32[io][jo][lo] * x_traces.ox12[io][ko] - traces.op12[jo * traces.electrons + lo] * x_traces.ds_x31[io][jo][ko]) * traces.ok12[io * traces.electrons + ko] * traces.ok12[jo * traces.electrons + lo] * wgt;
   direct_0_pair_4_one_ints[3] += cf * (x_traces.ds_x32[io][jo][lo] * traces.op12[io * traces.electrons + ko] - x_traces.ox12[jo][lo] * traces.ds_p31[io][jo][ko]) * traces.ov12[io * traces.electrons + ko] * traces.ov12[jo * traces.electrons + lo] * wgt;
   direct_0_pair_4_one_ints[3] += cf * (traces.ds_p32[io][jo][lo] * x_traces.ox12[io][ko] - traces.op12[jo * traces.electrons + lo] * x_traces.ds_x31[io][jo][ko]) * traces.ov12[io * traces.electrons + ko] * traces.ov12[jo * traces.electrons + lo] * wgt;
   xchang_0_pair_4_one_ints[2] += cf * (x_traces.ox12[io][lo] * traces.ds_p32[io][jo][ko] - x_traces.ds_x31[io][jo][lo] * traces.op12[jo * traces.electrons + ko]) * traces.ok12[io * traces.electrons + ko] * traces.ok12[jo * traces.electrons + lo] * wgt;
   xchang_0_pair_4_one_ints[2] += cf * (traces.op12[io * traces.electrons + lo] * x_traces.ds_x32[io][jo][ko] - traces.ds_p31[io][jo][lo] * x_traces.ox12[jo][ko]) * traces.ok12[io * traces.electrons + ko] * traces.ok12[jo * traces.electrons + lo] * wgt;
   xchang_0_pair_4_one_ints[3] += cf * (x_traces.ox12[io][lo] * traces.ds_p32[io][jo][ko] - x_traces.ds_x31[io][jo][lo] * traces.op12[jo * traces.electrons + ko]) * traces.ov12[io * traces.electrons + ko] * traces.ov12[jo * traces.electrons + lo] * wgt;
   xchang_0_pair_4_one_ints[3] += cf * (traces.op12[io * traces.electrons + lo] * x_traces.ds_x32[io][jo][ko] - traces.ds_p31[io][jo][lo] * x_traces.ox12[jo][ko]) * traces.ov12[io * traces.electrons + ko] * traces.ov12[jo * traces.electrons + lo] * wgt;
              }
            }
          }
        }
      }
    }
  }
}

void GF2_F12_VBX::calculate_bx_k(const Electron_Pair_List* electron_pair_list, const Electron_List* electron_list) {
  for (int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 2> t{0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
     direct_1_pair_1_one_ints[2] += correlation_factor->f23[ip][io] * (correlation_factor->f23[ip][io] - correlation_factor->f13[ip][io]) *x_traces.x12[ip] * traces.op11[io] * traces.k12[ip] * electron_pair_list->rv[ip] / electron_list->weight[io];
     direct_1_pair_1_one_ints[2] += correlation_factor->f23[ip][io] * (correlation_factor->f23[ip][io] - correlation_factor->f13[ip][io]) * traces.p12[ip] * x_traces.ox11[io] * traces.k12[ip] * electron_pair_list->rv[ip] / electron_list->weight[io];
 
     xchang_1_pair_1_one_ints[2] += correlation_factor->f23[ip][io] * (correlation_factor->f23[ip][io] - correlation_factor->f13[ip][io]) * x_traces.x13[ip][io] * traces.p23[ip * traces.electrons + io] * traces.k12[ip] * electron_pair_list->rv[ip] / electron_list->weight[io];
     xchang_1_pair_1_one_ints[2] += correlation_factor->f23[ip][io] * (correlation_factor->f23[ip][io] - correlation_factor->f13[ip][io]) * traces.p13[ip * traces.electrons + io] * x_traces.x23[ip][io] * traces.k12[ip] * electron_pair_list->rv[ip] / electron_list->weight[io];
    }
  }

  for (int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 4> t{0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      std::array<double, 4> s{0.0, 0.0, 0.0, 0.0};
      for (int jo = 0; jo < electron_list->size(); ++jo) {
        if (jo != io) {
          auto f34o = correlation_factor->f12o[io][jo];

          auto f14p = correlation_factor->f13[ip][jo];
          auto f24p = correlation_factor->f23[ip][jo];

          auto f23 = correlation_factor->f23[ip][io];
          auto f13 = correlation_factor->f13[ip][io];
          auto wgt = electron_pair_list->rv[ip] / electron_list->weight[io] / electron_list->weight[jo];

direct_1_pair_2_one_ints[4] += f34o * (f14p - f24p) * x_traces.ox11[jo]     * traces.p23[ip * traces.electrons + io]  * traces.k13[ip * traces.electrons + io]  * traces.k12[ip] * wgt;;
direct_1_pair_2_one_ints[4] += f34o * (f14p - f24p) * traces.op11[jo]     * x_traces.x23[ip][io]  * traces.k13[ip * traces.electrons + io]  * traces.k12[ip] * wgt;;

xchang_1_pair_2_one_ints[4] += f34o * (f14p - f24p) * x_traces.ox12[io][jo] * traces.p23[ip * traces.electrons + jo]  * traces.k13[ip * traces.electrons + io]  * traces.k12[ip] * wgt;
xchang_1_pair_2_one_ints[4] += f34o * (f14p - f24p) * traces.op12[io * traces.electrons + jo] * x_traces.x23[ip][jo]  * traces.k13[ip * traces.electrons + io]  * traces.k12[ip] * wgt;

direct_1_pair_2_one_ints[5] += f24p * (f23 - f13) * x_traces.ox12[io][jo] * traces.p12[ip]      * traces.ok12[io * traces.electrons + jo] * traces.k12[ip] * wgt;
direct_1_pair_2_one_ints[5] += f24p * (f23 - f13) * traces.op12[io * traces.electrons + jo] * x_traces.x12[ip]      * traces.ok12[io * traces.electrons + jo] * traces.k12[ip] * wgt;

xchang_1_pair_2_one_ints[5] += f24p * (f23 - f13) * x_traces.x13[ip][jo]  * traces.p23[ip * traces.electrons + io]  * traces.ok12[io * traces.electrons + jo] * traces.k12[ip] * wgt;
xchang_1_pair_2_one_ints[5] += f24p * (f23 - f13) * traces.p13[ip * traces.electrons + jo]  * x_traces.x23[ip][io]  * traces.ok12[io * traces.electrons + jo] * traces.k12[ip] * wgt;
        }
      }
    }
  }

  for (int ip = 0; ip < electron_pair_list->size();ip++) {
    std::array<double, 4> t_i{0.0, 0.0, 0.0, 0.0};
    for (int io = 0; io < electron_list->size(); ++io) {
      std::array<double, 4> t_j{0.0, 0.0, 0.0, 0.0};
      for (int jo = 0; jo < electron_list->size(); ++jo) {
        if (jo != io) {
          std::array<double, 4> t_k{0.0, 0.0, 0.0, 0.0};
          for (int ko = 0; ko < electron_list->size(); ++ko) {
            if (ko != jo && ko != io) {
              auto cf = correlation_factor->f12o[io][ko] * (correlation_factor->f13[ip][jo] - correlation_factor->f23[ip][jo]);
              auto wgt = electron_pair_list->rv[ip] / electron_list->weight[io] / electron_list->weight[jo] / electron_list->weight[ko];
              direct_1_pair_3_one_ints[0] += cf * x_traces.x23[ip][io] * traces.op12[jo * traces.electrons + ko] * traces.k13[ip * traces.electrons + io] * traces.ok12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              direct_1_pair_3_one_ints[0] += cf * traces.p23[ip * traces.electrons + io] * x_traces.ox12[jo][ko] * traces.k13[ip * traces.electrons + io] * traces.ok12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              direct_1_pair_3_one_ints[0] -= cf * x_traces.x23[ip][io] * traces.op12[jo * traces.electrons + ko] * traces.v13[ip * traces.electrons + io] * traces.ov12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              direct_1_pair_3_one_ints[0] -= cf * traces.p23[ip * traces.electrons + io] * x_traces.ox12[jo][ko] * traces.v13[ip * traces.electrons + io] * traces.ov12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              xchang_1_pair_3_one_ints[0] += cf * x_traces.x23[ip][ko] * traces.op12[io * traces.electrons + jo] * traces.k13[ip * traces.electrons + io] * traces.ok12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              xchang_1_pair_3_one_ints[0] += cf * traces.p23[ip * traces.electrons + ko] * x_traces.ox12[io][jo] * traces.k13[ip * traces.electrons + io] * traces.ok12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              xchang_1_pair_3_one_ints[0] -= cf * x_traces.x23[ip][ko] * traces.op12[io * traces.electrons + jo] * traces.v13[ip * traces.electrons + io] * traces.ov12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
              xchang_1_pair_3_one_ints[0] -= cf * traces.p23[ip * traces.electrons + ko] * x_traces.ox12[io][jo] * traces.v13[ip * traces.electrons + io] * traces.ov12[jo * traces.electrons + ko] * traces.k12[ip] * wgt;
            }
          }
        }
      }
    }
  }
}
