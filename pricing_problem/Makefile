all: master_problem pricing_problem.pdf

master_problem: master_problem.cpp
	g++ -lscip -lscipopt master_problem.cpp -o master_problem

%.pdf: %.md
	pandoc $< -o $@
