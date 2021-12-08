PS : vrm Panel CCPS

CCPS : CCPS.c
	gcc -o CCPS CCPS.c -lpthread

vrm : vrm.c
	gcc -o vrm vrm.c -lpthread
Panel : Panel.c
	gcc -o Panel Panel.c -lpthread
clean : 
	rm Panel
	rm CCPS
	rm vrm