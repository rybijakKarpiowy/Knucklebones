#include <iostream>

class PlayerBoard
{
public:
	// board consists of 32 bits, 0-8 are first column, 9-17 are second column, 18-26 are third column, 27-31 are empty
	// dices are put in the less significant bits first
	int board;
	int rollDice()
	{
		int dice = rand() % 6 + 1;
		return dice;
	}
	// returns -1 if column is full, 0 if dice is put in the first row, 1 if dice is put in the second row, 2 if dice is put in the third row
	void appendDice(int dice, short pos) // pos is 0, 1 or 2
	{
		// check if pos has free space
		short extractedPos = (board >> (pos * 9)) & 0x1FF;
		// if extractedPos < 0x40, then there is space in the last row
		// if extractedPos < 8, then there is space in the second row
		// if extractedPos == 0 then there is space in the first row (column is empty)
		if (extractedPos == 0)
		{
			// put dice in the first row of the column
			board = board | (dice << (pos * 9));
			//return 0;
		}
		else if (extractedPos < 0x8)
		{
			// put dice in the second row of the column
			board = board | (dice << (pos * 9 + 3));
			//return 1;
		}
		else if (extractedPos < 0x40)
		{
			// put dice in the third row of the column
			board = board | (dice << (pos * 9 + 6));
			//return 2;
		}
		//else
		//{
		//	// column is full
		//	return -1;
		//}
	}
	// returns 3bit number, 0 bit is the first column, 1 bit is the second column, 2 bit is the third column (bit number = pos)
	short getPossibleMoves()
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
	short getScore()
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
	void clearColumn(int dice, short pos)
	{
		short extractedPos = (board >> (pos * 9)) & 0x1FF;
		// start from the third row and go up
		for (int i = 2; i >= 0; i--)
		{
			short extractedDice = (extractedPos >> (i * 3)) & 0x7;
			if (extractedDice == dice)
			{
				// clear the dice
				extractedPos = extractedPos & ~(0x7 << (i * 3));
				// shift the rest of the dice down
				if (i == 2)
				{
					// we removed the last dice, so we don't need to shift anything
					continue;
				}
				if (i == 1)
				{
					// we removed the second dice, so we need to shift the third dice to the second place
					// (extractedPos >> 3) is the third dice on second place, since extractedPos second place is 0
					// we use | to put the third dice on second place
					// we use & 0x3F to clear the third dice from third place
					extractedPos = (extractedPos | (extractedPos >> 3)) & 0x3F;
					continue;
				}
				// if we remove the first dice, we need to shift the third and second dice to second and first place respectively
				// in fact, we can just shift the whole extractedPos by 3 bits to the right
				extractedPos = extractedPos >> 3;
			}
		}
		// put the new extractedPos back in the board
		board = board & ~(0x1FF << (pos * 9));
		board = board | (extractedPos << (pos * 9));
	}
};

using namespace std;

bool isGameOn(PlayerBoard player1, PlayerBoard player2)
{
	// check if both players have possible moves
	return player1.getPossibleMoves() && player2.getPossibleMoves();
}

void printBoard(int board, bool reversed)
{
	// for each row, print the row, start from the first row, then the second, then the third
	// if reversed is true, print the board upside down
	for (int i = 0; i < 3; i++)
	{
		// print every dice in the i-th row
		for (int j = 0; j < 3; j++)
		{
			short extractedDice = (board >> (6 * reversed + i * 3 * (reversed ? -1 : 1) + j * 9)) & 0x7;
			cout << extractedDice << " ";
		}
		cout << endl;
	}
	cout << endl;
}

int main()
{
	PlayerBoard player1;
	PlayerBoard player2;
	player1.board = 0;
	player2.board = 0;

	bool player1Turn = true;
	// game loop
	while (true)
	{
		PlayerBoard* currentPlayer;
		PlayerBoard* otherPlayer;
		if (player1Turn)
		{
			currentPlayer = &player1;
			otherPlayer = &player2;
		}
		else
		{
			currentPlayer = &player2;
			otherPlayer = &player1;
		}

		// roll the dice
		int dice = currentPlayer->rollDice();
		// check current player's possible moves
		short possibleMoves = currentPlayer->getPossibleMoves();

		// initialize the turn
		cout << "Player " << (player1Turn ? 1 : 2) << " turn:" << endl;
		// print boards, first the enemy board then the player's board
		printBoard(otherPlayer->board, true);
		printBoard(currentPlayer->board, false);
		// parse possible moves, and let the player choose
		cout << "You rolled " << dice << endl;
		cout << "You can put the dice in the following columns: ";
		for (int i = 0; i < 3; i++)
		{
			if (possibleMoves & (1 << i))
			{
				cout << i + 1 << " ";
			}
		}
		cout << endl;

		short chosenPos;
		do
		{
			cout << "Choose a column: ";
			cin >> chosenPos;
			chosenPos--;
		} while (!(possibleMoves & (1 << chosenPos)));
		cout << endl;

		// put the dice in the chosen column
		currentPlayer->appendDice(dice, chosenPos);
		otherPlayer->clearColumn(dice, chosenPos);

		// check if the game is over
		if (!isGameOn(player1, player2))
		{
			break;
		}

		// change the turn
		player1Turn = !player1Turn;
	}

	// game is over
	// calculate the score
	cout << "Game over!" << endl;
	cout << "Player 1 board:" << endl;
	printBoard(player1.board, true);
	cout << "Player 2 board:" << endl;
	printBoard(player2.board, false);
	short player1Score = player1.getScore();
	short player2Score = player2.getScore();
	cout << "Player 1 score: " << player1Score << endl;
	cout << "Player 2 score: " << player2Score << endl;
	if (player1Score > player2Score)
	{
		cout << "Player 1 wins!" << endl;
	}
	else if (player1Score < player2Score)
	{
		cout << "Player 2 wins!" << endl;
	}
	else
	{
		cout << "It's a tie!" << endl;
	}
}