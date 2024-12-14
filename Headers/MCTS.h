// Author : Lindley

#pragma once
#include <torch/torch.h>
#include "Board.h"
#include <cmath>
#include <random>
#include <list>
#include <map>

using std::swap;
using std::mt19937;
using std::random_device;
using std::gamma_distribution;
using std::discrete_distribution;

struct Node
{
	Node* father;
	list<pair<pair<short, short>, Node*>> children;
	int vis;
	double Q, W;

	Node(Node* ptr)
	{
		father = ptr;
		vis = Q = W = 0;
	}

	void update(double val)
	{
		vis += 1;
		W += val;
		Q = W / (1.0 * vis);
	}

	void update_back(double val)
	{
		if (father != nullptr) 
			father->update_back(-val);
		update(val);
	}

	double get_val()
	{
		return sqrt(2.0 * log(1.0 * father->vis) / (1 + vis)) + Q;
	}

	void expand(MCTSBoard state)
	{
		if (state.avail.size() == 0) return;
		for (auto mov : state.avail)
			children.push_back(make_pair(mov, new Node(this)));
	}

	pair<pair<short, short>, Node*> select()
	{
		double maxx = -999999;
		pair<pair<short, short>, Node*> mxitem;
		for (auto item : children)
		{
			double val = item.second->get_val();
			if (val > maxx)
			{
				maxx = val;
				mxitem = item;
			}
		}
		return mxitem;
	}
};

struct MCTS
{
	int play_n;
	Node* root;

	MCTS(int n)
	{
		root = new Node(nullptr);
		play_n = n;
	}

	void clear(Node* cur)
	{
		if (cur->children.size() == 0) 
			return;
		for (auto item : cur->children)
		{
			clear(item.second);
			delete item.second;
		}
	}

	double eva_rol(MCTSBoard state)
	{
		int w = 0, l = 0;
		for (auto item : state.avail)
		{
			w += state.vplate[item.first][item.second];
			l += state._vplate[item.first][item.second];
			if (state.cur_ply != 1) 
				swap(w, l);
		}
		return 1.0 * (w - l) / (w + l + 1);
	}

	void playout(MCTSBoard state)
	{
		Node* node = root; 
		pair<short, short> mov = make_pair(0, 0);
		short win = 0; 
		double val = 0;
		while (node->children.size() != 0)
		{
			auto act = node->select();
			node = act.second;
			state.move(act.first);
			mov = act.first;
		}
		if (mov.first != 0) 
			win = state.get_winner(mov);
		if (win) 
			val = 1;
		else
		{
			if (state.ref_val() && node != root) 
				val = -1;
			else
			{
				val = -eva_rol(state);
				node->expand(state);
			}
		}
		node->update_back(val);
	}

	pair<short, short> get_move(MCTSBoard state)
	{
		state.resize();
		for (int i = 1; i <= state.avail.size() * play_n; i++) 
			playout(state);
		int maxx = 0; 
		pair<pair<short, short>, Node*> mxitem;

		if (state.lst_mov.first == 0)
		{
			vector <pair<short, short>> v_mov;

			for (auto item : root->children)
				v_mov.push_back(item.first);

			mt19937 gen(random_device{}());
			mxitem.first = v_mov[gen() % state.avail.size()];
		}
		else
		{
			for (auto item : root->children)
			{
				if (item.second->vis > maxx)
				{
					maxx = item.second->vis;
					mxitem = item;
				}
			}
		}

		clear(root);
		delete root;
		root = new Node(nullptr);
		return mxitem.first;
	}

	auto get_move_probs(MCTSBoard state) // This function is used first to generate probs for reinforcement learning
	{
		state.resize();
		for (int i = 1; i <= state.avail.size() * play_n; i++)
			playout(state);

		vector <float> v_probs; vector <pair<short, short>> v_mov;

		for (auto item : root->children)
		{
			v_probs.push_back(log(1.0 * item.second->vis + 1) / log(2));
			v_mov.push_back(item.first);
		}

		torch::Tensor t_probs = torch::tensor(v_probs, torch::TensorOptions()
			.dtype(torch::kFloat));
		t_probs = torch::softmax(t_probs, 0);

		//cout << t_probs;

		v_probs = vector <float>(t_probs.data_ptr<float>(),
			t_probs.data_ptr<float>() + t_probs.numel());

		pair<short, short> mxmov = make_pair(0, 0);
		if (state.lst_mov.first == 0)
		{
			mt19937 gen(random_device{}());
			discrete_distribution<> dist(v_probs.begin(), v_probs.end());
			mxmov = v_mov[dist(gen)];
		}
		else
		{
			double mxprob = 0;
			for (short i = 0; i <= v_probs.size() - 1; i++)
			{
				if (v_probs[i] > mxprob)
				{
					mxprob = v_probs[i];
					mxmov = v_mov[i];
				}
			}
		}

		vector<double> result(225, 0);
		for (short i = 0; i <= v_mov.size() - 1; i++)
			result[(v_mov[i].first - 1) * 15 + v_mov[i].second - 1] = v_probs[i];

		clear(root);
		root = new Node(nullptr);
		return make_tuple(result, mxmov);
	}
};