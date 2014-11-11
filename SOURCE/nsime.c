/* ���C���\�[�X�t�@�C�� */
#define INCL_NLS
#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <os2im.h>
#include "nsimever.h"
#include "nsimedll.h"
#include "nsimeres.h"
#include "nsimedlg.h"


/* NSIME�̓���ݒ�Ȃǂ̏���ۑ�����\���� */
static NSIMESTATUS_T Status = NSIMESTATUS_DEFAULT;

static int japanese; /* ���{��Ǝ��̓��싖�t���O(�J�^�J�i) */


/* NSIMEDLL.DLL �̊e��v���V�[�W�� */
static HMODULE hmod = 0;
static char errmod[CCHMAXPATH];
static NSIMEDLL_ISEXISTIME* NSIMEDLL_IsExistIme = 0;
static NSIMEDLL_SETSTATUS* NSIMEDLL_SetStatus = 0;
static NSIMEDLL_INPUTPROC* NSIMEDLL_InputProc = 0;
static NSIMEDLL_SENDMSGPROC* NSIMEDLL_SendMsgProc = 0;


/* �R�}���h���C���I�v�V���� */
static int english;  /* /E ... �p�ꃂ�[�h�œ��� */
static int minimize; /* /N ... �ŏ�����ԂŎ��s */
static int hide;     /* /H ... �E�B���h�E�E���X�g�ɕ\�����Ȃ� */

static void CommandOptionCheck(void);


/* �e��E�B���h�E�E�v���V�[�W�� */
static MRESULT EXPENTRY MainWindowProc( HWND hwnd ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 );
static MRESULT EXPENTRY SettingDialogProc( HWND hdlg ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 );

/* �V�X�e�����j���[�X�V */
static void AddToSystemMenu( HWND hwframe );
/* �R���{�{�b�N�X�̃T�C�Y�␳ */
static void ResizeComboBox( HWND hdlg ,ULONG idcombo );


/* INI�t�@�C���p������F�E�B���h�E�ʒu */
static const char PROFILE_NAME[] ={ "NSIME.INI" };
static const char APP_WINDOWSPOS[] ={ "WINDOWSPOS" };
static const char KEY_WINDOWSPOS_X[] ={ "X" };
static const char KEY_WINDOWSPOS_Y[] ={ "Y" };
static const char KEY_WINDOWSPOS_CX[] ={ "CX" };
static const char KEY_WINDOWSPOS_CY[] ={ "CY" };
/* INI�t�@�C���p������F����ݒ� */
static const char APP_MSLIKE[] ={ "MSLIKE" };
static const char KEY_MSLIKE_ENABLE[] ={ "ENABLE" };
static const char KEY_MSLIKE_DELAY[] ={ "DELAY" };
static const char APP_IMEMODE[] ={ "IMEMODE" };
static const char KEY_IMEMODE_ACTIVE[] ={ "ACTIVE" };
static const char KEY_IMEMODE_DEACTIVE[] ={ "DEACTIVE" };
static const char APP_VISIBLE[] ={ "VISIBLE" };
static const char KEY_VISIBLE_SELECT[] ={ "SELECT" };

/* INI�t�@�C���ǂݏ��� */
static void LoadProfile( HAB hab );
static void SetWindowPos( HWND hwnd );
static void SaveWindowPos( HAB hab ,HWND hwnd );
static void SaveStatus( HAB hab );


/* ���C���E�E�B���h�E�̃N���C�A���g�̈�ɕ\������e�L�X�g */
static char NSIME_CLIANT_AA[384];

static const char CLIENT_SIGNATURE[] ={
"\n"
"\n"
"____________________________________\n"
"\n"
"NSIME ver" NSIME_VERSION_STR NSIME_REVISION_STR "(" __DATE__ ")\n"
"\n"
"by A.Y.DAYO(^-^)v (FZS02603@nifty.ne.jp)\n"
"____________________________________\n"
};

/* �e��G���[���b�Z�[�W */
static char STR_ERROR_NOIME[128];
static char STR_ERROR_NODLL[128];
static char STR_ERROR_INVALVER[128];

/* �e��R���{�{�b�N�X�̑I�����̕����� */
static char STR_NLS_ALPHANUM[32];
static char STR_NLS_HIRAGANA[32];
static char STR_NLS_KATAKANA[32];
static char STR_WIDTH_HALF[32];
static char STR_WIDTH_FULL[32];
static char STR_ROMAJI_OFF[32];
static char STR_ROMAJI_ON[32];

/* �����񃊃\�[�X�̃��[�h */
static void LoadString( HAB hab ,ULONG cp );


static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt );
static void __strcat__( char* dst ,const char* src );
static char* __strichr__( const char* str ,char chr );


/* ���C���֐� */
int main(void)
{
	static const char SEM_NAME[] ={ "\\SEM32\\A.Y.DAYO\\NSIME.EVENT\\ALREADY.EXECUTED" };
	HEV hevexec;
	if( !DosCreateEventSem( (PSZ)SEM_NAME ,&hevexec ,DC_SEM_SHARED ,FALSE ) ){
		/* �����d���s���֎~���邽�߁A���O�t���Z�}�t�H�𗘗p���� */
		/* �@�Z�}�t�H���쐬�ł��Ȃ������ꍇ�A���� NSIME �����s���ł��� */
		/* �@�Ɣ��f���A�v���O�������I������ */

		HAB hab = WinInitialize(0);
		HMQ hmq = WinCreateMsgQueue(hab,0);
		if(hmq){
			/* �R�}���h���C���I�v�V�����m�F */
			CommandOptionCheck();

			/* �R�[�h�y�[�W���m�F */
			{
				ULONG cp[8];
				ULONG cr;
				DosQueryCp( sizeof(cp) ,cp ,&cr );
				japanese = (*cp==932 || *cp==942 || *cp==943);
				if( !japanese ){
					/* �R�[�h�y�[�W�����{��łȂ��ꍇ�́A�����I�ɉp�ꃂ�[�h */
					english = 1;
				}

				/* �����񃊃\�[�X�̃��[�h */
				LoadString( hab ,*cp );
			}

			/* INI�t�@�C���ɕۑ����Ă������ǂݍ��� */
			LoadProfile( hab );

			/* �c�k�k�����[�h */
			/* �C���|�[�g���C�u�������g�����������[�h���s��Ȃ��A */
			/* �킴�킴�`�o�h���Ăяo���Č�t���Ń��[�h����̂́A */
			/* �h�l�d���������ł̎��s���ɃG���[���b�Z�[�W�a�n�w�� */
			/* �\������̂��ړI */
			/* �i�h�l�d(OS2IM.DLL)���C���X�g�[������Ă��Ȃ������ƁA */
			/* NSIMEDLL.DLL �̃��[�h�Ɏ��s����B���炩���� EXE �� */
			/* DLL ����������Ă���΁ADLL �̃��[�h�Ɏ��s���Ă��A */
			/* EXE �P�ƂŎ��s�ł���j */
			DosLoadModule( errmod ,sizeof(errmod) ,(PSZ)"NSIMEDLL" ,&hmod );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_IsExistIme"  ,(PFN*)&NSIMEDLL_IsExistIme );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_SetStatus"   ,(PFN*)&NSIMEDLL_SetStatus );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_InputProc"   ,(PFN*)&NSIMEDLL_InputProc );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_SendMsgProc" ,(PFN*)&NSIMEDLL_SendMsgProc );

			if( !__memcmp__(errmod,"OS2IM",6) || (NSIMEDLL_IsExistIme!=0&&!NSIMEDLL_IsExistIme()) ){
				/* �h�l�d���C���X�g�[������Ă��Ȃ� */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_NOIME ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else
			if( !hmod || !NSIMEDLL_SetStatus || !NSIMEDLL_InputProc || !NSIMEDLL_SendMsgProc ){
				/* �c�k�k���[�h���s */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_NODLL ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else
			if( !NSIMEDLL_SetStatus( &Status )
			 || !WinSetHook( hab ,0 ,HK_INPUT ,(PFN)NSIMEDLL_InputProc ,hmod )
			 || !WinSetHook( hab ,0 ,HK_SENDMSG ,(PFN)NSIMEDLL_SendMsgProc ,hmod )
			){
				/* �c�k�k�o�[�W�����s���A�܂��͏�����/�t�b�N���s */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_INVALVER ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else{
				/* ���C���E�E�B���h�E�쐬 */
				static const char NSIME_MAINWINDOW_CLASS[] ={ "NSIME_MAINWINDOW" };
				ULONG cflg = FCF_AUTOICON|FCF_TITLEBAR|FCF_SYSMENU
				 |FCF_MINBUTTON|FCF_SIZEBORDER|FCF_ICON;
				HWND hwframe;

				if(!hide){ cflg |= FCF_TASKLIST; }
				if(!Status.pos.cx||!Status.pos.cy){ cflg |= FCF_SHELLPOSITION; }

				WinRegisterClass( hab ,(PSZ)NSIME_MAINWINDOW_CLASS
				 ,MainWindowProc ,0 ,0 );

				hwframe = WinCreateStdWindow( HWND_DESKTOP ,0 ,&cflg ,(PSZ)NSIME_MAINWINDOW_CLASS
				 ,(PSZ)"NSIME" ,WS_VISIBLE|MLS_READONLY ,(HMODULE)0 ,NSIME_DEFAULT_RES ,&Status.hwnd );
				if(hwframe){
					QMSG qmsg;

					/* �ŏ����y�уE�B���h�E�ʒu���� */
					SetWindowPos( hwframe );

					/* �c�k�k���ϐ��̃��t���b�V�� */
					NSIMEDLL_SetStatus( &Status );

					/* �V�X�e�����j���[�X�V */
					AddToSystemMenu( hwframe );

					/* ���b�Z�[�W���[�v */
					while( WinGetMsg(hab,&qmsg,0,0,0) ){
						WinDispatchMsg(hab,&qmsg);
					}

					/* �ŏ����y�уE�B���h�E�ʒu��INI�t�@�C���ɕۑ� */
					SaveWindowPos( hab ,hwframe );

					/* �h�l�d����{�^�����������ςȂ��̎�������̂ŁA */
					/* �O�̂��ߍĕ\�����s�� */
					Status.visible = NSIME_VISIBLE_ALWAYS;
					NSIMEDLL_SetStatus( &Status );

					WinDestroyWindow(hwframe);
				}

				WinReleaseHook( hab ,0 ,HK_INPUT ,(PFN)NSIMEDLL_InputProc ,hmod );
				WinReleaseHook( hab ,0 ,HK_SENDMSG ,(PFN)NSIMEDLL_SendMsgProc ,hmod );
				DosFreeModule( hmod );
			}

			WinDestroyMsgQueue(hmq);
		}
		WinTerminate(hab);
		DosCloseEventSem(hevexec);
	}
	return 0;
}


/* ���C���E�B���h�E�E�v���V�[�W�� */
static MRESULT EXPENTRY MainWindowProc( HWND hwnd ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 )
{
	static HWND hwmle;
	switch( msg ){
	 case WM_CREATE:
		hwmle = WinCreateWindow( hwnd ,WC_MLE ,(PSZ)NSIME_CLIANT_AA
		 ,WS_VISIBLE|MLS_READONLY ,0,0,0,0 ,hwnd ,HWND_TOP ,0 ,0 ,0 );
		break;
	 case WM_SIZE:
		WinSetWindowPos( hwmle ,0 ,0 ,0 ,SHORT1FROMMP(mp2) ,SHORT2FROMMP(mp2)
		 ,SWP_MOVE|SWP_SIZE );
		break;
	 case WM_COMMAND:
		switch( COMMANDMSG(&msg)->cmd ){
		 case NSIME_CMDID_SETTINGS:
			WinDlgBox( HWND_DESKTOP ,hwnd ,SettingDialogProc ,(HMODULE)0
			 ,!english ? DIALOGID_NSIME_SETTING_JP : DIALOGID_NSIME_SETTING_EN
			 ,(PVOID)0 );
			break;
		}
		break;
	}
	return WinDefWindowProc( hwnd ,msg ,mp1 ,mp2 );
}


/* �u�ݒ�v�_�C�A���O�E�B���h�E�E�v���V�[�W�� */
static MRESULT EXPENTRY SettingDialogProc( HWND hdlg ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 )
{
	switch( msg ){
	 case WM_INITDLG:
		/* �R���{�{�b�N�X�ɍ��ڒǉ� */
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_ALPHANUM) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_HIRAGANA) );
		if(japanese){
			WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_KATAKANA) );
		}

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH  ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_WIDTH_HALF) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH  ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_WIDTH_FULL) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_ROMAJI_OFF) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_ROMAJI_ON) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_ALPHANUM) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_HIRAGANA) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS    ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_NLS_KATAKANA) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH  ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_WIDTH_HALF) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH  ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_WIDTH_FULL) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_ROMAJI_OFF) );
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI ,LM_INSERTITEM ,MPFROMSHORT(LIT_END) ,MPFROMP(STR_ROMAJI_ON) );

		/* �R���{�{�b�N�X�̃T�C�Y�␳ */
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI );

		/* �`�F�b�N�{�b�N�X�̐ݒ� */
		WinCheckButton( hdlg ,DID_NSIME_CHKB_MSLIKE_ENABLE ,Status.mslike.enable );
		WinCheckButton( hdlg ,DID_NSIME_CHKB_MSLIKE_DELAY ,Status.mslike.delay );
		WinCheckButton( hdlg
		 ,Status.visible==NSIME_VISIBLE_CANUSE ? DID_NSIME_RADIO_VISIBLE_CANUSE
		 :Status.visible==NSIME_VISIBLE_ONLYUSE ? DID_NSIME_RADIO_VISIBLE_ONLYUSE
		 :DID_NSIME_RADIO_VISIBLE_ALWAYS ,TRUE );

		/* �R���{�{�b�N�X�̍��ڑI�� */
		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS    ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.active&IMI_IM_NLS_KATAKANA)==IMI_IM_NLS_KATAKANA?2:(Status.mslike.mode.active&IMI_IM_NLS_HIRAGANA)?1:0)
		 ,MPFROMSHORT(TRUE) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH  ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.active&IMI_IM_WIDTH_FULL)?1:0)
		 ,MPFROMSHORT(TRUE) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.active&IMI_IM_ROMAJI_ON)?1:0)
		 ,MPFROMSHORT(TRUE) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS    ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.deactive&IMI_IM_NLS_KATAKANA)==IMI_IM_NLS_KATAKANA?2:(Status.mslike.mode.deactive&IMI_IM_NLS_HIRAGANA)?1:0)
		 ,MPFROMSHORT(TRUE) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH  ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.deactive&IMI_IM_WIDTH_FULL)?1:0)
		 ,MPFROMSHORT(TRUE) );

		WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI ,LM_SELECTITEM
		 ,MPFROMSHORT((Status.mslike.mode.deactive&IMI_IM_ROMAJI_ON)?1:0)
		 ,MPFROMSHORT(TRUE) );

		/* ���z�I�Ɂu�N���b�N�v���b�Z�[�W�𑗕t���A�_�C�A���O�̗L��/������Ԃ�ݒ肷�� */
		WinSendMsg( hdlg ,WM_CONTROL ,MPFROM2SHORT(DID_NSIME_CHKB_MSLIKE_ENABLE,BN_CLICKED)
		 ,(MPARAM)(WinWindowFromID(hdlg,DID_NSIME_CHKB_MSLIKE_ENABLE)) );

		break;
	 case WM_CONTROL:
		switch( SHORT1FROMMP(mp1) ){
		 case DID_NSIME_CHKB_MSLIKE_ENABLE:
			{
				/* �_�C�A���O�A�C�e���̗L��/���������A���^�C���ɐ؂�ւ��� */
				static const USHORT idbag[] ={
				  DID_NSIME_GROUP_MSLIKE_SET
				 ,DID_NSIME_CHKB_MSLIKE_DELAY
				 ,DID_NSIME_GROUP_MSLIKE_ACT
				 ,DID_NSIME_LTXT_NLS
				 ,DID_NSIME_LTXT_WIDTH
				 ,DID_NSIME_LTXT_ROMAJI
				 ,DID_NSIME_LTXT_MSLIKE_ACTIVE
				 ,DID_NSIME_COMB_MSLIKE_ACT_NLS
				 ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH
				 ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI
				 ,DID_NSIME_LTXT_MSLIKE_DEACTIVE
				 ,DID_NSIME_COMB_MSLIKE_DEA_NLS
				 ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH
				 ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI
				 ,0
				};
				BOOL ena = WinQueryButtonCheckstate(hdlg,DID_NSIME_CHKB_MSLIKE_ENABLE);
				const USHORT* lp;
				for( lp = idbag ;*lp ;lp++ ){
					WinEnableControl(hdlg,*lp,ena);
				}
			}
			break;
		}
		break;
	 case WM_COMMAND:
		if( COMMANDMSG(&msg)->cmd==DID_OK ){
			Status.mslike.enable = WinQueryButtonCheckstate(hdlg,DID_NSIME_CHKB_MSLIKE_ENABLE);
			Status.mslike.delay = WinQueryButtonCheckstate(hdlg,DID_NSIME_CHKB_MSLIKE_DELAY);
			Status.mslike.mode.active = 0;
			Status.mslike.mode.deactive = 0;
			switch( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
			 case 1: Status.mslike.mode.active |= IMI_IM_NLS_HIRAGANA; break;
			 case 2: Status.mslike.mode.active |= IMI_IM_NLS_KATAKANA; break;
			}

			if( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
				Status.mslike.mode.active |= IMI_IM_WIDTH_FULL;
			}

			if( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
				Status.mslike.mode.active |= IMI_IM_ROMAJI_ON;
			}

			switch( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
			 case 1: Status.mslike.mode.deactive |= IMI_IM_NLS_HIRAGANA; break;
			 case 2: Status.mslike.mode.deactive |= IMI_IM_NLS_KATAKANA; break;
			}

			if( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
				Status.mslike.mode.deactive |= IMI_IM_WIDTH_FULL;
			}

			if( SHORT1FROMMR(WinSendDlgItemMsg( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI ,LM_QUERYSELECTION ,MPFROMSHORT(LIT_FIRST) ,0 )) ){
				Status.mslike.mode.deactive |= IMI_IM_ROMAJI_ON;
			}

			Status.visible = WinQueryButtonCheckstate(hdlg,DID_NSIME_RADIO_VISIBLE_CANUSE) ? NSIME_VISIBLE_CANUSE
			 : WinQueryButtonCheckstate(hdlg,DID_NSIME_RADIO_VISIBLE_ONLYUSE) ? NSIME_VISIBLE_ONLYUSE : NSIME_VISIBLE_ALWAYS;

			/* �ݒ��K�p���AINI�t�@�C���ɕۑ� */
			NSIMEDLL_SetStatus( &Status );
			SaveStatus( WinQueryAnchorBlock(hdlg) );
		}
		break;
	}
	return WinDefDlgProc( hdlg ,msg ,mp1 ,mp2 );
}


/* �R�}���h���C���I�v�V�����ŁA�ŏ����v��(/N)���̎w�肪���邩�𒲂ׂ� */
/* �i���ۂɂ́A�X���b�V���̗L���͒��ׂĂ��Ȃ��E�E�E�ʓ|�������̂�(��)�j */
static void CommandOptionCheck(void)
{
	PPIB ppib;
	PTIB ptib;
	const char* opt;

	DosGetInfoBlocks( &ptib ,&ppib );
	opt = __strichr__(ppib->pib_pchcmd,'\0')+1;

	english  = (__strichr__( opt ,'E' )!=0);
	minimize = (__strichr__( opt ,'N' )!=0);
	hide     = (__strichr__( opt ,'H' )!=0);
}


/* INI�t�@�C���ǂݍ��� */
static void LoadProfile( HAB hab )
{
	HINI hini = PrfOpenProfile( hab ,(PSZ)PROFILE_NAME );
	if( hini ){
		ULONG siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_X ,&Status.pos.x ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_Y ,&Status.pos.y ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_CX ,&Status.pos.cx ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_CY ,&Status.pos.cy ,&siz );

		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_MSLIKE  ,(PSZ)KEY_MSLIKE_ENABLE    ,&Status.mslike.enable ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_MSLIKE  ,(PSZ)KEY_MSLIKE_DELAY     ,&Status.mslike.delay ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_IMEMODE ,(PSZ)KEY_IMEMODE_ACTIVE   ,&Status.mslike.mode.active ,&siz );
		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_IMEMODE ,(PSZ)KEY_IMEMODE_DEACTIVE ,&Status.mslike.mode.deactive ,&siz );

		siz = sizeof(ULONG);
		PrfQueryProfileData( hini ,(PSZ)APP_VISIBLE ,(PSZ)KEY_VISIBLE_SELECT ,&Status.visible ,&siz );

		PrfCloseProfile( hini );
	}
}


/* �E�B���h�E�ʒu��ݒ� */
static void SetWindowPos( HWND hwnd )
{
	/* INI�t�@�C���ɃE�B���h�E�ʒu���ۑ�����Ă���΁A�������� */
	/* �Ȃ��A�E�B���h�E�ʒu���ۑ�����Ă������ۂ��̊m�F�́A */
	/* �E�B���h�E�T�C�Y(cx,cy)�̒l�� 0 �ȊO�ł��邩�ۂ��ōs�� */
	if( minimize ){
		/* �ŏ����w���L�� */
		HWND hwactive = WinQueryWindow(hwnd,QW_NEXTTOP);
		WinSetWindowPos( hwnd ,0 ,0,0,0,0 ,SWP_MINIMIZE|SWP_SHOW );
		/* �ŏ�����A�荠�ȃE�B���h�E�Ƀt�H�[�J�X������ */
		WinSetActiveWindow( HWND_DESKTOP ,hwactive );
		if( Status.pos.cx && Status.pos.cy ){
			/* �E�B���h�E�ʒu���L�� �� �E�B���h�E�����ʒu���㏑������ */
			WinSetWindowUShort( hwnd ,QWS_XRESTORE ,Status.pos.x );
			WinSetWindowUShort( hwnd ,QWS_YRESTORE ,Status.pos.y );
			WinSetWindowUShort( hwnd ,QWS_CXRESTORE ,Status.pos.cx );
			WinSetWindowUShort( hwnd ,QWS_CYRESTORE ,Status.pos.cy );
		}
	}
	else{
		/* �ŏ����w������ */
		if( Status.pos.cx && Status.pos.cy ){
			/* �E�B���h�E�ʒu���L�� �� �E�B���h�E�ʒu��ݒ肷�� */
			WinSetWindowPos( hwnd ,0 ,Status.pos.x ,Status.pos.y ,Status.pos.cx ,Status.pos.cy ,SWP_MOVE|SWP_SIZE|SWP_ACTIVATE|SWP_SHOW );
		}
		else{
			/* �E�B���h�E�ʒu��񖳂� �� �E�B���h�E�̕\���������s�� */
			WinShowWindow( hwnd ,TRUE );
		}
	}
}


/* INI�t�@�C���ɃE�B���h�E�ʒu��ۑ� */
static void SaveWindowPos( HAB hab ,HWND hwnd )
{
	HINI hini = PrfOpenProfile( hab ,(PSZ)PROFILE_NAME );
	if( hini ){
		SWP swp;
		WinQueryWindowPos( hwnd ,&swp );
		if( (swp.fl&SWP_MINIMIZE) ){
			/* �ŏ������́A�E�B���h�E�ʒu�̎擾�@�����قȂ� */
			swp.x = WinQueryWindowUShort( hwnd ,QWS_XRESTORE );
			swp.y = WinQueryWindowUShort( hwnd ,QWS_YRESTORE );
			swp.cx = WinQueryWindowUShort( hwnd ,QWS_CXRESTORE );
			swp.cy = WinQueryWindowUShort( hwnd ,QWS_CYRESTORE );
		}

		PrfWriteProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_X ,&swp.x ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_Y ,&swp.y ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_CX ,&swp.cx ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_WINDOWSPOS ,(PSZ)KEY_WINDOWSPOS_CY ,&swp.cy ,sizeof(ULONG) );

		PrfCloseProfile( hini );
	}
}


/* INI�t�@�C���ɓ���ݒ��ۑ� */
static void SaveStatus( HAB hab )
{
	HINI hini = PrfOpenProfile( hab ,(PSZ)PROFILE_NAME );
	if( hini ){
		PrfWriteProfileData( hini ,(PSZ)APP_MSLIKE  ,(PSZ)KEY_MSLIKE_ENABLE    ,&Status.mslike.enable ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_MSLIKE  ,(PSZ)KEY_MSLIKE_DELAY     ,&Status.mslike.delay ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_IMEMODE ,(PSZ)KEY_IMEMODE_ACTIVE   ,&Status.mslike.mode.active ,sizeof(ULONG) );
		PrfWriteProfileData( hini ,(PSZ)APP_IMEMODE ,(PSZ)KEY_IMEMODE_DEACTIVE ,&Status.mslike.mode.deactive ,sizeof(ULONG) );

		PrfWriteProfileData( hini ,(PSZ)APP_VISIBLE ,(PSZ)KEY_VISIBLE_SELECT ,&Status.visible ,sizeof(ULONG) );

		PrfCloseProfile( hini );
	}
}


/* �V�X�e�����j���[�X�V */
static void AddToSystemMenu( HWND hwframe )
{
	HWND hsysmenu = WinWindowFromID(hwframe,FID_SYSMENU);
	HWND htmpmenu = WinLoadMenu( HWND_DESKTOP ,(HMODULE)0 ,!english ? NSIME_MENUID_INSERT_JP : NSIME_MENUID_INSERT_EN );
	SHORT inspos;
	MENUITEM mitem;
	char mtext[64];

	WinSendMsg( hsysmenu ,MM_QUERYITEM
	 ,MPFROM2SHORT( SHORT1FROMMR(WinSendMsg( hsysmenu ,MM_ITEMIDFROMPOSITION ,MPFROMSHORT(0) ,0 )) ,FALSE )
	 ,MPFROMP(&mitem) );
	hsysmenu = mitem.hwndSubMenu;

	/* �V�X�e�����j���[�ɁANSIME�Ǝ��̃��j���[���ڂ�}������ */
	/* �}���ʒu�́A�u�N���[�Y�v�̒��O */
	inspos = SHORT1FROMMR(WinSendMsg( hsysmenu ,MM_ITEMPOSITIONFROMID ,MPFROM2SHORT(SC_CLOSE,TRUE) ,0 )) - 1;

	while( WinSendMsg( htmpmenu ,MM_QUERYITEMCOUNT ,0 ,0 )!=0 ){
		SHORT id = SHORT1FROMMR(WinSendMsg( htmpmenu ,MM_ITEMIDFROMPOSITION ,MPFROMSHORT(0) ,0 ));

		WinSendMsg( htmpmenu ,MM_QUERYITEM ,MPFROM2SHORT(id,FALSE) ,MPFROMP(&mitem) );
		WinSendMsg( htmpmenu ,MM_QUERYITEMTEXT ,MPFROM2SHORT(id,sizeof(mtext)) ,MPFROMP(mtext) );
		WinSendMsg( htmpmenu ,MM_REMOVEITEM ,MPFROM2SHORT(id,FALSE) ,0 );
		mitem.iPosition = inspos;
		WinSendMsg( hsysmenu ,MM_INSERTITEM ,MPFROMP(&mitem) ,MPFROMP(mtext) );

		inspos++;
	}

	WinDestroyWindow(htmpmenu);
}


/* �R���{�{�b�N�X�̃T�C�Y�␳ */
static void ResizeComboBox( HWND hdlg ,ULONG idcombo )
{
	HWND hcombo = WinWindowFromID( hdlg ,idcombo );
	USHORT ay;
	SWP swp;
	WinQueryWindowPos( hcombo ,&swp );

	/* �R���{�{�b�N�X�̃��X�g���h���b�v�_�E���������ۂɁA���X�g */
	/* �{�b�N�X�̒��ɍ��ڂ��s�b�^�����܂�T�C�Y�ɕ␳����B */

	/* �ǉ��T�C�Y�����̃T�C�Y�~���ڐ��~�P�O���P�Q */
	/* �������A���̃T�C�Y�͂��炩���߁u�_�C�A���O�E�G�f�B�^�[�v��� */
	/* �P�Q�ɐݒ肵�Ă������� */
	/* �i�P�Q���P��s���̓t�B�[���h�̃f�t�H���g�̍����Ɠ����j */
	ay = (USHORT)(swp.cy * SHORT1FROMMR(WinSendMsg(hcombo,LM_QUERYITEMCOUNT,0,0)) * 10 / 12);

	/* �h���b�v�_�E���E���X�g�����X�N���[���o�[���܂�ł���ꍇ�� */
	/* ���̕��c���ɂ��� */
	if( (WinQueryWindowULong(hcombo,QWL_STYLE)&LS_HORZSCROLL) ){
		ay += swp.cy * 10 / 12;
	}

	WinSetWindowPos( hcombo ,0 ,swp.x ,swp.y-ay ,swp.cx ,swp.cy+ay
	 ,SWP_MOVE|SWP_SIZE );
}


/* �����񃊃\�[�X�̃��[�h */
struct NSIME_RCSTR_T{
	SHORT id_jp;
	SHORT id_en;
	PSZ ptr;
	LONG siz;
} rcstr[] ={
  { NSIME_STRID_CLIENT_CAPTION_JP ,NSIME_STRID_CLIENT_CAPTION_EN ,NSIME_CLIANT_AA ,128 }
 ,{ NSIME_STRID_ERROR_NOIME_JP    ,NSIME_STRID_ERROR_NOIME_EN    ,STR_ERROR_NOIME ,sizeof(STR_ERROR_NOIME) }
 ,{ NSIME_STRID_ERROR_NODLL_JP    ,NSIME_STRID_ERROR_NODLL_EN    ,STR_ERROR_NODLL ,sizeof(STR_ERROR_NODLL) }
 ,{ NSIME_STRID_ERROR_INVALVER_JP ,NSIME_STRID_ERROR_INVALVER_EN ,STR_ERROR_INVALVER ,sizeof(STR_ERROR_INVALVER) }
 ,{ NSIME_STRID_NLS_ALPHANUM_JP ,NSIME_STRID_NLS_ALPHANUM_EN ,STR_NLS_ALPHANUM ,sizeof(STR_NLS_ALPHANUM) }
 ,{ NSIME_STRID_NLS_HIRAGANA_JP ,NSIME_STRID_NLS_HIRAGANA_EN ,STR_NLS_HIRAGANA ,sizeof(STR_NLS_HIRAGANA) }
 ,{ NSIME_STRID_NLS_KATAKANA_JP ,NSIME_STRID_NLS_KATAKANA_EN ,STR_NLS_KATAKANA ,sizeof(STR_NLS_KATAKANA) }
 ,{ NSIME_STRID_WIDTH_HALF_JP   ,NSIME_STRID_WIDTH_HALF_EN   ,STR_WIDTH_HALF   ,sizeof(STR_WIDTH_HALF) }
 ,{ NSIME_STRID_WIDTH_FULL_JP   ,NSIME_STRID_WIDTH_FULL_EN   ,STR_WIDTH_FULL   ,sizeof(STR_WIDTH_FULL) }
 ,{ NSIME_STRID_ROMAJI_OFF_JP   ,NSIME_STRID_ROMAJI_OFF_EN   ,STR_ROMAJI_OFF   ,sizeof(STR_ROMAJI_OFF) }
 ,{ NSIME_STRID_ROMAJI_ON_JP    ,NSIME_STRID_ROMAJI_ON_EN    ,STR_ROMAJI_ON    ,sizeof(STR_ROMAJI_ON) }
 ,{ 0 ,0 ,0 ,0 }
};

static void LoadString( HAB hab ,ULONG cp )
{
	struct NSIME_RCSTR_T* lp;
	for( lp = rcstr ;lp->ptr ;lp++ ){
		*lp->ptr = '\0';
		WinLoadString( hab ,0 ,!english ? lp->id_jp : lp->id_en ,lp->siz ,lp->ptr );
	}

	if(!japanese){
		SHORT id;
		switch(cp){
		 case 934: case 944: /* �؍� */
			id = NSIME_STRID_NLS_HANGEUL_EN;
			break;
		 case 936: case 946: /* ���� */
			id = NSIME_STRID_NLS_PRC_EN;
			break;
		 case 938: case 948: /* ��p */
			id = NSIME_STRID_NLS_TAIWAN_EN;
			break;
		 default: /* ���̑� */
			id = NSIME_STRID_NLS_ANYTHING_EN;
		}
		WinLoadString( hab ,0 ,id ,sizeof(STR_NLS_HIRAGANA) ,STR_NLS_HIRAGANA );
	}

	__strcat__( NSIME_CLIANT_AA ,CLIENT_SIGNATURE );
}


/* �b���C�u�������g��Ȃ��̂ŁA���O�ŗp�� */
static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt )
{
	int rc = 0;
	for( ;!rc && cnt ;cnt-- ){
		rc = (int)(*(buf1++) - *(buf2++));
	}
	return rc;
}


/* �b���C�u�������g��Ȃ��̂ŁA���O�ŗp�� */
static void __strcat__( char* dst ,const char* src )
{
	const char* sp = src;
	char* dp = dst;
	while(*dp){
		dp++;
	}
	while(*sp){
		*(dp++) = *(sp++);
	}
	*dp = '\0';
}


/* �召�����𓯈ꎋ���镶������ */
static char* __strichr__( const char* str ,char chr )
{
	if( chr>='A' && chr<='Z' ){ chr |= 0x20; }
	for( ;*str && (*str|0x20)!=chr ;str++ ){}
	return (!*str && chr) ? 0 : (char*)str;
}

