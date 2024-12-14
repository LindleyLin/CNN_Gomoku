#pragma once
#include <vector>
#include <list>

using std::list;
using std::pair;
using std::vector;
using std::make_pair;
using std::max;
using std::min;

struct Board
{
	vector <short> plate[16]; //Chess Plate
	list <pair<short, short>> avail, moves; //Empty Locations
	short cur_ply = 0, str_ply = 0; //Current Player (1, -1)
	pair<short, short> lst_mov;

	void init(short ply)
	{
		str_ply = cur_ply = ply;
		lst_mov = make_pair(0, 0);

		for (short i = 1; i <= 15; i++)
		{
			plate[i].resize(16);
			for (short j = 1; j <= 15; j++)
				avail.push_back(make_pair(i, j));
		}
	}

	void move(pair<short, short> mov)
	{
		lst_mov = mov;
		plate[mov.first][mov.second] = cur_ply;
		avail.remove(mov);
		moves.push_back(mov);
		cur_ply = -cur_ply;
	}

	vector<short> get_sit()
	{
		vector<short> cur_sit(675, 0);
		for (short i = 1; i <= 15; i++)
			for (short j = 1; j <= 15; j++)
				if (plate[i][j] != 0)
				{
					if (plate[i][j] == cur_ply)
						cur_sit[(i - 1) * 15 + j - 1] = 1;
					else
						cur_sit[(i - 1) * 15 + j - 1 + 225] = 1;
				}
		if (lst_mov.first != 0)
			cur_sit[(lst_mov.first - 1) * 15 + lst_mov.second - 1 + 450] = 1;
		return cur_sit;
	}

	void print()
	{
		printf("Player 1 -> O\nPlayer 2 -> X\n");
		printf("    1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  \n\n");
		for (int i = 1; i <= 15; i++)
		{
			if (i <= 9)
				printf("%d   ", i);
			else
				printf("%d  ", i);
			for (int j = 1; j <= 15; j++)
			{
				if (plate[i][j] == 0)
					printf(".");
				else if (plate[i][j] == 1)
					printf("O");
				else
					printf("X");
				if (i == lst_mov.first && j == lst_mov.second)
					printf("<- ");
				else
					printf("   ");
			}
			printf("\n\n");
		}
	}

	void resize()
	{
		list <pair<short, short>> ava;
		if (avail.size() == 225)
		{
			for (short i = 2; i <= 14; i++)
				for (short j = 2; j <= 14; j++)
					if (j >= i && j <= 8 && i == 2)
						if (abs(i - 8) + abs(j - 8) <= 7 && abs(i - 8) + abs(j - 8) >= 6)
							ava.push_back(make_pair(i, j));
		}
		else
		{
			Board board = *this;
			for (short i = 1; i <= 15; i++)
				for (short j = 1; j <= 15; j++)
					if (board.plate[i][j] == 1 || board.plate[i][j] == -1)
						for (short p = max(1, i - 2); p <= min(15, i + 2); p++)
							for (short q = max(1, j - 2); q <= min(15, j + 2); q++)
								if (board.plate[p][q] == 0)
									board.plate[p][q] = 3;
			for (short i = 1; i <= 15; i++)
				for (short j = 1; j <= 15; j++)
					if (board.plate[i][j] == 3)
						ava.push_back(make_pair(i, j));
		}
		avail = ava;
	}

	short get_winner(pair<short, short> mov)
	{
		short x = mov.second, y = mov.first;
		short l1 = 0, l2 = 0, l3 = 0, l4 = 0;

		for (short i = 1; i <= 15 - x; i++)
		{
			if (plate[y][x] == plate[y][x + i])
				l1++;
			else
				break;
		}
		for (short i = 1; i <= x - 1; i++)
		{
			if (plate[y][x] == plate[y][x - i])
				l1++;
			else
				break;
		}
		if (l1 >= 4)
			return plate[y][x];
		for (short i = 1; i <= min(15 - y, 15 - x); i++)
		{
			if (plate[y][x] == plate[y + i][x + i])
				l2++;
			else
				break;
		}
		for (short i = 1; i <= min(y - 1, x - 1); i++)
		{
			if (plate[y][x] == plate[y - i][x - i])
				l2++;
			else
				break;
		}
		if (l2 >= 4)
			return plate[y][x];
		for (short i = 1; i <= 15 - y; i++)
		{
			if (plate[y][x] == plate[y + i][x])
				l3++;
			else
				break;
		}
		for (short i = 1; i <= y - 1; i++)
		{
			if (plate[y][x] == plate[y - i][x])
				l3++;
			else
				break;
		}
		if (l3 >= 4)
			return plate[y][x];
		for (short i = 1; i <= min(x - 1, 15 - y); i++)
		{
			if (plate[y][x] == plate[y + i][x - i])
				l4++;
			else
				break;
		}
		for (short i = 1; i <= min(15 - x, y - 1); i++)
		{
			if (plate[y][x] == plate[y - i][x + i])
				l4++;
			else
				break;
		}
		if (l4 >= 4)
			return plate[y][x];
		return 0;
	}
};

struct MCTSBoard : Board 
{
	vector <short> vplate[16], _vplate[16], lplate[16], _lplate[16]; //Value Plate

	void init(short ply)
	{
		str_ply = cur_ply = ply;
		lst_mov = make_pair(0, 0);
		
		for (short i = 1; i <= 15; i++)
		{
			plate[i].resize(16);
			vplate[i].resize(16, 1);
			_vplate[i].resize(16, 1);
			lplate[i].resize(16, 3);
			_lplate[i].resize(16, 3);
			for (short j = 1; j <= 15; j++)
				avail.push_back(make_pair(i, j));
		}
	}

	void move(pair<short, short> mov)
	{
		lst_mov = mov;
		plate[mov.first][mov.second] = cur_ply;
		avail.remove(mov);
		moves.push_back(mov);
		cur_ply = -cur_ply;
		upload();
	}

	void upload()
	{
		short x = lst_mov.second, y = lst_mov.first;

		for (short j = 1; j <= 2; j++)
		{
			for (short i = 1; i <= 4; i++)
			{
				if (x + i <= 15)
					if (plate[y][x + i] == 0) 
						eva(y, x + i, 1);
				if (x - i >= 1)
					if (plate[y][x - i] == 0) 
						eva(y, x - i, 1);
				if (x + i <= 15 && y + i <= 15)
					if (plate[y + i][x + i] == 0) 
						eva(y + i, x + i, 2);
				if (x - i >= 1 && y - i >= 1)
					if (plate[y - i][x - i] == 0) 
						eva(y - i, x - i, 2);
				if (y + i <= 15)
					if (plate[y + i][x] == 0) 
						eva(y + i, x, 3);
				if (y - i >= 1)
					if (plate[y - i][x] == 0) 
						eva(y - i, x, 3);
				if (x + i <= 15 && y - i >= 1)
					if (plate[y - i][x + i] == 0) 
						eva(y - i, x + i, 4);
				if (x - i >= 1 && y + i <= 15)
					if (plate[y + i][x - i] == 0) 
						eva(y + i, x - i, 4);
			}
			cur_ply = -cur_ply;
		}
	}

	bool ref_val()
	{
		short cur_val = 3, opp_val = 3;
		if (cur_ply == 1)
		{
			for (short i = 1; i <= 15; i++)
				for (short j = 1; j <= 15; j++)
					if (plate[i][j] == 0)
					{
						cur_val = min(_lplate[i][j], cur_val);
						opp_val = min(lplate[i][j], opp_val);
					}
		}
		else
		{
			for (short i = 1; i <= 15; i++)
				for (short j = 1; j <= 15; j++)
					if (plate[i][j] == 0)
					{
						cur_val = min(lplate[i][j], cur_val);
						opp_val = min(_lplate[i][j], opp_val);
					}
		}
		if (cur_val == 1) 
			return true;
		if (cur_val == 2)
		{
			if (opp_val == 1) 
				return false;
			else 
				return true;
		}
		return false;
	}

	void eva(short y, short x, short d)
	{
		short lx = lst_mov.second, ly = lst_mov.first;
		short v = get_val(y, x, d), lp = plate[ly][lx];
		plate[ly][lx] = 0;
		v -= get_val(y, x, d);
		plate[ly][lx] = lp;
		if (cur_ply == 1) 
			vplate[y][x] += v;
		else 
			_vplate[y][x] += v;

		if (cur_ply == -1) 
			lplate[y][x] = greedy(y, x);
		else 
			_lplate[y][x] = greedy(y, x);
	}

	short get_val(short y, short x, short d) // Opposite Direction
	{
		short l = 0, c = 0;
		if (d == 1)
		{
			for (short i = 1; i <= 4; i++)
			{
				if (x + i <= 15)
				{
					if (plate[y][x + i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y][x + i] == cur_ply) c++;
					}
				}
				else 
					break;
			}
			for (short i = 1; i <= 4; i++)
			{
				if (x - i >= 1)
				{
					if (plate[y][x - i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y][x - i] == cur_ply) c++;
					}
				}
				else 
					break;
			}
		}
		else if (d == 2)
		{
			for (short i = 1; i <= 4; i++)
			{
				if (x + i <= 15 && y + i <= 15)
				{
					if (plate[y + i][x + i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y + i][x + i] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
			for (short i = 1; i <= 4; i++)
			{
				if (x - i >= 1 && y - i >= 1)
				{
					if (plate[y - i][x - i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y - i][x - i] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
		}
		else if (d == 3)
		{
			for (short i = 1; i <= 4; i++)
			{
				if (y + i <= 15)
				{
					if (plate[y + i][x] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y + i][x] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
			for (short i = 1; i <= 4; i++)
			{
				if (y - i >= 1)
				{
					if (plate[y - i][x] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y - i][x] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
		}
		else
		{
			for (short i = 1; i <= 4; i++)
			{
				if (x + i <= 15 && y - i >= 1)
				{
					if (plate[y - i][x + i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y - i][x + i] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
			for (short i = 1; i <= 4; i++)
			{
				if (x - i >= 1 && y + i <= 15)
				{
					if (plate[y + i][x - i] == -cur_ply) 
						break;
					else
					{
						l++;
						if (plate[y + i][x - i] == cur_ply) 
							c++;
					}
				}
				else 
					break;
			}
		}
		if (l <= 3) 
			return 1;
		return 1 << c;
	}

	short greedy(short y, short x)
	{
		short l1 = 0, l2 = 0, l3 = 0, l4 = 0, l5 = 0, l6 = 0, l7 = 0, l8 = 0, l[5] = {0, 0, 0, 0, 0};
		short b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0, b8 = 0, t = 0;

		if (x == 15) 
			b1++;
		for (short i = 1; i <= 15 - x; i++)
		{
			if (plate[y][x + i] == -cur_ply)
			{
				b1++;
				break;
			}
			else if (plate[y][x + i] == cur_ply)
			{
				if (i == 15 - x) b1++;
				l1++;
			}
			else 
				break;
		}
		if (x == 1) 
			b2++;
		for (short i = 1; i <= x - 1; i++)
		{
			if (plate[y][x - i] == -cur_ply)
			{
				b2++;
				break;
			}
			else if (plate[y][x - i] == cur_ply)
			{
				if (i == x - 1) b2++;
				l2++;
			}
			else break;
		}
		if (l1 + l2 >= 4) return 1;
		if (l1 + l2 == 3 && b1 + b2 == 0) t++;
		if (l1 + l2 == 3 && b1 + b2 == 1) l[1]++;
		if (x >= 4 && x <= 12)
			if (plate[y][x + 1] == cur_ply && plate[y][x + 2] == 0 && plate[y][x + 3] == cur_ply
				&& plate[y][x - 1] == cur_ply && plate[y][x - 2] == 0 && plate[y][x - 3] == cur_ply) t++;
		if (l1 + l2 == 2 && b1 + b2 == 0)
		{
			if (x + l1 + 2 <= 15) if (plate[y][x + l1 + 2] == 0) l[1]++;
			if (x - l2 - 2 >= 1) if (plate[y][x - l2 - 2] == 0) l[1]++;
		}
		if (x <= 12 && x >= 3)
			if (plate[y][x + 1] == 0 && plate[y][x + 2] == cur_ply && plate[y][x + 3] == 0 && plate[y][x - 1] == cur_ply && plate[y][x - 2] == 0) l[1]++;
		if (x <= 13 && x >= 4)
			if (plate[y][x - 1] == 0 && plate[y][x - 2] == cur_ply && plate[y][x - 3] == 0 && plate[y][x + 1] == cur_ply && plate[y][x + 2] == 0) l[1]++;
		if (x <= 13 && x >= 3)
		{
			if (plate[y][x + 1] == cur_ply && plate[y][x + 2] == cur_ply && plate[y][x - 1] == 0 && plate[y][x - 2] == cur_ply) l[1]++;
			if (plate[y][x - 1] == cur_ply && plate[y][x - 2] == cur_ply && plate[y][x + 1] == 0 && plate[y][x + 2] == cur_ply) l[1]++;
		}
		if (x <= 12 && x >= 2)
		{
			if (plate[y][x + 1] == 0 && plate[y][x + 2] == cur_ply && plate[y][x + 3] == cur_ply && plate[y][x - 1] == cur_ply) l[1]++;
			if (plate[y][x + 1] == cur_ply && plate[y][x + 2] == 0 && plate[y][x + 3] == cur_ply && plate[y][x - 1] == cur_ply) l[1]++;
		}
		if (x <= 14 && x >= 4)
		{
			if (plate[y][x - 1] == 0 && plate[y][x - 2] == cur_ply && plate[y][x - 3] == cur_ply && plate[y][x + 1] == cur_ply) l[1]++;
			if (plate[y][x - 1] == cur_ply && plate[y][x - 2] == 0 && plate[y][x - 3] == cur_ply && plate[y][x + 1] == cur_ply) l[1]++;
		}
		if (x <= 11)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y][x + i] == cur_ply) p++;
				else if (plate[y][x + i] == 0) q++;
			}
			if (p == 3 && q == 1) l[1]++;
			if (p == 2 && q == 2 && !b2 && plate[y][x + 4] == 0) l[1]++;
		}
		if (x >= 5)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y][x - i] == cur_ply) p++;
				else if (plate[y][x - i] == 0) q++;
			}
			if (p == 3 && q == 1) l[1]++;
			if (p == 2 && q == 2 && !b1 && plate[y][x - 4] == 0) l[1]++;
		}


		if (y == 15 || x == 15) b3++;
		for (short i = 1; i <= min(15 - y, 15 - x); i++)
		{
			if (plate[y + i][x + i] == -cur_ply)
			{
				b3++;
				break;
			}
			else if (plate[y + i][x + i] == cur_ply)
			{
				if (i == min(15 - y, 15 - x)) b3++;
				l3++;
			}
			else break;
		}
		if (y == 1 || x == 1) b4++;
		for (short i = 1; i <= min(y - 1, x - 1); i++)
		{
			if (plate[y - i][x - i] == -cur_ply)
			{
				b4++;
				break;
			}
			else if (plate[y - i][x - i] == cur_ply)
			{
				if (i == min(y - 1, x - 1)) b4++;
				l4++;
			}
			else break;
		}
		if (l3 + l4 >= 4) return 1;
		if (l3 + l4 == 3 && b3 + b4 == 0) t++;
		if (l3 + l4 == 3 && b3 + b4 == 1) l[2]++;
		if (x >= 4 && x <= 12 && y >= 4 && y <= 12)
			if (plate[y + 1][x + 1] == cur_ply && plate[y + 2][x + 2] == 0 && plate[y + 3][x + 3] == cur_ply
				&& plate[y - 1][x - 1] == cur_ply && plate[y - 2][x - 2] == 0 && plate[y - 3][x - 3] == cur_ply) t++;
		if (l3 + l4 == 2 && b3 + b4 == 0)
		{
			if (max(x, y) + l3 + 2 <= 15) if (plate[y + l3 + 2][x + l3 + 2] == 0) l[2]++;
			if (min(x, y) - l4 - 2 >= 1) if (plate[y - l4 - 2][x - l4 - 2] == 0) l[2]++;
		}
		if (x <= 12 && x >= 3 && y <= 12 && y >= 3)
			if (plate[y + 1][x + 1] == 0 && plate[y + 2][x + 2] == cur_ply && plate[y + 3][x + 3] == 0 && plate[y - 1][x - 1] == cur_ply && plate[y - 2][x - 2] == 0) l[2]++;
		if (x <= 13 && x >= 4 && y <= 13 && y >= 4)
			if (plate[y - 1][x - 1] == 0 && plate[y - 2][x - 2] == cur_ply && plate[y - 3][x - 3] == 0 && plate[y + 1][x + 1] == cur_ply && plate[y + 2][x + 2] == 0) l[2]++;
		if (x <= 13 && x >= 3 && y <= 13 && y >= 3)
		{
			if (plate[y + 1][x + 1] == cur_ply && plate[y + 2][x + 2] == cur_ply && plate[y - 1][x - 1] == 0 && plate[y - 2][x - 2] == cur_ply) l[2]++;
			if (plate[y - 1][x - 1] == cur_ply && plate[y - 2][x - 2] == cur_ply && plate[y + 1][x + 1] == 0 && plate[y + 2][x + 2] == cur_ply) l[2]++;
		}
		if (x <= 12 && x >= 2 && y <= 12 && y >= 2)
		{
			if (plate[y + 1][x + 1] == 0 && plate[y + 2][x + 2] == cur_ply && plate[y + 3][x + 3] == cur_ply && plate[y - 1][x - 1] == cur_ply) l[2]++;
			if (plate[y + 1][x + 1] == cur_ply && plate[y + 2][x + 2] == 0 && plate[y + 3][x + 3] == cur_ply && plate[y - 1][x - 1] == cur_ply) l[2]++;
		}
		if (x <= 14 && x >= 4 && y <= 14 && y >= 4)
		{
			if (plate[y - 1][x - 1] == 0 && plate[y - 2][x - 2] == cur_ply && plate[y - 3][x - 3] == cur_ply && plate[y + 1][x + 1] == cur_ply) l[2]++;
			if (plate[y - 1][x - 1] == cur_ply && plate[y - 2][x - 2] == 0 && plate[y - 3][x - 3] == cur_ply && plate[y + 1][x + 1] == cur_ply) l[2]++;
		}
		if (x <= 11 && y <= 11)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y + i][x + i] == cur_ply) p++;
				else if (plate[y + i][x + i] == 0) q++;
			}
			if (p == 3 && q == 1) l[2]++;
			if (p == 2 && q == 2 && !b4 && plate[y + 4][x + 4] == 0) l[2]++;
		}
		if (x >= 5 && y >= 5)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y - i][x - i] == cur_ply) p++;
				else if (plate[y - i][x - i] == 0) q++;
			}
			if (p == 3 && q == 1) l[2]++;
			if (p == 2 && q == 2 && !b3 && plate[y - 4][x - 4] == 0) l[2]++;
		}


		if (y == 15) b5++;
		for (short i = 1; i <= 15 - y; i++)
		{
			if (plate[y + i][x] == -cur_ply)
			{
				b5++;
				break;
			}
			else if (plate[y + i][x] == cur_ply)
			{
				if (i == 15 - y) b5++;
				l5++;
			}
			else break;
		}
		if (y == 1) b6++;
		for (short i = 1; i <= y - 1; i++)
		{
			if (plate[y - i][x] == -cur_ply)
			{
				b6++;
				break;
			}
			else if (plate[y - i][x] == cur_ply)
			{
				if (i == y - 1) b6++;
				l6++;
			}
			else break;
		}
		if (l5 + l6 >= 4) return 1;
		if (l5 + l6 == 3 && b5 + b6 == 0) t++;
		if (l5 + l6 == 3 && b5 + b6 == 1) l[3]++;
		if (y >= 4 && y <= 12)
			if (plate[y + 1][x] == cur_ply && plate[y + 2][x] == 0 && plate[y + 3][x] == cur_ply
				&& plate[y - 1][x] == cur_ply && plate[y - 2][x] == 0 && plate[y - 3][x] == cur_ply) t++;
		if (l5 + l6 == 2 && b5 + b6 == 0)
		{
			if (y + l5 + 2 <= 15) if (plate[y + l5 + 2][x] == 0) l[3]++;
			if (y - l6 - 2 >= 1) if (plate[y - l6 - 2][x] == 0) l[3]++;
		}
		if (y <= 12 && y >= 3)
			if (plate[y + 1][x] == 0 && plate[y + 2][x] == cur_ply && plate[y + 3][x] == 0 && plate[y - 1][x] == cur_ply && plate[y - 2][x] == 0) l[3]++;
		if (y <= 13 && y >= 4)
			if (plate[y - 1][x] == 0 && plate[y - 2][x] == cur_ply && plate[y - 3][x] == 0 && plate[y + 1][x] == cur_ply && plate[y + 2][x] == 0) l[3]++;
		if (y <= 13 && y >= 3)
		{
			if (plate[y + 1][x] == cur_ply && plate[y + 2][x] == cur_ply && plate[y - 1][x] == 0 && plate[y - 2][x] == cur_ply) l[3]++;
			if (plate[y - 1][x] == cur_ply && plate[y - 2][x] == cur_ply && plate[y + 1][x] == 0 && plate[y + 2][x] == cur_ply) l[3]++;
		}
		if (y <= 12 && y >= 2)
		{
			if (plate[y + 1][x] == 0 && plate[y + 2][x] == cur_ply && plate[y + 3][x] == cur_ply && plate[y - 1][x] == cur_ply) l[3]++;
			if (plate[y + 1][x] == cur_ply && plate[y + 2][x] == 0 && plate[y + 3][x] == cur_ply && plate[y - 1][x] == cur_ply) l[3]++;
		}
		if (y <= 14 && y >= 4)
		{
			if (plate[y - 1][x] == 0 && plate[y - 2][x] == cur_ply && plate[y - 3][x] == cur_ply && plate[y + 1][x] == cur_ply) l[3]++;
			if (plate[y - 1][x] == cur_ply && plate[y - 2][x] == 0 && plate[y - 3][x] == cur_ply && plate[y + 1][x] == cur_ply) l[3]++;
		}
		if (y <= 11)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y + i][x] == cur_ply) p++;
				else if (plate[y + i][x] == 0) q++;
			}
			if (p == 3 && q == 1) l[3]++;
			if (p == 2 && q == 2 && !b6 && plate[y + 4][x] == 0) l[3]++;
		}
		if (y >= 5)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y - i][x] == cur_ply) p++;
				else if (plate[y - i][x] == 0) q++;
			}
			if (p == 3 && q == 1) l[3]++;
			if (p == 2 && q == 2 && !b5 && plate[y - 4][x] == 0) l[3]++;
		}


		if (y == 15 || x == 1) b7++;
		for (short i = 1; i <= min(x - 1, 15 - y); i++)
		{
			if (plate[y + i][x - i] == -cur_ply)
			{
				b7++;
				break;
			}
			else if (plate[y + i][x - i] == cur_ply)
			{
				if (i == min(x - 1, 15 - y)) b7++;
				l7++;
			}
			else break;
		}
		if (y == 1 || x == 15) b8++;
		for (short i = 1; i <= min(15 - x, y - 1); i++)
		{
			if (plate[y - i][x + i] == -cur_ply)
			{
				b8++;
				break;
			}
			else if (plate[y - i][x + i] == cur_ply)
			{
				if (i == min(15 - x, y - 1)) b8++;
				l8++;
			}
			else break;
		}
		if (l7 + l8 >= 4) return 1;
		if (l7 + l8 == 3 && b7 + b8 == 0) t++;
		if (l7 + l8 == 3 && b7 + b8 == 1) l[4]++;
		if (x >= 4 && x <= 12 && y >= 4 && y <= 12)
			if (plate[y + 1][x - 1] == cur_ply && plate[y + 2][x - 2] == 0 && plate[y + 3][x - 3] == cur_ply
				&& plate[y - 1][x + 1] == cur_ply && plate[y - 2][x + 2] == 0 && plate[y - 3][x + 3] == cur_ply) t++;
		if (l7 + l8 == 2 && b7 + b8 == 0)
		{
			if (y + l7 + 2 <= 15 && x - l7 - 2 >= 1) if (plate[y + l7 + 2][x - l7 - 2] == 0) l[4]++;
			if (y - l8 - 2 >= 1 && x + l8 + 2 <= 15) if (plate[y - l8 - 2][x + l8 + 2] == 0) l[4]++;
		}
		if (x <= 12 && x >= 3 && y <= 13 && y >= 4)
			if (plate[y - 1][x + 1] == 0 && plate[y - 2][x + 2] == cur_ply && plate[y - 3][x + 3] == 0 && plate[y + 1][x - 1] == cur_ply && plate[y + 2][x - 2] == 0) l[4]++;
		if (x <= 13 && x >= 4 && y <= 12 && y >= 3)
			if (plate[y + 1][x - 1] == 0 && plate[y + 2][x - 2] == cur_ply && plate[y + 3][x - 3] == 0 && plate[y - 1][x + 1] == cur_ply && plate[y - 2][x + 2] == 0) l[4]++;
		if (x <= 13 && x >= 3 && y <= 13 && y >= 3)
		{
			if (plate[y - 1][x + 1] == cur_ply && plate[y - 2][x + 2] == cur_ply && plate[y + 1][x - 1] == 0 && plate[y + 2][x - 2] == cur_ply) l[4]++;
			if (plate[y + 1][x - 1] == cur_ply && plate[y + 2][x - 2] == cur_ply && plate[y - 1][x + 1] == 0 && plate[y - 2][x + 2] == cur_ply) l[4]++;
		}
		if (x <= 12 && x >= 2 && y <= 14 && y >= 4)
		{
			if (plate[y - 1][x + 1] == 0 && plate[y - 2][x + 2] == cur_ply && plate[y - 3][x + 3] == cur_ply && plate[y + 1][x - 1] == cur_ply) l[4]++;
			if (plate[y - 1][x + 1] == cur_ply && plate[y - 2][x + 2] == 0 && plate[y - 3][x + 3] == cur_ply && plate[y + 1][x - 1] == cur_ply) l[4]++;
		}
		if (x <= 14 && x >= 4 && y <= 12 && y >= 2)
		{
			if (plate[y + 1][x - 1] == 0 && plate[y + 2][x - 2] == cur_ply && plate[y + 3][x - 3] == cur_ply && plate[y - 1][x + 1] == cur_ply) l[4]++;
			if (plate[y + 1][x - 1] == cur_ply && plate[y + 2][x - 2] == 0 && plate[y + 3][x - 3] == cur_ply && plate[y - 1][x + 1] == cur_ply) l[4]++;
		}
		if (x >= 5 && y <= 11)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y + i][x - i] == cur_ply) p++;
				else if (plate[y + i][x - i] == 0) q++;
			}
			if (p == 3 && q == 1) l[4]++;
			if (p == 2 && q == 2 && !b8 && plate[y + 4][x - 4] == 0) l[4]++;
		}
		if (y >= 5 && x <= 11)
		{
			short p = 0, q = 0;
			for (short i = 1; i <= 4; i++)
			{
				if (plate[y - i][x + i] == cur_ply) p++;
				else if (plate[y - i][x + i] == 0) q++;
			}
			if (p == 3 && q == 1) l[4]++;
			if (p == 2 && q == 2 && !b7 && plate[y - 4][x + 4] == 0) l[4]++;
		}


		if (t) return 2;
		for (int i = 1; i <= 4; i++) if (l[i]) l[0]++;
		if (l[0] >= 2) return 2;

		return 3;
	}
};

struct CompressedBoard
{
	unsigned long long block[8] = { 0 };
	unsigned short lst_mov = 0, targ = 0;
	float val = 0;

	void encodeBoard(const vector<short> sit, pair<short, short> mov, pair<short, short> _targ, double _val)
	{
		for (int i = 0; i < 225; i++) 
		{
			int blockIndex = i / 64;
			int bitOffset = i % 64;

			if (sit[i] == 1) 
				block[blockIndex] |= (1ULL << bitOffset);

			if (sit[i + 225] == 1)
				block[blockIndex + 4] |= (1ULL << bitOffset);
		}
		lst_mov = (mov.first << 4) | mov.second;
		targ = (_targ.first << 4) | _targ.second;
		val = _val;
	}

	auto decodeBoard() 
	{
		vector<short> sit(675, 0);

		for (int i = 0; i < 225; i++) 
		{
			int blockIndex = i / 64;
			int bitOffset = i % 64;

			sit[i] = (block[blockIndex] & (1ULL << bitOffset)) ? 1 : 0;

			sit[i + 225] = (block[blockIndex + 4] & (1ULL << bitOffset)) ? 1 : 0;
		
		}

		short y = (lst_mov >> 4) & 0xF, x = lst_mov & 0xF;
		if (y != 0)
			sit[(y - 1) * 15 + x - 1 + 450] = 1;
		pair<short, short> _targ = make_pair((targ >> 4) & 0xF, targ & 0xF);

		return make_tuple(sit, _targ, val);
	}
};