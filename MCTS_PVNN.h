#pragma once
#include "Board.h"
#include "MCTS.h"
#include "NN.h"
#include "Katagomo.h"
#include <cmath>
#include <random>
#include <tuple>
#include <list>
#include <map>

struct PVNN_Node
{
	PVNN_Node* father;
	list<pair<pair<short, short>, PVNN_Node*>> children;
	int vis;
	double Q, W, P;

	PVNN_Node(PVNN_Node* ptr, double p)
	{
		father = ptr;
		vis = Q = W = P = 0;
		P = p;
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
		return 1.4 * P * sqrt(1.0 * father->vis) / (1 + vis) + Q;
	}

	void expand(Board state, list<double> probs)
	{
		if (state.avail.size() == 0) return;
		auto iter = probs.begin();
		for (auto mov : state.avail)
		{
			children.push_back(make_pair(mov, new PVNN_Node(this, *iter)));
			iter++;
		}
	}

	auto select()
	{
		double maxx = -999999;
		pair<pair<short, short>, PVNN_Node*> mxitem;
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

struct MCTS_PVNN
{
	int play_n;
	PVNN_Node* root;
	Katagomo bot = Katagomo();
	PVNet net;

	MCTS_PVNN(int n) : net(string("pvnet.pth"))
	{
		root = new PVNN_Node(nullptr, 1.0);
		play_n = n;
	}

	void clear(PVNN_Node* cur)
	{
		if (cur->children.size() == 0) return;
		for (auto item : cur->children)
		{
			clear(item.second);
			delete item.second;
		}
	}

	vector<double> drcl_dist(int len, double x)
	{
		vector<double> d, _d;
		double s = 0;
		mt19937 gen(random_device{}());
		gamma_distribution<double> gamma(x, 1.0);
		for (int i = 1; i <= len; i++)
		{
			double k = gamma(gen);
			s += k;
			d.push_back(k);
		}
		for (auto item : d) _d.push_back(item / s);
		return _d;
	}

	void playout(Board state)
	{
		PVNN_Node* node = root; 
		pair<short, short> mov = make_pair(0, 0);
		short win = 0; 
		double val = 0;
		while (node->children.size() != 0)
		{
			pair<pair<short, short>, PVNN_Node*> act = node->select();
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
			auto tup = net.get_PV(state);
			val = get<1>(tup);
			node->expand(state, get<0>(tup));
		}
		node->update_back(val);
	}

	auto get_move(Board state)
	{
		//state.resize();
		for (int i = 1; i <= play_n; i++) 
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
		root = new PVNN_Node(nullptr, 1.0);
		return mxmov;
	}

	pair<short, short> get_move_rand(Board state)
	{
		if (state.lst_mov.first == 0)
		{
			state.resize();
			vector <pair<short, short>> v_mov;

			for (auto item : state.avail)
				v_mov.push_back(item);

			mt19937 gen(random_device{}());
			return v_mov[gen() % state.avail.size()];
		}

		playout(state);

		vector <pair<short, short>> v_mov; 
		pair<short, short> mxmov; 
		double mxvis = 0;

		for (auto item : root->children)
		{
			if (item.second->P > mxvis)
			{
				mxvis = item.second->P;
				mxmov = item.first;
			}
			v_mov.push_back(item.first);
		}

		clear(root);
		delete root;
		root = new PVNN_Node(nullptr, 1.0);

		return mxmov;
	}

	auto get_train_data(Board state, int str_hand) // Get train data
	{
		vector<int> cur_states; // winner data
		vector<double> cur_probs;
		vector<double> cur_wins;
		vector<pair<short, short>> cur_movs;

		int len = 0;
		bot.clear();
		while (1)
		{
			len++;

			auto tup = bot.get_move_single(state, "2");

			pair<short, short> mov;
			if (len % 2 == str_hand) 
				mov = get_move_rand(state);
			else mov = get<1>(tup);

			cur_movs.push_back(get<0>(tup));
			cur_wins.push_back(get<2>(tup));

			auto cur_state = state.get_sit();
			for (auto item : cur_state)
				cur_states.push_back(item);

			state.move(mov);
			//state.print();
			short win = state.get_winner(mov);
			if (win)
			{
				if ((win == 1 && str_hand == 1) || (win == -1 && str_hand == 0)) 
					printf("NN   "); //print winner
				else 
					printf("MCTS ");
				printf("%d ", len);
				cur_probs.resize(len * 225);
				for (int i = 0; i <= len - 1; i++)
				{
					for (int j = 0; j <= 224; j++)
						cur_probs[i * 225 + j] = 0;
					cur_probs[i * 225 + (cur_movs[i].first - 1) * 15 + cur_movs[i].second - 1] = 1;
				}
				break;
			}
			else if (state.avail.size() == 0)
			{
				printf("DRAW \n");
				break;
			}
		}
		return make_tuple(cur_states, cur_probs, cur_wins);
	}

	vector <CompressedBoard> get_supervised_train_data(Board board, int depth)
	{
		board.print();
		auto tup = bot.get_move_list(board, "2");

		vector <CompressedBoard> datas;
		CompressedBoard data;
		data.encodeBoard(board.get_sit(), board.lst_mov, get<0>(tup)[0], get<1>(tup));
		if (board.lst_mov.first != 0)
			datas.push_back(data);
		
		auto nexts = get<0>(tup);
		if (depth % 2 == 0)
			nexts.resize(1);
		for (auto item : nexts)
		{
			Board _board = board;

			_board.move(item);
			if (_board.get_winner(item) == 0 && depth <= 24)
			{
				auto temp = get_supervised_train_data(_board, depth + 1);
				datas.insert(datas.end(), make_move_iterator(temp.begin()),
					make_move_iterator(temp.end()));
			}
		}
		bot.undo();
		return datas;
	}

	void supervised_train(Board board)
	{
		bot.init();
		auto datas = get_supervised_train_data(board, 1);
		printf("%d\n", datas.size());

		int batchs = 10000;
		mt19937 gen(random_device{}());
		while (batchs--)
		{
			printf("%d ", batchs);

			vector<int> chs_states;
			vector<double> chs_probs;
			vector<double> chs_wins;

			while (chs_wins.size() != 24)
			{
				auto tup = datas[gen() % datas.size()].decodeBoard();
				auto sit = get<0>(tup);
				auto targ = get<1>(tup);
				auto val = get<2>(tup);

				for (int i = 0; i <= 674; i++)
					chs_states.push_back(sit[i]);

				for (int i = 1; i <= 15; i++)
					for (int j = 1; j <= 15; j++)
					{
						if (targ.first == i && targ.second == j)
							chs_probs.push_back(1);
						else
							chs_probs.push_back(0);
					}

				chs_wins.push_back(val);
			}

			net.train(chs_states, chs_probs, chs_wins);
			printf("\n");

			if (batchs % 5 == 0)
			{
				torch::serialize::OutputArchive out_arc;
				net.net.save(out_arc);
				out_arc.save_to("pvnet.pth");
			}
		}
	}

	void train(Board board)
	{
		bot.init();
		int batchs = 10000;
		while (batchs--)
		{
			printf("%d ", batchs);
			auto tup = get_train_data(board, batchs % 2);
			if (get<2>(tup).size() == 0) continue;

			vector<int> buf_states = get<0>(tup);
			vector<double> buf_probs = get<1>(tup);
			vector<double> buf_wins = get<2>(tup);

			vector<int> chs_states;
			vector<double> chs_probs;
			vector<double> chs_wins;

			for (int i = 2; i <= buf_wins.size(); i++)
			{
				for (int j = 1; j <= 675; j++)
					chs_states.push_back(buf_states[(i - 1) * 675 + j - 1]);
				for (int j = 1; j <= 225; j++)
					chs_probs.push_back(buf_probs[(i - 1) * 225 + j - 1]);
				chs_wins.push_back(buf_wins[i - 1]);
				if (i == 8 || i == 16 || (i == buf_wins.size() && i <= 16))
				{
					net.train(chs_states, chs_probs, chs_wins);
					chs_states.clear();
					chs_probs.clear();
					chs_wins.clear();
				}
			}
			printf("\n");

			if (batchs % 5 == 0)
			{
				torch::serialize::OutputArchive out_arc;
				net.net.save(out_arc);
				out_arc.save_to("pvnet.pth");
			}
		}
	}
};
