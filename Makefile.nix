GAMENAME= mflags

$(GAMENAME): agup-0.11/agup.a $(GAMENAME).c
	gcc -Wall $(GAMENAME).c -o $(GAMENAME) agup-0.11/agup.a `allegro-config --libs`

agup-0.11/agup.a:
	cd agup-0.11 && make

clean:
	cd agup-0.11 && make clean 
	rm -rf $(GAMENAME)
