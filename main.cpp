#include <windows.h>
#include "YMDirectSoundPlayer.h"
#include "resource.h"

/* Declare Windows procedure */
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DialogProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
char fileName[1024];

// Emulator object.
YMDirectSoundPlayer *player;

// Instance
HINSTANCE hInstance;
OPENFILENAME ofStr;

/* Make the class name into a global variable */
char szClassName[ ] = "WindowsApp";
int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nFunsterStil)

{
    // Save the instance.
    hInstance = hThisInstance;
    
    // Prep the openfile struct
    memset(&ofStr, 0, sizeof(ofStr));
    ofStr.lStructSize = sizeof(OPENFILENAME);       
    ofStr.hInstance = hInstance;
    ofStr.lpstrFile = fileName;
    ofStr.nMaxFile = sizeof(fileName);
    memset(ofStr.lpstrFile, 0, 1024);
    
    // Create and display main dialog.
	HWND hwndDialog = (HWND) DialogBox(hThisInstance,
              MAKEINTRESOURCE(IDD_YMPLAYMFC_DIALOG),
	          HWND_DESKTOP, DialogProcedure);
	          
            
	ShowWindow(hwndDialog, SW_SHOW);          



    return 0;
}

void Update(HWND hwnd)
{
    bool playing = player->IsPlaying();
    SetDlgItemText(hwnd, IDC_PLAYBUTTON, playing ? "Stop" : "Play");
}

void PlayStop(HWND hwnd)
{
    if(player->IsPlaying())
    {
        player->Stop();
    }
    else
    {
        player->Play();
    }
    
    Update(hwnd);
}



void SelectFile(HWND hwnd)
{        
    // Open the dialog
    ofStr.hwndOwner = hwnd;
    if(GetOpenFileName(&ofStr))
    {
        // Put result into text control.
        SetDlgItemText(hwnd, IDC_FILEPATH, ofStr.lpstrFile);
        
        player->Stop();
        
        if(player->Load(ofStr.lpstrFile) != 0)
        {
           MessageBoxEx(hwnd, "Failed to load file", "Load Error",
                        MB_OK | MB_ICONERROR, 0);
        }
        else
        {
            player->Play();            
            HWND play_hwnd = GetDlgItem(hwnd, IDC_PLAYBUTTON);
            EnableWindow(play_hwnd, true);                      

        }
    }
    
    Update(hwnd);
}

BOOL CALLBACK DialogProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{    
    int winId = LOWORD(wParam);     
     
    switch (message)                  /* handle the messages */
    {
           case WM_INITDIALOG:
                // Create emulator object.
                player = new YMDirectSoundPlayer(hwnd);
//                player->Load("test.ym");
//                player->Play();
                break;
                
           case WM_COMMAND:

                if(winId == IDC_QUITBTN)
                {
                    EndDialog(hwnd, TRUE);        /* send a WM_QUIT to the message queue */
                }
                
                if(winId == IDC_SELTRACK)
                {
                    SelectFile(hwnd);
                }
                
                if(winId == IDC_PLAYBUTTON)
                {
                    PlayStop(hwnd);
                }
                break;

           case WM_CLOSE:
           case WM_DESTROY:
				
			   if (!player)
				{
					delete player;
					player = 0;
				}
			    
                EndDialog(hwnd, TRUE);        /* send a WM_QUIT to the message queue */
                break;
           default:                   /* for messages that we don't deal with */
           return FALSE;
    }
    return TRUE;
}


/* This function is called by the Windows function DispatchMessage( ) */
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
           case WM_DESTROY:
              PostQuitMessage(0);        /* send a WM_QUIT to the message queue */
              break;

           default:                   /* for messages that we don't deal with */
              return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
