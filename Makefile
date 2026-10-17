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

TESTDIR  = tests

# Engine vs Stockfish at fixed Elo levels, then a rating estimate (needs: brew install stockfish).
# Pass options through MATCH_ARGS, e.g. make match MATCH_ARGS="--tc 10+0.1 1320 1500"
MATCH_SRC = $(SRCDIR)/Board.cpp $(SRCDIR)/evaluate.cpp $(SRCDIR)/search.cpp $(TESTDIR)/Match.cpp

match: $(MATCH_SRC) $(TESTDIR)/test.h | $(BINDIR)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) $(MATCH_SRC) -o $(BINDIR)/match
	./$(BINDIR)/match $(MATCH_ARGS)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -f $(BINDIR)/chess $(BINDIR)/chessgui $(BINDIR)/perft $(BINDIR)/match

.PHONY: terminal gui perft match clean
