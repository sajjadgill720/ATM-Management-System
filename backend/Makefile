# Simple build for the MYBANK console app.
#   make        -> builds ./bank (bank.exe on Windows)
#   make run    -> builds then runs
#   make clean  -> removes the program (keeps your data files)
#   make reset  -> also deletes saved data (fresh bank)

CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
# Shared domain code used by both the console app and the HTTP server.
CORE     = bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp
SRC      = main.cpp atm.cpp admin.cpp $(CORE)
SERVERSRC = server.cpp $(CORE)
TARGET   = bank
SERVER   = server
# Flags for the HTTP server on Windows/MinGW. IMPORTANT: these link flags MUST
# come AFTER the source files (GNU ld resolves libraries left-to-right), and
# -static bundles libstdc++ / libwinpthread so server.exe runs with no extra DLLs.
#   -lws2_32  : Windows Sockets (WSAStartup, socket, bind, ... used by httplib)
#   -pthread  : threading used by httplib
#   -static   : link the runtime statically so the .exe runs standalone
SERVERLDFLAGS = -lws2_32 -pthread -static

all: $(TARGET) $(SERVER)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

$(SERVER): $(SERVERSRC) httplib.h
	$(CXX) $(CXXFLAGS) $(SERVERSRC) -o $(SERVER) $(SERVERLDFLAGS)

run: $(TARGET)
	./$(TARGET)

serve: $(SERVER)
	./$(SERVER)

clean:
	rm -f $(TARGET) $(TARGET).exe $(SERVER) $(SERVER).exe

reset: clean
	rm -f accounts.txt pins.txt transactions.txt loans.txt cash.txt audit.log bank_data.json receipt_*.txt otp.txt
