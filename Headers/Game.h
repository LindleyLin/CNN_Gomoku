// Author : Lindley

#pragma once
#include "Board.h"
#include "Human.h"
#include "MCTS.h"
#include "MCTS_PVNN.h"
#include "MCTS_VNN.h"
#include "Katagomo.h"

struct Game
{
	Board trainboard; // Basic board (Only for training)
	MCTSBoard board; // Complex board (including some board features)
	MCTS mcts = MCTS(200); // Pure MCTS Bot
	MCTS_PVNN mctspvnn = MCTS_PVNN(2); // MCTS Bot with outputs both policy and value
	MCTS_VNN mctsvnn = MCTS_VNN(200); // MCTS Bot with outputs value only
	Katagomo katagomo = Katagomo(); // Katagomo bot (Teacher)
	Human human; // If you want to play

	void start_play(int str_ply) // 1 stands for O (Black) and -1 stands for X 
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