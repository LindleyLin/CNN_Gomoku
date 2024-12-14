// Author : Lindley

#pragma once
#include <torch/torch.h>
#include <tuple>
#include "Board.h"

namespace nn = torch::nn;
namespace fu = torch::nn::functional;
namespace op = torch::optim;

using std::list;
using std::string;
using std::vector;
using std::make_tuple;
using std::get;
using std::tie;

struct SE : torch::nn::Module // SE Block
{
    nn::Linear fc1, fc2;

    SE(int chn, int red) :
        fc1(chn, chn / red),
        fc2(chn / red, chn)
    {
        register_module("fc1", fc1);
        register_module("fc2", fc2);
    }

    torch::Tensor forward(torch::Tensor x) // Forward
    {
        int chn = x.size(1);

        auto ext = fu::adaptive_avg_pool2d(x, fu::AdaptiveAvgPool2dFuncOptions({1, 1}));
        ext = fu::relu(fc1(ext.view({-1, chn})));
        ext = torch::sigmoid(fc2(ext)).view({-1, chn, 1, 1});

        return x * ext;
    }
};

struct Res : torch::nn::Module // Residual Block
{
	nn::Conv2d conv1, conv2; // Convolution
	nn::BatchNorm2d bn1, bn2; // Normalization
    nn::Sequential se;

    Res(int in_c, int out_c) :
        conv1(nn::Conv2dOptions(in_c, out_c, 3).padding(1)), 
        conv2(nn::Conv2dOptions(out_c, out_c, 3).padding(1)),
        bn1(nn::BatchNorm2dOptions(out_c)),
        bn2(nn::BatchNorm2dOptions(out_c))
    {
        register_module("conv1", conv1);
        register_module("conv2", conv2);
        register_module("bn1", bn1);
        register_module("bn2", bn2);
        register_module("se", se);
        se->push_back(SE(out_c, 16));
    }

    torch::Tensor forward(torch::Tensor x) // Forward
    {
        auto res = x;
        x = torch::relu(bn1->forward(conv1->forward(x)));
        x = se->forward(x);
        x = bn2->forward(conv2->forward(x));
        x += res;
        return torch::relu(x);
    }
};

struct NetPV : nn::Module
{
    nn::Sequential layers; // A Sequence of Residual Layers 
    nn::Conv2d act_conv, val_conv, init_conv;
    nn::BatchNorm2d act_bn, val_bn, init_bn;
    nn::Linear act_fc, val_fc1, val_fc2;

    NetPV() :
        init_conv(nn::Conv2dOptions(3, 128, 3).padding(1)),
        act_conv(nn::Conv2dOptions(128, 2, 1)),
        val_conv(nn::Conv2dOptions(128, 1, 1)),
        init_bn(nn::BatchNorm2dOptions(128)),
        act_bn(nn::BatchNorm2dOptions(2)),
        val_bn(nn::BatchNorm2dOptions(1)),
        act_fc(2 * 225, 225), val_fc1(1 * 225, 128), val_fc2(128, 1)
    {
        for (int i = 1; i <= 8; i++)
            layers->push_back(Res(128, 128));
        register_module("layers", layers);
        register_module("init_conv", init_conv);
        register_module("act_conv", act_conv);
        register_module("val_conv", val_conv);
        register_module("init_bn", init_bn);
        register_module("act_bn", act_bn);
        register_module("val_bn", val_bn);
        register_module("act_fc", act_fc);
        register_module("val_fc1", val_fc1);
        register_module("val_fc2", val_fc2);
    }

    auto forward(torch::Tensor x)
    {
        x = fu::relu(init_bn->forward(init_conv->forward(x)));
        x = layers->forward(x);

        torch::Tensor x_act = act_bn->forward(act_conv->forward(x));
        x_act = fu::relu(x_act);
        x_act = x_act.view({ -1, 2 * 225 });
        x_act = act_fc->forward(x_act);

        torch::Tensor x_val = val_bn->forward(val_conv->forward(x));
        x_val = fu::relu(x_val);
        x_val = x_val.view({ -1, 1 * 225 });
        x_val = fu::relu(val_fc1->forward(x_val));
        x_val = torch::tanh(val_fc2->forward(x_val));

        return make_tuple(x_act, x_val);
    }
};

struct NetV : nn::Module
{
    nn::Conv2d val_conv1, val_conv2;
    nn::BatchNorm2d val_bn1, val_bn2;
    nn::Linear val_fc1, val_fc2;

    NetV() :
        val_conv1(nn::Conv2dOptions(3, 16, 3)),
        val_conv2(nn::Conv2dOptions(16, 64, 3)),
        val_bn1(nn::BatchNorm2dOptions(16)),
        val_bn2(nn::BatchNorm2dOptions(64)),
        val_fc1(121 * 64, 64), val_fc2(64, 1)
    {
        register_module("val_conv1", val_conv1);
        register_module("val_conv2", val_conv2);
        register_module("val_bn1", val_bn1);
        register_module("val_bn2", val_bn2);
        register_module("val_fc1", val_fc1);
        register_module("val_fc2", val_fc2);
    }

    auto forward(torch::Tensor x)
    {
        auto x_val = fu::relu(val_bn1->forward(val_conv1->forward(x)));
        x_val = fu::relu(val_bn2->forward(val_conv2->forward(x_val)));
        x_val = x_val.view({ -1, 121 * 64 });
        x_val = fu::relu(val_fc1->forward(x_val));
        x_val = torch::tanh(val_fc2->forward(x_val));

        return x_val;
    }
};


struct PVNet
{
    NetPV net;
    op::AdamW opt;

    PVNet(string _file) : opt(net.parameters(), op::AdamWOptions(2e-3))
    {
        if (_file.length())
        {
            torch::serialize::InputArchive arc;
            arc.load_from(_file.c_str());
            net.load(arc);
        }
        net.to(torch::kCUDA);
    }

    auto get_PV(Board board)
    {
        net.eval();

        vector <short> cursit = board.get_sit();
        torch::Tensor batch = torch::tensor(cursit, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCUDA)).view({ 1, 3, 15, 15 });

        auto tup = net.forward(batch);
        torch::Tensor probs, val;
        tie(probs, val) = tup;

        probs = (torch::softmax(probs, 1)).view({ 225 }).to(torch::kCPU);
        val = val.view({ 1 }).to(torch::kCPU);
        vector <float> P(probs.data_ptr<float>(), probs.data_ptr<float>() + probs.numel()),
            V(val.data_ptr<float>(), val.data_ptr<float>() + val.numel());

        list <double> p; 
        double v = V[0];
        for (auto mov : board.avail)
            p.push_back(P[(mov.first - 1) * 15 + mov.second - 1]);
        return make_tuple(p, v);
    }

    void train(vector<int> v_states, vector<double> v_probs,
        vector<double> v_wins)
    {
        auto states = torch::tensor(v_states, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCPU)).view({ -1, 3, 15, 15 });
        auto probs = torch::tensor(v_probs, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCPU)).view({ -1, 15, 15 });
        auto wins = torch::tensor(v_wins, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCPU)).view({ -1, 1 });

        auto _states = torch::cat({ states, states.rot90(1, { 2, 3 }), states.rot90(2, { 2, 3 }),
            states.rot90(3, { 2, 3 }), states.flip(2), states.flip(3), states.transpose(2, 3),
            states.rot90(2, { 2, 3 }).transpose(2, 3) }).to(at::kCUDA);
        auto _probs = torch::cat({ probs, probs.rot90(1, { 1, 2 }), probs.rot90(2, { 1, 2 }),
            probs.rot90(3, { 1, 2 }), probs.flip(1), probs.flip(2), probs.transpose(1, 2),
            probs.rot90(2, { 1, 2 }).transpose(1, 2) }).to(at::kCUDA);
        auto _wins = torch::cat({ wins,wins,wins,wins,wins,wins,wins,wins }).to(at::kCUDA);
        net.train();

        int epochs = 5;
        while (epochs--)
        {
            opt.zero_grad();

            auto result = net.forward(_states);

            auto val_loss = torch::mse_loss(get<1>(result), _wins);
            auto pol_loss = torch::cross_entropy_loss(get<0>(result), _probs.view({ -1, 225 }));;
            auto loss = val_loss + pol_loss;

            if (epochs == 4)
                printf("%.3f ", loss.item().toFloat());

            auto kl_loss = torch::kl_div(torch::log_softmax(get<0>(result), 1), _probs.view({ -1, 225 }));
            if (epochs == 4)
                printf("%.4f ", kl_loss.item().toFloat());

            // If the dataset is not balanced, please cancel the annotation

            /*double lr = 2e-5;
            if (kl_loss.item().toFloat() > 1e-2)
                lr *= 2;

            if (kl_loss.item().toFloat() < 1e-3)
                break;

            for (auto& param_group : opt.param_groups())
                param_group.options().set_lr(lr);*/

            loss.backward();

            opt.step();
        }

        net.zero_grad();
    }
};

struct VNet
{
    NetV net;
    op::AdamW opt;

    VNet(string _file) : opt(net.parameters(), op::AdamWOptions(1e-3))
    {
        if (_file.length())
        {
            torch::serialize::InputArchive arc;
            arc.load_from(_file.c_str());
            net.load(arc);
        }
        net.to(torch::kCUDA);
    }

    auto get_V(Board board)
    {
        net.eval();

        vector <short> cursit = board.get_sit();
        torch::Tensor batch = torch::tensor(cursit, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCUDA)).view({ 1, 3, 15, 15 });

        auto val = net.forward(batch).view({ 1 }).to(torch::kCPU);
        vector <float> V(val.data_ptr<float>(), val.data_ptr<float>() + val.numel());

        return (double) V[0];
    }

    void train(vector<int> v_states, vector<int> v_wins)
    {
        auto states = torch::tensor(v_states, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCPU)).view({ -1, 3, 15, 15 });
        auto wins = torch::tensor(v_wins, torch::TensorOptions()
            .dtype(torch::kFloat).device(torch::kCPU)).view({ -1, 1 });

        auto _states = torch::cat({ states, states.rot90(1, { 2, 3 }), states.rot90(2, { 2, 3 }),
            states.rot90(3, { 2, 3 }), states.flip(2), states.flip(3), states.transpose(2, 3),
            states.rot90(2, { 2, 3 }).transpose(2, 3) }).to(at::kCUDA);
        auto _wins = torch::cat({ wins,wins,wins,wins,wins,wins,wins,wins }).to(at::kCUDA);
        net.train();

        int epochs = 5;
        while (epochs--)
        {
            opt.zero_grad();

            auto result = net.forward(_states);

            auto val_loss = torch::mse_loss(result , _wins);

            if (epochs == 4)
                printf("%.4f ", val_loss.item().toFloat());

            val_loss.backward();

            opt.step();
        }

        net.zero_grad();
    }
};