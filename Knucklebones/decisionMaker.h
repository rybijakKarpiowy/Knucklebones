#pragma once
#include <cmath>
#include <list>
#include <iostream>

using namespace std;

class decisionMaker
{
public:
	int makeDecision(int myBoard, int enemyBoard, short dice)
	{
		// implement UCT (Upper Confidence Bounds applied to Trees)
		// https://en.wikipedia.org/wiki/Monte_Carlo_tree_search

		// 0. Initialization
		node root = { true, nullptr, {nullptr}, 0, 0, 1, 1 };
		//std::cout << "Initialiation done\n";

		// for each iteration
		for (int _ = 0; _ < 200; _++)
		{
			// 1. Selection
			node* selectedNode = &root;
			while (!selectedNode->isLeaf)
			{
				selectedNode = selectNode(*selectedNode);
			}
			//std::cout << "Selection done in iteration " << _ << "\n";

			// 2. Expansion
			expandNode(*selectedNode);
			//std::cout << "Expansion done in iteration " << _ << "\n";

			// 3. Simulation
			// a number between -1 and 1, -1 means losing, 1 means winning
			//std::cout << "Simulation started in iteration " << _ << "\n";
			float winRate = playout(selectedNode, myBoard, enemyBoard, dice, 50); // playout has some problems ://
			//std::cout << "Simulation done in iteration " << _ << "\n";

			// 4. Backpropagation
			// update the winRate and simulationCount of the selectedNode and all its parents
			selectedNode->winCount += winRate;
			selectedNode->simulationCount++;
			node* currentNode = selectedNode;
			while (currentNode->parent != nullptr)
			{
				currentNode = currentNode->parent;
				currentNode->winCount += selectedNode->winCount;
				currentNode->simulationCount++;
			}
			//std::cout << "Backpropagation done in iteration " << _ << "\n";
		}

		// 5. Action
		// select the root's child with the highest winRate
		// if there are multiple children with the same winRate, select the one with the highest simulationCount
		short bestChild = -1;
		float bestWinRate = -INFINITY;
		short bestSimulationCount = 0;
		// root node has at most 3 children
		for (short i = 0; i < 3; i++)
		{
			std::cout << "Win rate of child " << i << " is " << root.children[i]->winCount << "\n";
			if (root.children[i] == nullptr)
			{
				// no children
				continue;
			}

			if (root.children[i]->winCount > bestWinRate)
			{
				std::cout<< "Child " << i << " is better than the previous best child\n";
				bestChild = i;
				bestWinRate = root.children[i]->winCount;
				bestSimulationCount = root.children[i]->simulationCount;
			}
			else if (root.children[i]->winCount == bestWinRate)
			{
				if (root.children[i]->simulationCount > bestSimulationCount)
				{
					bestChild = i;
					bestWinRate = root.children[i]->winCount;
					bestSimulationCount = root.children[i]->simulationCount;
				}
			}
		}

		// return the index of chosen column, that is bestChild (0, 1 or 2)
		return bestChild;
	}

private:
	struct state
	{
		int myBoard;
		int enemyBoard;
		short dice;
	};

	struct node
	{
		bool isLeaf;
		node* parent;
		node* children[3];
		float winCount;
		short simulationCount;
		bool isLeft;
		bool isRight;
	};

	// returns 3bit number, 0 bit is the first column, 1 bit is the second column, 2 bit is the third column (bit number = pos)
	short DMgetPossibleMoves(int& board)
	{
		short possibleMoves = 0;
		// check if first column has free space
		if ((board & 0x1FF) < 0x40)
		{
			possibleMoves = possibleMoves | 0x1;
		}
		// check if second column has free space
		if (((board >> 9) & 0x1FF) < 0x40)
		{
			possibleMoves = possibleMoves | 0x2;
		}
		// check if third column has free space
		if (((board >> 18) & 0x1FF) < 0x40)
		{
			possibleMoves = possibleMoves | 0x4;
		}
		return possibleMoves;
	}
	short DMgetScore(int& board)
	{
		short score = 0;
		// check numbers in the first column
		for (int i = 0; i < 3; i++)
		{
			short first, second, third;
			first = (board >> (i * 9)) & 0x7;
			second = (board >> (i * 9 + 3)) & 0x7;
			third = (board >> (i * 9 + 6)) & 0x7;
			if (first == second)
			{
				if (second == third)
				{
					// all three are equal
					score += first * first * first;
				}
				else
				{

					// first two are equal
					score += first * first + third;
				}
				continue;
			}
			if (second == third)
			{
				// last two are equal
				score += second * second + first;
				continue;
			}
			if (first == third)
			{
				// first and last are equal
				score += first * first + second;
				continue;
			}
			// no two are equal
			score += first + second + third;
		}

		return score;
	}
	// clears the specified column from specified dice and shifts the rest of the dice down
	int DMclearColumn(short dice, short pos, int board)
	{
		short extractedPos = (board >> (pos * 9)) & 0x1FF;
		// start from the third row and go up
		for (int i = 2; i >= 0; i--)
		{
			short extractedDice = (extractedPos >> (i * 3)) & 0x7;

			if (extractedDice != dice)
			{
				continue;
			}

			// else, clear the dice
			extractedPos = extractedPos & ~(0x7 << (i * 3));
			// shift the rest of the dice down
			if (i == 1)
			{
				// we removed the second dice, so we need to shift the third dice to the second place
				// (extractedPos >> 3) is the third dice on second place, since extractedPos second place is 0
				// we use | to put the third dice on second place
				// we use & 0x3F to clear the third dice from third place
				extractedPos = (extractedPos | (extractedPos >> 3)) & 0x3F;
				continue;
			}
			if (i == 0)
			{
				// if we remove the first dice, we need to shift the third and second dice to second and first place respectively
				// in fact, we can just shift the whole extractedPos by 3 bits to the right
				extractedPos = extractedPos >> 3;
			}
			//else: that is if (i == 2) we removed the last dice, so we don't need to shift anything

		}
		// put the new extractedPos back in the board
		board = board & ~(0x1FF << (pos * 9));
		board = board | (extractedPos << (pos * 9));

		return board;
	}
	// returns -1 if column is full, 0 if dice is put in the first row, 1 if dice is put in the second row, 2 if dice is put in the third row
	int DMappendDice(short dice, short pos, int board) // pos is 0, 1 or 2
	{
		// check if pos has free space
		short extractedPos = (board >> (pos * 9)) & 0x1FF;
		// if extractedPos < 0x40, then there is space in the last row
		// if extractedPos < 8, then there is space in the second row
		// if extractedPos == 0 then there is space in the first row (column is empty)
		if (extractedPos == 0)
		{
			// put dice in the first row of the column
			board = board | (int)(dice << (pos * 9));
			//return 0;
		}
		else if (extractedPos < 0x8)
		{
			// put dice in the second row of the column
			board = board | (int)(dice << (pos * 9 + 3));
			//return 1;
		}
		else if (extractedPos < 0x40)
		{
			// put dice in the third row of the column
			board = board | (int)(dice << (pos * 9 + 6));
			//return 2;
		}
		//else
		//{
		//	// column is full
		//	return -1;
		//}
		return board;
	}

	void expandNode(node& nodeExpanded)
	{
		// for each possible move, create a child node
		nodeExpanded.children[0] = new node{ true, &nodeExpanded, {nullptr}, 0, 0, 1, 0 };
		nodeExpanded.children[1] = new node{ true, &nodeExpanded, {nullptr}, 0, 0, 0, 0 };
		nodeExpanded.children[2] = new node{ true, &nodeExpanded, {nullptr}, 0, 0, 0, 1 };
		nodeExpanded.isLeaf = false;
	}

	node* selectNode(node& parent)
	{
		if (parent.isLeaf)
		{
			return &parent;
		}

		// select the child with the highest UCT value
		// UCT = winRate + C * sqrt(ln(parentSimulationCount) / childSimulationCount)
		// C is a constant, usually 1.414
		// winRate is the winCount / simulationCount
		// parentSimulationCount is the simulationCount of the parent node
		// childSimulationCount is the simulationCount of the child node
		// if there are multiple children with the same UCT value, select the one with the highest simulationCount
		short bestChild = 0;
		double bestUCT = 0;
		short bestSimulationCount = 0;
		// parent node has 3 children
		for (short i = 0; i < 3; i++)
		{
			if (parent.children[i] == nullptr)
			{
				// no children
				continue;
			}

			short childSimulationCount = parent.children[i]->simulationCount;
			double UCT = (double)(parent.children[i]->winCount / childSimulationCount) + 1.414 * sqrt(log(parent.simulationCount) / childSimulationCount);

			if (UCT > bestUCT)
			{
				bestChild = i;
				bestUCT = UCT;
				bestSimulationCount = childSimulationCount;
			}
			else if (UCT == bestUCT)
			{
				if (childSimulationCount > bestSimulationCount)
				{
					bestChild = i;
					bestUCT = UCT;
					bestSimulationCount = childSimulationCount;
				}
			}
		}

		// return the selected child
		return parent.children[bestChild];
	}

	// return the winRate, if some move is winning, return 1, if some move is losing, return -1, on draw return 0, if a move is invalid, return -1
	float playout(node* selectedNode, int& myBoard, int& enemyBoard, short inputDice, short iterations)
	{
		// get a list of moves
		list<short> movesList;
		node* parent = selectedNode->parent;
		while (parent != nullptr)
		{
			if (parent->isLeft)
			{
				movesList.push_front(0);
			}
			else if (parent->isRight)
			{
				movesList.push_front(2);
			}
			else
			{
				movesList.push_front(1);
			}
			parent = parent->parent;
		}

		float winCount = 0;
		// for each iteration
		for (int _ = 0; _ < iterations; _++)
		{
			// Copy boards
			int myBoardCopy = myBoard;
			int enemyBoardCopy = enemyBoard;
			short dice = inputDice;

			// for each move in the list, perform the move and perform a random move for the enemy
			for (auto const& move : movesList)
			{
				// check if move is possible
				short possibleMoves = DMgetPossibleMoves(myBoardCopy);
				if ((possibleMoves & (1 << move)) == 0)
				{
					// move is not possible, add -1 to the winCount of the selectedNode
					winCount--;
					break;
				}

				// move is possible, perform the move
				DMappendDice(dice, move, myBoardCopy);
				DMclearColumn(dice, move, enemyBoardCopy);
				// check if the game is over
				if (!(DMgetPossibleMoves(myBoardCopy) && DMgetPossibleMoves(enemyBoardCopy)))
				{
					// game is over, check if we won
					if (DMgetScore(myBoardCopy) > DMgetScore(enemyBoardCopy))
					{
						// we won, add 1 to the winCount of the selectedNode
						winCount++;
					}
					else if (DMgetScore(myBoardCopy) < DMgetScore(enemyBoardCopy))
					{
						// we lost, add -1 to the winCount of the selectedNode
						winCount--;
					}
					// else: draw, do nothing
					break;
				}

				// perform a random move for the enemy
				dice = rand() % 6 + 1;
				possibleMoves = DMgetPossibleMoves(enemyBoardCopy);
				short enemyMove = rand() % 3;
				while ((possibleMoves & (1 << enemyMove)) == 0)
				{
					// move is not possible, reroll the move
					enemyMove = rand() % 3;
				}

				// move is possible, perform the move
				DMappendDice(dice, enemyMove, enemyBoardCopy);
				DMclearColumn(dice, enemyMove, myBoardCopy);
				// check if the game is over
				if (!(DMgetPossibleMoves(myBoardCopy) && DMgetPossibleMoves(enemyBoardCopy)))
				{
					// game is over, check if we won
					if (DMgetScore(myBoardCopy) > DMgetScore(enemyBoardCopy))
					{
						// we won, add 1 to the winCount of the selectedNode
						winCount++;
					}
					else if (DMgetScore(myBoardCopy) < DMgetScore(enemyBoardCopy))
					{
						// we lost, add -1 to the winCount of the selectedNode
						winCount--;
					}
					// else: draw, do nothing
					break;
				}
			}
			// reroll the dice
			dice = rand() % 6 + 1;
		}

		// return the winRate
		return winCount / iterations;
	}
};