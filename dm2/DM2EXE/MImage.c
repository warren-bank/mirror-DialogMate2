/* http://dm2.sf.net */
/* Mini image library. 
 * Conversion jpeg and gif(don't include the gif animation) image to bitmap.
 *
 * History:
 * v0.12 (2005/08/31): fix a bug.
 * v0.11 (2005/04/05): added: Read image from file.
 * v0.10 (2005/04/04): bug fixed: pMem too small when image conversion.
 *
 * When you no longer need the bitmap, call the DeleteObject function to delete it.
 *
 * code by flyfancy. 2005
 */

#include <windows.h>
#include <olectl.h>
#include "MImage.h"


HBITMAP ConversionImage(LPVOID pRes, DWORD size, PMIMAGE_BASEINFO info)
{
	LPVOID pMem;
	IPicture* pPicture = NULL;
	LPSTREAM pStream = 0;
	HBITMAP hBmp = NULL, hOldBmp;
	OLE_XSIZE_HIMETRIC width;
	OLE_YSIZE_HIMETRIC height;
	int bw, bh;

	CoInitialize(NULL);
	pMem = CoTaskMemAlloc(size + 500*1024);
	memcpy(pMem, pRes, size);
	CreateStreamOnHGlobal(pMem, TRUE, &pStream);
	OleLoadPicture(pStream, 0, TRUE, &IID_IPicture, (void **)&pPicture);
	if(pPicture)
	{
		HDC hDC = GetDC(NULL);
		HDC hMemDC = CreateCompatibleDC(hDC);
		if(hMemDC)
		{
			pPicture->lpVtbl->get_Width(pPicture, &width);
			pPicture->lpVtbl->get_Height(pPicture, &height);
			bw = MulDiv(width, GetDeviceCaps(hDC, LOGPIXELSX), 0x09EC);
			bh = MulDiv(height, GetDeviceCaps(hDC, LOGPIXELSY), 0x09EC);
			hBmp = CreateCompatibleBitmap(hDC, bw, bh);
			if(hBmp)
			{
				hOldBmp = SelectObject(hMemDC, hBmp);
				pPicture->lpVtbl->Render(pPicture, hMemDC, 0, 0, bw, bh, 0,
					height, width, -height, NULL);
				SelectObject(hMemDC, hOldBmp);
				//Get Image base info
				if(info)
				{
					info->Width = bw;
					info->Height = bh;
				}
			}
		}
		ReleaseDC(NULL, hDC);
		DeleteDC(hMemDC);
		pPicture->lpVtbl->Release(pPicture);
	}
	pStream->lpVtbl->Release(pStream);
	CoUninitialize();
	return hBmp;
}

HBITMAP LoadImageFromRes(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType, PMIMAGE_BASEINFO info)
{
	LPVOID pRes;
	DWORD dwRes;
	HRSRC jpeg = FindResource(hModule, lpName, lpType);
	if(jpeg)
	{
		if(dwRes = SizeofResource(hModule, jpeg))
		{
			pRes = LockResource(LoadResource(hModule, jpeg));
			return ConversionImage(pRes, dwRes, info);
		}
	}
	return NULL;
}

HBITMAP LoadImageFromFile(LPCTSTR lpFile, PMIMAGE_BASEINFO info)
{
	HANDLE hFile = CreateFile(lpFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, GetFileAttributes(lpFile), NULL);
	HBITMAP hBmp = NULL;
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwRead;
		DWORD dwSize = GetFileSize(hFile, NULL);
		LPVOID pMem = malloc(dwSize);
		ReadFile(hFile, pMem, dwSize, &dwRead, NULL);
		hBmp = ConversionImage(pMem, dwSize, info);
		free(pMem);
		CloseHandle(hFile);
	}
	
	return hBmp;
}

BOOL SaveBitmapToFile(HBITMAP hBitmap, LPCTSTR lpszFileName)
{
	HDC hDC;
	int iBits;
	WORD wBitCount;
	DWORD dwPaletteSize=0, dwBmBitsSize=0, dwDIBSize=0, dwWritten=0;
	BITMAP Bitmap;
	BITMAPFILEHEADER bmfHdr;
	BITMAPINFOHEADER bi;
	LPBITMAPINFOHEADER lpbi;
	HANDLE fh, hDib, hPal,hOldPal=NULL;

	hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
	DeleteDC(hDC);
	if (iBits <= 1)
	wBitCount = 1;
	else if (iBits <= 4)
	wBitCount = 4;
	else if (iBits <= 8)
	wBitCount = 8;
	else
	wBitCount = 24;
	GetObject(hBitmap, sizeof(Bitmap), (LPSTR)&Bitmap);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = Bitmap.bmWidth;
	bi.biHeight = Bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = wBitCount;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrImportant = 0;
	bi.biClrUsed = 0;
	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight;

	hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER));
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;

	hPal = GetStockObject(DEFAULT_PALETTE);
	if (hPal)
	{ 
		hDC = GetDC(NULL);
		hOldPal = SelectPalette(hDC, (HPALETTE)hPal, FALSE);
		RealizePalette(hDC);
	}

	GetDIBits(hDC, hBitmap, 0, (UINT) Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) 
	+dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

	if (hOldPal)
	{
		SelectPalette(hDC, (HPALETTE)hOldPal, TRUE);
		RealizePalette(hDC);
		ReleaseDC(NULL, hDC);
	}

	fh = CreateFile(lpszFileName, GENERIC_WRITE,0, NULL, CREATE_ALWAYS, 
	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	if (fh == INVALID_HANDLE_VALUE)
	return FALSE; 

	bmfHdr.bfType = 0x4D42; // "BM"
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize;
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);

	GlobalUnlock(hDib);
	GlobalFree(hDib);
	CloseHandle(fh);
	return TRUE;
}