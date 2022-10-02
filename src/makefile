CC=gcc
FLAGS=-Wall -Werror -Wextra -lstdc++ -std=c++17 -lgtest
SRC=calc_tests/tests.cc controller/controller.cc model/model.cc

all: install

tests: clean
	$(CC) $(FLAGS) $(SRC) -o tests
	./tests

clean:
	rm -rf tests

leaks: tests
	leaks -atExit -- ./tests

install:
	cd view && qmake && make && make clean && rm Makefile && cd ../ && mv view/view.app view/SmartCalc_v2.app && mv view/SmartCalc_v2.app ../build/

uninstall: clean
	rm -rf build/SmartCalc_v2.app

dvi:
	open instructions.html

style_test:
	clang-format -n calc_tests/*
	clang-format -n controller/*
	clang-format -n model/helpers/*
	clang-format -n model/model.cc
	clang-format -n model/model.h
	clang-format -n view/credit.cc
	clang-format -n view/credit.h
	clang-format -n view/view.cc
	clang-format -n view/view.h
