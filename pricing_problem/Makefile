all: master_problem pricing_problem.pdf pricing_problem

master_problem: master_problem.cpp
	g++ -lscip -lscipopt master_problem.cpp -o master_problem

pricing_problem: pricing_problem.cpp
	g++ -lscip -lscipopt pricing_problem.cpp -o pricing_problem

%.pdf: %.md
	pandoc $< -o $@
