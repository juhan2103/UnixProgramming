PS : rsm Panel CCPS

CCPS : CCPS.c
	gcc -o CCPS CCPS.c -lpthread

vrm : rsm.c
	gcc -o rsm rsm.c -lpthread
Panel : Panel.c
	gcc -o Panel Panel.c -lpthread
clean : 
	rm Panel
	rm CCPS
	rm rsm