test:
	gcc -Wall -o cavalier_GUI cavalier_GUI.c $$(pkg-config --cflags --libs gtk+-3.0)