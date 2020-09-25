#include "gtest/gtest.h"
#include "../test_helper.h"

#include "dummy_electron_list.h"
#include "dummy_electron_pair_list.h"
#include "../../src/MCF12/correlation_factor_data.h"

namespace {
  std::vector<double> f12p_result() {
    std::vector<double> result = {
        0.0000000000000000e+00,
        8.9125887066303333e-01,
        1.0228390753523038e+00,
        1.0757796642511721e+00,
        1.1043596489127128e+00,
        1.1222483533326908e+00,
        1.1344996331410484e+00,
        1.1434156023585165e+00,
        1.1501950915696266e+00,
        1.1555238656732347e+00};
    return result;
  }
  std::vector<double> f12p_a_result() {
    std::vector<double> result = {
        -2.0000000000000000e+00,
        -3.4061966915876672e-02,
        -6.4356152548617736e-03,
        -2.2185251364769216e-03,
        -1.0125342967311672e-03,
        -5.4402022294892027e-04,
        -3.2525011200960349e-04,
        -2.0968933802809997e-04,
        -1.4298902188721613e-04,
        -1.0182803359465335e-04};
    return result;
  }
  std::vector<double> f12p_c_result() {
    std::vector<double> result = {
        1.0000000000000000e+00,
        2.5728427444747209e-01,
        1.4763410387308015e-01,
        1.0351694645735657e-01,
        7.9700292572739417e-02,
        6.4793038889424323e-02,
        5.4583639049126331e-02,
        4.7153664701236254e-02,
        4.1504090358644392e-02,
        3.7063445272304388e-02};
    return result;
  }
  std::vector<double> f12o_result() {
        std::vector<double> result = {
0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00,
1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00,
7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01,
1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00,
8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01,
9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00,
9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01,
8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00,
1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00,
7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00,
1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01,
0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00,
1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01,
7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01,
1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01,
8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01,
1.1043596489127128e+00, 1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00,
9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01,
1.1142267823062899e+00, 1.1043596489127128e+00, 1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00,
1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00};
    return result;
  }
  std::vector<double> f12o_b_result() {
    std::vector<double> result(100, 0.0);
    return result;
  }
  std::vector<double> f12o_d_result() {
      std::vector<double> result = {
0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02, -2.9332132413292633e-02, -1.8163190522006184e-02,
-1.2342554048979885e-02, -8.9297985032126895e-03, -6.7590990903054040e-03, -5.2934471968168846e-03, -4.2575491166276687e-03,
-1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02, -2.9332132413292633e-02,
-1.8163190522006184e-02, -1.2342554048979885e-02, -8.9297985032126895e-03, -6.7590990903054040e-03, -5.2934471968168846e-03,
-5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02,
-2.9332132413292633e-02, -1.8163190522006184e-02, -1.2342554048979885e-02, -8.9297985032126895e-03, -6.7590990903054040e-03,
-2.9332132413292633e-02, -5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01,
-5.5162664898301794e-02, -2.9332132413292633e-02, -1.8163190522006184e-02, -1.2342554048979885e-02, -8.9297985032126895e-03,
-1.8163190522006184e-02, -2.9332132413292633e-02, -5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00,
-1.3958484311375471e-01, -5.5162664898301794e-02, -2.9332132413292633e-02, -1.8163190522006184e-02, -1.2342554048979885e-02,
-1.2342554048979885e-02, -1.8163190522006184e-02, -2.9332132413292633e-02, -5.5162664898301794e-02, -1.3958484311375471e-01,
0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02, -2.9332132413292633e-02, -1.8163190522006184e-02,
-8.9297985032126895e-03, -1.2342554048979885e-02, -1.8163190522006184e-02, -2.9332132413292633e-02, -5.5162664898301794e-02,
-1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02, -2.9332132413292633e-02,
-6.7590990903054040e-03, -8.9297985032126895e-03, -1.2342554048979885e-02, -1.8163190522006184e-02, -2.9332132413292633e-02,
-5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01, -5.5162664898301794e-02,
-5.2934471968168846e-03, -6.7590990903054040e-03, -8.9297985032126895e-03, -1.2342554048979885e-02, -1.8163190522006184e-02,
-2.9332132413292633e-02, -5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00, -1.3958484311375471e-01,
-4.2575491166276687e-03, -5.2934471968168846e-03, -6.7590990903054040e-03, -8.9297985032126895e-03, -1.2342554048979885e-02,
-1.8163190522006184e-02, -2.9332132413292633e-02, -5.5162664898301794e-02, -1.3958484311375471e-01, 0.0000000000000000e+00};
    return result;
  }
  std::vector<double> f13_result() {
        std::vector<double> result = {
    0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00,
1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00,
7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01,
1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00,
8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01,
9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00,
9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01,
8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00,
1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00,
7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00,
1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01,
0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00,
1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01,
7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01,
1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00, 9.7486465224187979e-01,
8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01,
1.1043596489127128e+00, 1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00, 1.0228390753523038e+00,
9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00, 7.0887617762872868e-01,
1.1142267823062899e+00, 1.1043596489127128e+00, 1.0919272317924273e+00, 1.0757796642511721e+00, 1.0539591379214803e+00,
1.0228390753523038e+00, 9.7486465224187979e-01, 8.9125887066303333e-01, 7.0887617762872868e-01, 0.0000000000000000e+00};
    return result;
  }
  std::vector<double> f23_result() {
        std::vector<double> result = {
0.0000000000000000e+00, 7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00,
1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00,
7.0887617762872868e-01, 8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00,
1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00,
8.9125887066303333e-01, 9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00,
1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00,
9.7486465224187979e-01, 1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00,
1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00, 1.1344996331410484e+00,
1.0228390753523038e+00, 1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00,
1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00, 1.1344996331410484e+00, 1.1392831887062083e+00,
1.0539591379214803e+00, 1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00,
1.1222483533326908e+00, 1.1288978736810185e+00, 1.1344996331410484e+00, 1.1392831887062083e+00, 1.1434156023585165e+00,
1.0757796642511721e+00, 1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00,
1.1288978736810185e+00, 1.1344996331410484e+00, 1.1392831887062083e+00, 1.1434156023585165e+00, 1.1470213530442905e+00,
1.0919272317924273e+00, 1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00,
1.1344996331410484e+00, 1.1392831887062083e+00, 1.1434156023585165e+00, 1.1470213530442905e+00, 1.1501950915696266e+00,
1.1043596489127128e+00, 1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00, 1.1344996331410484e+00,
1.1392831887062083e+00, 1.1434156023585165e+00, 1.1470213530442905e+00, 1.1501950915696266e+00, 1.1530100700479620e+00,
1.1142267823062899e+00, 1.1222483533326908e+00, 1.1288978736810185e+00, 1.1344996331410484e+00, 1.1392831887062083e+00,
1.1434156023585165e+00, 1.1470213530442905e+00, 1.1501950915696266e+00, 1.1530100700479620e+00, 1.1555238656732347e+00};
    return result;
  }

  template <template <typename, typename> typename Container, template <typename> typename Allocator>
  class Correlation_Factor_Data_Fixture {
    typedef Dummy_Electron_List<Container, Allocator> Electron_List_Type;
    typedef Dummy_Electron_Pair_List<Container, Allocator> Electron_Pair_List_Type;
    typedef Correlation_Factor_Data<Container, Allocator> Correlation_Factor_Data_Type; 

    public:
      Correlation_Factor_Data_Fixture() :
        electrons(10),
        electron_pairs(10),
        correlation_factor(CORRELATION_FACTORS::Rational),
        gamma(-1.0),
        beta(-1.0),
        electron_list(new Electron_List_Type(electrons)),
        electron_pair_list(new Electron_Pair_List_Type(electron_pairs)),
        correlation_factor_data(electrons, electron_pairs, correlation_factor, gamma, beta)
    {
      correlation_factor_data.update(electron_pair_list.get(), electron_list.get());
    }

    int electrons;
    int electron_pairs;
    CORRELATION_FACTORS::CORRELATION_FACTORS correlation_factor;
    double gamma;
    double beta;

    std::shared_ptr<Electron_List_Type> electron_list;
    std::shared_ptr<Electron_Pair_List_Type> electron_pair_list;
    Correlation_Factor_Data_Type correlation_factor_data;
  };

  template <typename T>
  class CorrelationFactorDataTest : public testing::Test {
   public:
     T correlation_factor_data_fixture;

     void check(const std::vector<double>& reference, const std::vector<double> trial, std::string test_name) {
       ASSERT_EQ(reference.size(), trial.size());
       for (int i = 0; i < reference.size(); ++i) {
         ASSERT_FLOAT_EQ(trial[i], reference[i]) << test_name;
       }
     }
  };

  using Implementations = testing::Types<
    Correlation_Factor_Data_Fixture<std::vector, std::allocator>,
    Correlation_Factor_Data_Fixture<thrust::device_vector, thrust::device_allocator>
    >;
  TYPED_TEST_SUITE(CorrelationFactorDataTest, Implementations);

  TYPED_TEST(CorrelationFactorDataTest, f12pTest) {
    this->check(f12p_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12p),
        "f12pTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f12paTest) {
    this->check(f12p_a_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12p_a),
        "f12paTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f12pcTest) {
    this->check(f12p_c_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12p_c),
        "f12pcTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f12oTest) { 
    this->check(f12o_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12o),
        "f12oTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f12obTest) {
    this->check(f12o_b_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12o_b),
        "f12obTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f12odTest) { 
    this->check(f12o_d_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f12o_d),
        "f12odTest");
  }
  TYPED_TEST(CorrelationFactorDataTest, f13Test) {
    this->check(f13_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f13),
        "f13Test");
  }
  TYPED_TEST(CorrelationFactorDataTest, f23Test) { 
    this->check(f23_result(),
        get_vector(this->correlation_factor_data_fixture.correlation_factor_data.f23),
        "f23Test");
  }
}
