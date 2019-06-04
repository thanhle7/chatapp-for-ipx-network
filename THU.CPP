#include <stdio.h>
#include <dos.h>
#include <CONIO.H>

typedef  void far (*far_pf)();

void thongbao(void)
{
	printf("Thu coi choi .....\n");
};

void main(void)
{
	unsigned char kytu;
	far_pf pf;
	pf = (far_pf )thongbao;
	(*pf)();
	kytu = getch();
	if (kytu == 0) kytu = getch();
	printf("co tri : %d", kytu);
}