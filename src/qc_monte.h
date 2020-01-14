// Copyright 2017
#ifndef QC_MONTE_H_
#define QC_MONTE_H_

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

#include "el_pair.h"
#include "weight_function.h"
#include "basis/qc_basis.h"
#include "qc_geom.h"
#include "qc_input.h"
#include "qc_mpi.h"
#include "qc_ovps.h"
#include "qc_random.h"
#include "tau_integrals.h"
#include "control_variate.h"

class GFStats {
 private:
  std::vector<std::vector<double>> qepsEx1, qepsEx2, qepsAvg, qepsVar;
  std::vector<std::ofstream*> output_streams;
  bool isMaster;
  double tasks;

 public:
  std::vector<std::vector<double>> qeps;

  GFStats(bool, int, int, int, int, const std::string&, int);
  GFStats(const GFStats& gf) { throw std::runtime_error("Tried to copy GFStats"); }
  GFStats operator=(const GFStats& gf) { exit(0); }

  ~GFStats();

  void blockIt(const int&);
  void reduce();
  void print(const int& step, const double& time_span);
};

class QC_monte {
 public:
  QC_monte(MPI_info p0, IOPs p1, Molec p2, Basis p3, GTO_Weight p4, Electron_Pair_List*);
  virtual ~QC_monte() {
    basis.gpu_free();
    delete tau;
  }
  virtual void monte_energy() = 0;

 protected:
  MPI_info mpi_info;
  IOPs iops;
  Molec molec;
  Basis basis;
  GTO_Weight mc_basis;

  Random random;
  Electron_Pair_List* el_pair_list;
  OVPs ovps;
  Tau* tau;

  int nDeriv;
  int numBand, offBand;
  int iocc1, iocc2, ivir1, ivir2;


  void update_wavefunction();
  void move_walkers();
  static void print_mc_head(std::chrono::high_resolution_clock::time_point);
  static void print_mc_tail(double, std::chrono::high_resolution_clock::time_point);
};

class MP : public QC_monte {
 public:
  void monte_energy();
  virtual void energy() = 0;
 protected:
  MP(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep, std::vector<int> cv_sizes) :
  QC_monte(p1, p2, p3, p4, p5, ep) {
    tau->resize(cv_sizes.size());
    emp.resize(cv_sizes.size());

    if (ep->requires_blocking()) {
      for (int n : cv_sizes) {
        control.emplace_back(std::vector<double>(n));
        cv.push_back(new BlockingAccumulator(n, std::vector<double>(n, 0.0)));
      }
      cv_sizes.push_back(std::accumulate(cv_sizes.begin(), cv_sizes.end(), 0));
      control.emplace_back(std::vector<double>(cv_sizes.back()));
      cv.push_back(new BlockingAccumulator(cv_sizes.back(), std::vector<double>(cv_sizes.back(), 0.0)));
    } else {
      for (int n : cv_sizes) {
        control.emplace_back(std::vector<double>(n));
        cv.push_back(new ControlVariate(n, std::vector<double>(n, 0.0)));
      }
      cv_sizes.push_back(std::accumulate(cv_sizes.begin(), cv_sizes.end(), 0));
      control.emplace_back(std::vector<double>(cv_sizes.back()));
      cv.push_back(new ControlVariate(cv_sizes.back(), std::vector<double>(cv_sizes.back(), 0.0)));
    }
  }
  ~MP() {
    for (auto &item : cv) {
      delete item;
    }
  }


  std::vector<double> emp;
  std::vector<std::vector<double>> control;
  std::vector<Accumulator*> cv;
  void zero_energies();

  void mcmp2_energy_fast(double&, std::vector<double>&);
  void mcmp2_energy(double&, std::vector<double>&);
  void mcmp3_energy(double&, std::vector<double>&);
  void mcmp4_energy(double&, std::vector<double>&);
  void mcmp4_energy_ij(double&, std::vector<double>&);
  void mcmp4_energy_ik(double&, std::vector<double>&);
  void mcmp4_energy_il(double&, std::vector<double>&);
  void mcmp4_energy_ijkl(double&, std::vector<double>&);
  void mcmp4_energy_ijkl_fast(double&, std::vector<double>&);
};

class MP2 : public MP {
 public:
  MP2(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : MP(p1, p2, p3, p4, p5, ep, {6}) {}
  ~MP2() override = default;

 protected:
  void energy() override;
};

class MP3 : public MP {
 public:
  MP3(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : MP(p1, p2, p3, p4, p5, ep, {6, 36}) {
    ovps.init(cv.size() - 1, iops.iopns[KEYS::MC_NPAIR], basis);
  }
  ~MP3() override {
    ovps.free();
  }

 protected:
  void energy() override;
};

class MP4 : public MP {
 public:
  MP4(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : MP(p1, p2, p3, p4, p5, ep, {6, 36, 72}) {
    ovps.init(cv.size() - 1, iops.iopns[KEYS::MC_NPAIR], basis);
  }
  ~MP4() {
    ovps.free();
  }

 protected:
  void energy() override;
};

class GF : public  QC_monte {
 public:
  void monte_energy() override;

 protected:
  GF(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : QC_monte(p1, p2, p3, p4, p5, ep) {}

  std::vector<GFStats> qeps;
  virtual void mc_local_energy(const int& step) = 0;
  virtual int full_print(int& step, int checkNum) = 0;
  std::string genFileName(int, int, int, int, int, int);

  void mcgf2_local_energy_core();
  void mcgf2_local_energy(std::vector<std::vector<double>>&);
  void mcgf2_local_energy_diff(std::vector<std::vector<double>>&);
  void mcgf2_local_energy_full(int);
  void mcgf2_local_energy_full_diff(int);

  void mcgf3_local_energy_core();
  void mcgf3_local_energy(std::vector<std::vector<double>>&);
  void mcgf3_local_energy_diff(std::vector<std::vector<double>>&);
  void mcgf3_local_energy_full(int);
  void mcgf3_local_energy_full_diff(int);

  void mc_gf_statistics(int,
                        std::vector<std::vector<double*>>&,
                        std::vector<double*>&,
                        std::vector<std::vector<double*>>&);
  void mc_gf_full_diffs(int band, std::vector<double> m);

  void mc_gf_copy(std::vector<double>&, double*);

  void mc_gf_full_print(int band, int steps, int checkNum, int order,
      std::vector<double*>& d_ex1,
      std::vector<std::vector<double*>>& d_cov);
};

class GPU_GF2 : public GF {
 protected:
  void mc_local_energy(std::vector<std::vector<double>>&, int);

 public:
  GPU_GF2(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : GF(p1, p2, p3, p4, p5, ep) {
    ovps.init_02(iops.iopns[KEYS::MC_NPAIR], iops.iopns[KEYS::NUM_BAND],
                 iops.iopns[KEYS::OFF_BAND], iops.iopns[KEYS::DIFFS],
                 iops.iopns[KEYS::NBLOCK], basis, (iops.iopns[KEYS::TASK] == TASKS::GFFULL) || (iops.iopns[KEYS::TASK] == TASKS::GFFULLDIFF));

    ovps.alloc_02();
  }
  ~GPU_GF2() {
    ovps.free_tau_02();
    ovps.free_02();
  }
  void monte_energy();
};

class GF2 : public GF {
 public:
  GF2(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : GF(p1, p2, p3, p4, p5, ep) {
    ovps.init(1, iops.iopns[KEYS::MC_NPAIR], basis);
    ovps.init_02(iops.iopns[KEYS::MC_NPAIR], iops.iopns[KEYS::NUM_BAND],
                 iops.iopns[KEYS::OFF_BAND], iops.iopns[KEYS::DIFFS],
                 iops.iopns[KEYS::NBLOCK], basis, (iops.iopns[KEYS::TASK] == TASKS::GFFULL) || (iops.iopns[KEYS::TASK] == TASKS::GFFULLDIFF));

    ovps.alloc_02();
    tau->resize(2);

    qeps.reserve(1);
    qeps.emplace_back(mpi_info.sys_master, mpi_info.numtasks, numBand, offBand, nDeriv, iops.sopns[KEYS::JOBNAME], 2);
  }
  ~GF2() {
    ovps.free_tau_02();
    ovps.free_02();
  }
 protected:
  void mc_local_energy(const int& step);
  int full_print(int& step, int checkNum);
};

class GPU_GF3 : public GF {
 protected:
  void mc_local_energy(std::vector<std::vector<double>>&, std::vector<std::vector<double>>&, int);

 public:
  GPU_GF3(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : GF(p1, p2, p3, p4, p5, ep) {
    ovps.init_03(iops.iopns[KEYS::MC_NPAIR], iops.iopns[KEYS::NUM_BAND],
                 iops.iopns[KEYS::OFF_BAND], iops.iopns[KEYS::DIFFS],
                 iops.iopns[KEYS::NBLOCK], basis, (iops.iopns[KEYS::TASK] == TASKS::GFFULL) || (iops.iopns[KEYS::TASK] == TASKS::GFFULLDIFF));
    ovps.alloc_03();
  }
  ~GPU_GF3() {
    ovps.free_tau_03();
    ovps.free_03();
  }
  void monte_energy();
};

class GF3 : public GF {
 public:
  GF3(MPI_info p1, IOPs p2, Molec p3, Basis p4, GTO_Weight p5, Electron_Pair_List* ep) : GF(p1, p2, p3, p4, p5, ep) {
    ovps.init(2, iops.iopns[KEYS::MC_NPAIR], basis);
    ovps.init_03(iops.iopns[KEYS::MC_NPAIR], iops.iopns[KEYS::NUM_BAND],
                 iops.iopns[KEYS::OFF_BAND], iops.iopns[KEYS::DIFFS],
                 iops.iopns[KEYS::NBLOCK], basis, (iops.iopns[KEYS::TASK] == TASKS::GFFULL) || (iops.iopns[KEYS::TASK] == TASKS::GFFULLDIFF));
    ovps.alloc_03();
    tau->resize(2);

    qeps.reserve(2);
    qeps.emplace_back(mpi_info.sys_master, mpi_info.numtasks, numBand, offBand, nDeriv, iops.sopns[KEYS::JOBNAME], 2);
    qeps.emplace_back(mpi_info.sys_master, mpi_info.numtasks, numBand, offBand, nDeriv, iops.sopns[KEYS::JOBNAME], 3);
  }
  ~GF3() {
    ovps.free_tau_03();
    ovps.free_03();
  }

 protected:
  void mc_local_energy(const int& step);
  int full_print(int& step, int checkNum);
};
#endif  // QC_MONTE_H_
