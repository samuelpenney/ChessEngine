CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -g
SRCDIR   = src
BINDIR   = bin

COMMON_SRC = $(SRCDIR)/Board.cpp $(SRCDIR)/Game.cpp $(SRCDIR)/evaluate.cpp $(SRCDIR)/search.cpp

# Terminal target
terminal: $(COMMON_SRC) $(SRCDIR)/TerminalGame.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(COMMON_SRC) $(SRCDIR)/TerminalGame.cpp -o $(BINDIR)/chess

# GUI target (SFML) — placeholder for future use
SFML_INCLUDE = -I/opt/homebrew/opt/sfml/include
SFML_LIB     = -L/opt/homebrew/opt/sfml/lib -Wl,-rpath,/opt/homebrew/opt/sfml/lib
SFML_LINK    = -lsfml-graphics -lsfml-window -lsfml-system

gui: $(COMMON_SRC) $(SRCDIR)/main.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(SFML_INCLUDE) $(COMMON_SRC) $(SRCDIR)/main.cpp $(SFML_LIB) $(SFML_LINK) -o $(BINDIR)/chessgui

# Perft: checks move generation against published node counts, then runs it.
# Pass options through PERFT_ARGS, e.g. make perft PERFT_ARGS=--deep
perft: $(SRCDIR)/Board.cpp $(SRCDIR)/Board.h $(SRCDIR)/Perft.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(SRCDIR)/Board.cpp $(SRCDIR)/Perft.cpp -o $(BINDIR)/perft
	./$(BINDIR)/perft $(PERFT_ARGS)

# Tests for evaluation and search, with runtimes and a search benchmark.
# Pass options through TEST_ARGS, e.g. make test TEST_ARGS=--no-bench
TESTDIR  = tests
TEST_SRC = $(SRCDIR)/Board.cpp $(SRCDIR)/evaluate.cpp $(SRCDIR)/search.cpp \
           $(TESTDIR)/EvaluateTests.cpp $(TESTDIR)/SearchTests.cpp $(TESTDIR)/TestMain.cpp

test: $(TEST_SRC) $(TESTDIR)/test.h | $(BINDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(TEST_SRC) -o $(BINDIR)/tests
	./$(BINDIR)/tests $(TEST_ARGS)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -f $(BINDIR)/chess $(BINDIR)/chessgui $(BINDIR)/perft $(BINDIR)/tests

.PHONY: terminal gui perft test clean
