
#include <iostream>
#include <fstream>
#include <assert.h>
#include <cmath>
#include <cstring>
#include "test_types.h"
#include "gtest/gtest.h"

#include "../src/Kernel.h"
#include "../src/LogicalCube.h"
#include "../src/Layer.h"
#include "../src/util.h"
#include "../src/Connector.h"
#include "../src/bridges/ConvolutionBridge.h"

#include "../src/kernels/include.h"

template <typename T>
void simple_conv(LogicalCube<T, Layout_CRDB>* in, LogicalCube<T, Layout_CRDB>* kernel, LogicalCube<T, Layout_CRDB>* out){
  int ofm = out->D;
  int ifm = in->D;
  for (int n = 0; n < out->B; n++) {
    for (int o = 0; o < ofm; o++) {
      for (int k = 0; k < ifm; k++) {
        for (int y = 0; y < out->R; y++) {
          for (int x = 0; x < out->C; x++) {
            for (int p = 0; p < kernel->R; p++) {
              for (int q = 0; q < kernel->C; q++) {
                int in_y = y + p;
                int in_x = x + q;
                *out->logical_get(y, x, o, n) +=
                  *in->logical_get(in_y, in_x, k, n)*
                  *kernel->logical_get(p, q, k, o);
              }
            }
          }
        }
      }
    }
  }
}

template <typename TypeParam>
class ParallelizedConvolutionBridgeTest2 : public ::testing::Test {
  public:
    typedef typename TypeParam::T T;
    ParallelizedConvolutionBridgeTest2(){
      data1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);
      grad1 = new LogicalCube<T, Layout_CRDB>(iR, iC, iD, mB);

      data2 = new LogicalCube<T, Layout_CRDB>(oR, oC, oD, mB);
      grad2 = new LogicalCube<T, Layout_CRDB> (oR, oC, oD, mB);

      layer1 = new Layer<T, Layout_CRDB>(data1, grad1);
      layer2 = new Layer<T, Layout_CRDB>(data2, grad2);

      cnn::LayerParameter layer_param;
      cnn::ConvolutionParameter * const conv_param = layer_param.mutable_convolution_param();
      conv_param->set_num_output(oD);
      conv_param->set_kernel_size(k);
      conv_param->set_pad(p);
      conv_param->set_stride(s);

      solver_param.set_base_lr(0.01);
      solver_param.set_momentum(0.0);
      solver_param.set_lr_policy("step");
      solver_param.set_stepsize(10000);

      ConvolutionBridge_ = new ConvolutionBridge<DataType_SFFloat, Layout_CRDB, DataType_SFFloat, Layout_CRDB, CPUDriver>
              (layer1, layer2, &layer_param, &solver_param, &pdriver);

      ConvolutionBridge_->run_with_n_threads = 1;

      ConvolutionBridge_->needs_to_calc_backward_grad = true;
    }

    virtual ~ParallelizedConvolutionBridgeTest2() { delete layer1; delete layer2; }

    ConvolutionBridge<DataType_SFFloat,
      Layout_CRDB, DataType_SFFloat, Layout_CRDB, CPUDriver> * ConvolutionBridge_;

    LogicalCube<T, Layout_CRDB>* data1;
    LogicalCube<T, Layout_CRDB>* grad1;

    LogicalCube<T, Layout_CRDB>* data2;
    LogicalCube<T, Layout_CRDB>* grad2;

    Layer<T, Layout_CRDB>* layer1;
    Layer<T, Layout_CRDB>* layer2;

    cnn::SolverParameter solver_param;

    CPUDriver pdriver;

    /*
    static const int mB = 1;
    static const int iD = 3;
    static const int oD = 10;
    static const int iR = 3;
    static const int iC = 3;
    static const int k = 2;
    static const int s = 2;
    static const int p = 1;
    */

    /*
    static const int mB = 4;
    static const int iD = 3;
    static const int oD = 10;
    static const int iR = 20;
    static const int iC = 20;
    static const int k = 5;
    static const int s = 2;
    static const int p = 2;
    */


    static const int mB = 4;
    static const int iD = 3;
    static const int oD = 10;
    static const int iR = 20;
    static const int iC = 20;
    static const int k = 5;
    static const int s = 4;
    static const int p = 2;


    /*
    static const int mB = 32;
    static const int iD = 48;
    static const int oD = 128;
    static const int iR = 27;
    static const int iC = 27;
    static const int k = 5;
    static const int s = 1;
    static const int p = 2;
    */

    static const int oR = static_cast<int>((static_cast<float>(iR + 2*p - k) / s)) + 1;
    static const int oC = static_cast<int>((static_cast<float>(iC + 2*p - k) / s)) + 1;
};

typedef ::testing::Types<FloatNOFUNC> DataTypes;

TYPED_TEST_CASE(ParallelizedConvolutionBridgeTest2, DataTypes);

TYPED_TEST(ParallelizedConvolutionBridgeTest2, TestInitialization){
  EXPECT_TRUE(this->ConvolutionBridge_);
  EXPECT_TRUE(this->layer1);
  EXPECT_TRUE(this->layer2);
}

TYPED_TEST(ParallelizedConvolutionBridgeTest2, TestForward){


  std::fstream input("tests/input/conv_forward_in.txt", std::ios_base::in);
  if (input.is_open()){
    for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
      input >> this->data1->get_p_data()[i];
      this->grad1->get_p_data()[i] = 0;
    }
  }
  else{
    FAIL();
  }
  input.close();

  std::fstream model("tests/input/conv_model.txt", std::ios_base::in);
  if (model.is_open()){
    for(int i=0;i<this->iR*this->iC*this->iD*this->oD;i++){
      model >> this->ConvolutionBridge_->get_model_cube()->get_p_data()[i];
    }
  }
  else{
    FAIL();
  }
  model.close();

  std::fstream bias_file("tests/input/conv_bias_in.txt", std::ios_base::in);
  if (bias_file.is_open()){
    for(int i=0;i<this->oD;i++){
      bias_file >> this->ConvolutionBridge_->get_bias_cube()->get_p_data()[i];
    }
  }
  else{
    FAIL();
  }
  bias_file.close();


  /*
  for(int i=0;i<this->iR*this->iC*this->iD*this->mB;i++){
      this->data1->get_p_data()[i] = i;
  }

  for(int i=0;i<this->k*this->k*this->iD*this->oD;i++){
    this->ConvolutionBridge_->get_model_cube()->get_p_data()[i] = 1;
  }
  */


  this->ConvolutionBridge_->run_with_n_threads = 2;
  this->ConvolutionBridge_->forward();

  this->ConvolutionBridge_->report_forward_last_transfer.print();
  this->ConvolutionBridge_->report_forward_kernel.print();
  this->ConvolutionBridge_->report_forward_lowering.print();

  //this->data2->logical_print();
  // float sum = 0.0;
  // for (int i=0;i<this->data2->R*this->data2->C*this->data2->D*this->data2->B;i++) {
  //   sum += this->data2->get_p_data()[i];
  // }
  // cout << "sum: " << sum << endl;

  //this->ConvolutionBridge_->backward();


  std::fstream expected_output("tests/output/conv_forward.txt", std::ios_base::in);
  if(TypeParam::FUNC == FUNC_NOFUNC){
    float output;
    int idx = 0;
    if (expected_output.is_open()) {

      while (expected_output >> output)
        EXPECT_NEAR(this->data2->get_p_data()[idx++], output, EPS);

    }else{
      FAIL();
    }
    expected_output.close();
  }
}



