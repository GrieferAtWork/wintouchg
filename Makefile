
CFLAGS := -g -Wall -Wextra

wintouchg.exe: main.o manifest.o gui/gui.o
	gcc ${CFLAGS} -o wintouchg.exe main.o manifest.o gui/gui.o -lgdi32
	cmd /C signtool sign /v \
		/s PrivateCertStore \
		/n "wintouchg.exe Certificate" \
		wintouchg.exe

setup:
	cmd /c makecert -r -pe -n "CN=wintouchg.exe Certificate" -ss PrivateCertStore app-wintouchg.cer
	cmd /c certmgr -add app-wintouchg.cer -s -r currentUser root

main.o: main.c
	gcc ${CFLAGS} -c -o main.o main.c

manifest.o: manifest.rc
	windres -F pe-x86-64 manifest.rc -o manifest.o
gui/gui.o: gui/gui.c
	gcc ${CFLAGS} -c -o gui/gui.o gui/gui.c
