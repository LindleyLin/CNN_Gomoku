#pragma once
#include "Board.h"
#include "Human.h"
#include "MCTS.h"
#include "MCTS_PVNN.h"
#include "MCTS_VNN.h"
#include "Katagomo.h"

struct Game
{
	Board trainboard;
	MCTSBoard board;
	MCTS mcts = MCTS(200);
	MCTS_PVNN mctspvnn = MCTS_PVNN(2);
	MCTS_VNN mctsvnn = MCTS_VNN(200);
	Katagomo katagomo = Katagomo();
	Human human;

	void start_play(int str_ply)
	{
		katagomo.init();
		board.init(str_ply);
		board.print();
		pair<short, short> move;
		while (true)
		{
			move = katagomo.get_move(board);
			board.move(move);
			board.print();
			int win = board.get_winner(move);
			if (win)
			{
				printf("Winner is Player %d", win);
				break;
			}

			move = mcts.get_move(board);
			board.move(move);
			board.print();
			win = board.get_winner(move);
			if (win)
			{
				printf("Winner is Player %d", win);
				break;
			}
		}
	}

	void train()
	{
		board.init(1);
		mctspvnn.supervised_train(trainboard);
	}
};