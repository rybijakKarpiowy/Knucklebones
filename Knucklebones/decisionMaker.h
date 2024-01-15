#pragma once
#include <cmath>

using namespace std;

class decisionMaker
{
public:
	int makeDecision(int myBoard, int enemyBoard, short dice)
	{
		// implement UCT (Upper Confidence Bounds applied to Trees)
		// https://en.wikipedia.org/wiki/Monte_Carlo_tree_search

		// 0. Initialization
		node root = { { myBoard, enemyBoard, dice, true }, true, nullptr, {nullptr}, 0, 1, 0 };


		// for each iteration
		for (int _ = 0; _ < 10; _++)
		{
			// 1. Selection
			node* selectedNode = &root;
			while (!selectedNode->isLeaf)
			{
				selectedNode = selectNode(*selectedNode);
			}

			// 2. Expansion
			expandNode(*selectedNode);

			// 3. Simulation
			// simulation is done in the expandNode function (the score is calculated there)
			// the score is just a comparison between the two boards, so it is a number 0 or 1
			// more advanced simulation can be done here

			// 4. Backpropagation
			// update the winRate and simulationCount of the selectedNode and all its parents
			node* currentNode = selectedNode;
			while (currentNode != nullptr)
			{
				currentNode->winCount += selectedNode->winCount;
				currentNode->simulationCount++;
				currentNode = currentNode->parent;
			}
		}

		// 5. Action
		// select the root's child with the highest winRate
		// if there are multiple children with the same winRate, select the one with the highest simulationCount
		short bestChild = 0;
		short bestWinRate = 0;
		short bestSimulationCount = 0;
		// root node has at most 3 children
		for (short i = 0; i < root.childCount; i++)
		{
			if (root.children[i] == nullptr)
			{
				// no children
				continue;
			}

			if (root.children[i]->winCount > bestWinRate)
			{
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
		bool myTurn;
	};

	struct node
	{
		state value;
		bool isLeaf;
		node* parent;
		node* children[18];
		short winCount;
		short simulationCount;
		short childCount;
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
		// for each possible dice roll, for each possible move, create a child node
		short possibleMoves = DMgetPossibleMoves(nodeExpanded.value.myBoard);
		bool myTurn = nodeExpanded.value.myTurn;
		for (short i = 0; i < 3; i++)
		{
			// check if move is possible
			if ((possibleMoves & (0x1 << i)) == 0)
			{
				// move is not possible
				continue;
			}

			// if parent == nullptr, then the node is the root node, therefore we know the dice roll (create only 3 children)
			if (nodeExpanded.parent == nullptr)
			{
				// myTurn is true, so the first board is myBoard
				int firstBoard = DMappendDice(nodeExpanded.value.dice, i, nodeExpanded.value.myBoard);
				int secondBoard = DMclearColumn(nodeExpanded.value.dice, i, nodeExpanded.value.enemyBoard);

				bool score = DMgetScore(firstBoard) - DMgetScore(secondBoard) > 0 ? 1 : 0;
				nodeExpanded.children[i] = new node{ { firstBoard, secondBoard, nodeExpanded.value.dice, false }, true, &nodeExpanded, {nullptr}, score, 1, 0 };
				nodeExpanded.childCount++;
				nodeExpanded.isLeaf = false;
				continue;
			}

			for (short j = 1; j < 7; j++)
			{
				// move is possible, perform it for every dice roll
				int firstBoard = DMappendDice(j, i, myTurn ? nodeExpanded.value.myBoard : nodeExpanded.value.enemyBoard);
				int secondBoard = DMclearColumn(j, i, myTurn ? nodeExpanded.value.enemyBoard : nodeExpanded.value.myBoard);
				// if myTurn is true, then the first board is myBoard
				// else, switch the boards
				if (myTurn)
				{
					bool score = DMgetScore(firstBoard) - DMgetScore(secondBoard) > 0 ? 1 : 0;
					nodeExpanded.children[i * 6 + j - 1] = new node{ { firstBoard, secondBoard, j, false }, true, &nodeExpanded, {nullptr}, score, 1, 0 };
				}
				else
				{
					bool score = DMgetScore(secondBoard) - DMgetScore(firstBoard) > 0 ? 1 : 0;
					nodeExpanded.children[i * 6 + j - 1] = new node{ { secondBoard, firstBoard, j, true }, true, &nodeExpanded, {nullptr}, score, 1, 0 };
				}

				nodeExpanded.childCount++;
				nodeExpanded.isLeaf = false;
			}
		}
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
		float bestUCT = 0;
		short bestSimulationCount = 0;
		// parent node has at most 18 children
		for (short i = 0; i < parent.childCount; i++)
		{
			if (parent.children[i] == nullptr)
			{
				// no children
				continue;
			}

			short childSimulationCount = parent.children[i]->simulationCount;
			float UCT = (float)(parent.children[i]->winCount / childSimulationCount) + 1.414 * sqrt(log(parent.simulationCount) / childSimulationCount);

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
};