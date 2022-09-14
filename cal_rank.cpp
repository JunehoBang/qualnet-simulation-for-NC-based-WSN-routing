//#include <bits/stdc++.h>
#include <cal_rank.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

void swap(unsigned mat[100][100],unsigned R,unsigned C, unsigned row1, unsigned row2,
	unsigned col)
{
	for (unsigned i = 0; i < col; i++)
	{
		unsigned temp = mat[row1][i];
		mat[row1][i] = mat[row2][i];
		mat[row2][i] = temp;
	}
}

unsigned rankOfMatrix(unsigned mat[100][100], unsigned R, unsigned C)
{
    unsigned rank = C;
 
    for (unsigned row = 0; row < rank; row++)
    {
        // Before we visit current row 'row', we make
        // sure that mat[row][0],....mat[row][row-1]
        // are 0.
 
        // Diagonal element is not zero
        if (mat[row][row])
        {
           for (unsigned col = 0; col < R; col++)
           {
               if (col != row)
               {
                 // This makes all entries of current
                 // column as 0 except entry 'mat[row][row]'
                 double mult = (double)mat[col][row] /
                                       mat[row][row];
                 for (unsigned i = 0; i < rank; i++)
                   mat[col][i] -= mult * mat[row][i];
              }
           }
        }
 
        // Diagonal element is already zero. Two cases
        // arise:
        // 1) If there is a row below it with non-zero
        //    entry, then swap this row with that row
        //    and process that row
        // 2) If all elements in current column below
        //    mat[r][row] are 0, then remvoe this column
        //    by swapping it with last column and
        //    reducing number of columns by 1.
        else
        {
            bool reduce = true;
 
            /* Find the non-zero element in current
                column  */
            for (unsigned i = row + 1; i < R;  i++)
            {
                // Swap the row with non-zero element
                // with this row.
                if (mat[i][row])
                {
                    swap(mat, R,C,row, i, rank);
                    reduce = false;
                    break ;
                }
            }
 
            // If we did not find any row with non-zero
            // element in current columnm, then all
            // values in this column are 0.
            if (reduce)
            {
                // Reduce number of columns
                rank--;
 
                // Copy the last column here
                for (unsigned i = 0; i < R; i ++)
                    mat[i][row] = mat[i][rank];
            }
 
            // Process this row again
            row--;
        }
 
       // Uncomment these lines to see intermediate results
       // display(mat, R, C);
       // printf("\n");
    }
    return rank;
}



