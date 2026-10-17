CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR   = src
BINDIR   = bin

COMMON_SRC = $(SRCDIR)/Board.cpp $(SRCDIR)/Game.cpp

# Terminal target
terminal: $(COMMON_SRC) $(SRCDIR)/TerminalGame.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(COMMON_SRC) $(SRCDIR)/TerminalGame.cpp -o $(BINDIR)/chess

# GUI target (SFML) — placeholder for future use
SFML_INCLUDE = -I/opt/homebrew/opt/sfml/include
SFML_LIB     = -L/opt/homebrew/opt/sfml/lib -Wl,-rpath,/opt/homebrew/opt/sfml/lib
SFML_LINK    = -lsfml-graphics -lsfml-window -lsfml-system

gui: $(COMMON_SRC) $(SRCDIR)/main.cpp | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(SFML_INCLUDE) $(COMMON_SRC) $(SRCDIR)/main.cpp $(SFML_LIB) $(SFML_LINK) -o $(BINDIR)/chessgui

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -f $(BINDIR)/chess $(BINDIR)/chessgui

.PHONY: terminal gui clean
