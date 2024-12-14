// Author : Lindley

#pragma once
#include <list>
#include "Board.h"

struct Human
{
	Board board;

	auto get_move(Board board)
	{
		short i = 0, j = 0;
		while (1)
		{
			if (board.cur_ply == 1) 
				printf("O -> ");
			else 
				printf("X -> ");
			printf("Move : ");
			scanf("%d%d", &i, &j);
			auto move = make_pair(i, j);
			auto it = std::find(board.avail.begin(), board.avail.end(), make_pair(i, j));
			if (it != board.avail.end()) 
				return move;
			printf("Invalid Input\n");
		}
	}
};