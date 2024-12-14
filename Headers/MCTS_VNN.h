#pragma once
#include "Board.h"
#include "MCTS.h"
#include "NN.h"
#include <cmath>
#include <random>
#include <tuple>
#include <list>
#include <map>

struct VNN_Node
{
	VNN_Node* father;
	list<pair<pair<short, short>, VNN_Node*>> children;
	int vis;
	double Q, W;


	VNN_Node(VNN_Node* ptr)
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
		if (father != nullptr) father->update_back(-val);
		update(val);
	}

	double get_val()
	{
		return sqrt(2.0 * log(1.0 * father->vis) / (1 + vis)) + Q;
	}

	void expand(Board state)
	{
		if (state.avail.size() == 0) return;
		for (auto mov : state.avail)
			children.push_back(make_pair(mov, new VNN_Node(this)));
	}

	pair<pair<short, short>, VNN_Node*> select()
	{
		double maxx = -999999;
		pair<pair<short, short>, VNN_Node*> mxitem;
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

struct MCTS_VNN
{
	int play_n;
	VNN_Node* root;
	MCTS mcts;
	VNet net;

	MCTS_VNN(int n) : net(string("vnet.pth")), mcts(200)
	{
		root = new VNN_Node(nullptr);
		play_n = n;
	}

	void clear(VNN_Node* cur)
	{
		if (cur->children.size() == 0) return;
		for (auto item : cur->children)
		{
			clear(item.second);
			delete item.second;
		}
	}

	void playout(MCTSBoard state)
	{
		VNN_Node* node = root;
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
				val = net.get_V(state);
				node->expand(state);
			}
		}
		node->update_back(val);
	}

	auto get_move(MCTSBoard state)
	{
		state.resize();
		for (int i = 1; i <= state.avail.size() * play_n; i++) 
			playout(state);
		pair<short, short> mxmov;
		double mxvis = 0;
		for (auto item : root->children)
		{
			if (item.second->vis > mxvis)
			{
				mxvis = item.second->vis;
				mxmov = item.first;
			}
		}
		clear(root);
		delete root;
		root = new VNN_Node(nullptr);
		return mxmov;
	}

	auto get_train_data(MCTSBoard state, int str_hand) // Get train data
	{
		vector<int> cur_states; // winner data
		vector<int> cur_wins;

		int len = 0;
		while (1)
		{
			len++;

			pair<short, short> mov;
			if (len % 2 == str_hand) 
				mov = get_move(state);
			else 
				mov = mcts.get_move(state);
			//tup = get_move(state);

			auto cur_state = state.get_sit();
			for (auto item : cur_state)
				cur_states.push_back(item);

			state.move(mov);
			//state.print();
			int win = state.get_winner(mov);
			if (win)
			{
				if ((win == 1 && str_hand == 1) || (win == -1 && str_hand == 0)) printf("NN   "); // print winner
				else printf("MCTS ");
				printf("%d ", len);
				cur_wins.resize(len);
				for (int i = 0; i <= len - 1; i++)
				{
					if ((len + i) % 2 == 1) cur_wins[i] = 1;
					else cur_wins[i] = -1;
				}
				break;
			}
			else if (state.avail.size() == 0)
			{
				printf("DRAW \n");
				break;
			}
		}
		return make_tuple(cur_states, cur_wins);
	}

	void train(MCTSBoard board)
	{
		int batchs = 500;
		while (batchs--)
		{
			printf("%d ", batchs);
			auto tup = get_train_data(board, batchs % 2);
			if (get<1>(tup).size() == 0) continue;

			net.train(get<0>(tup), get<1>(tup));
			if (batchs % 5 == 0)
			{
				torch::serialize::OutputArchive out_arc;
				net.net.save(out_arc);
				out_arc.save_to("vnet.pth");
			}
		}
	}
};


