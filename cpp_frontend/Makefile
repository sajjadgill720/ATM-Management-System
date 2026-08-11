# ===========================================================================
#  MYBANK FUTURISTIC C++ CLIENT MAKEFILE
#  Builds the Windows desktop GUI application cyber_atm.exe.
# ===========================================================================

cyber_atm.exe: main.cpp
	g++ -std=c++11 main.cpp -o cyber_atm.exe -lgdi32 -lws2_32 -lmsimg32 -mwindows

clean:
	del /f /q cyber_atm.exe 2>nul || rm -f cyber_atm.exe
