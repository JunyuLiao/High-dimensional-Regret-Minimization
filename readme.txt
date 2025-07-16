Readme (Strongly Truthful Interactive Regret Minimization)
=========================
This package contains all source codes for 
1. UH-Simplex
2. UH-Random
3. Median (only work on 2-dimensonal datasets)
4. Hull (only work on 2-dimensonal datasets)

This package also provides the real datasets.

Make sure there is a folder called "input/" and a folder "output/" under the working directory.
They will be used for storing the input/output files and some intermediate results.

Usage Step
==========
a. Compilation
	make

	You will need to install the GLPK package (for solving LPs) at first.
	See GLPK webpage <http://www.gnu.org/software/glpk/glpk.html>.
    The Makefile should handle all compilation (once you get GLPK installed correctly).
	Note that "-lglpk" in the Makefile is the GLPK package and you may need to change it to the location where you installed the GLPK package.
	
b. Execution
	./run 

c. Input
	The input file contains the data set (whose format will be described in Appendix A.)
	
d. Output
	1. The output will be shown on the console (whose format will be described in Appendix B.)

Example
=======
Sample input (input/2d.txt) are provided. Try:
./run

Appendix A. Format of Input File
------------------------------------
The format of the first line is: n d
n - number of points in the data set, integer
d - dimensionality, integer
The format of the following n lines is
<dim 1> <dim 2> ... <dim d>.
Each line corresponds to a point.
	
Appendix B. Format of Console Output
------------------------------------
The format of the output is
-----------------------------------------------
|         Method | # of Questions | Point #ID |
-----------------------------------------------
|   Ground Truth |              - |    Pt-#ID |
-----------------------------------------------
|         Median |      Q-count-1 |    Pt-#ID |
-----------------------------------------------
|           Hull |      Q-count-2 |    Pt-#ID |
-----------------------------------------------
|     UH-Simplex |      Q-count-3 |    Pt-#ID |
-----------------------------------------------
|      UH-Random |      Q-count-4 |    Pt-#ID |
-----------------------------------------------
where Pt-#ID is the point ID, 
Q-count-1 is the number of questions asked in Medain,
Q-count-2 is the number of questions asked in Hull,
Q-count-3 is the number of questions asked in UH-Simplex and
Q-count-4 is the number of questions asked in UH-Random.
For example, you might see:
-----------------------------------------------
|         Method | # of Questions | Point #ID |
-----------------------------------------------
|   Ground Truth |              - |       726 |
-----------------------------------------------
|         Median |              4 |       726 |
-----------------------------------------------
|           Hull |              3 |       726 |
-----------------------------------------------
|     UH-Simplex |              3 |       726 |
-----------------------------------------------
|      UH-Random |              3 |       726 |
-----------------------------------------------

