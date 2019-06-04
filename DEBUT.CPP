#include <window.h>
#include <stdlib.h>

//TWindow(PTWindowsObject AParent, LPSTR ATitle, PTModule AModule = NULL);
class	cuaso : public TWindow
	{
		private:
			int x,y;
		public:
			cuaso(PTWindowObject cha, LPSTR tuade, PTModule _module):
			TWindow(cha, tuade, _module){};
	};
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)

/*Moi the hien cua 1 ung dung trong Window duoc an dinh bang 1 the
Hinstance, gia tri nay = Null khi do la the hien dau tien cua ung
dung. Tham so thu 3 la dong lenh ket thuc bang 1 ky tu Null. Tham
so thu 4 la gia tri int tra ve cho trinh quan ly Window*/
 
{
  HINSTANCE cur, bef;
  char	*buffer;


  Cur = hInstance;
  bef = hPrevInstance;
  sstrcpy(buffer, lpcmdLine);
  cuaso	cuson1(NULL, "Thu xem sao?",NULL);
  cuason1.creat();
  MessageBox(NULL, "Ban co Hinh dung", "Thay the", MB_OKCANCEL | MB_ICONHAND);
  printf("Cac thong tin: cur=%3f, bef=%3f, cmdshow=%3d\n",cur,bef,nCmdShow);
  printf("Dong thong bao lenh: %s", buffer);
/* Thong Bao Loi

  TAClockApp AClockApp("Cuckoo Clock", hInstance, hPrevInstance,
    lpCmdLine, nCmdShow);
  AClockApp.Run();

  //Free Borland custom controls
  if((int) hBorLibrary > 32)
     FreeLibrary(hBorLibrary);
  return AClockApp.Status;    */
  return 9;
}
