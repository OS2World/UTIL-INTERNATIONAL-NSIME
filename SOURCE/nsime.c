/* メインソースファイル */
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


/* NSIMEの動作設定などの情報を保存する構造体 */
static NSIMESTATUS_T Status = NSIMESTATUS_DEFAULT;

static int japanese; /* 日本語独自の動作許可フラグ(カタカナ) */


/* NSIMEDLL.DLL の各種プロシージャ */
static HMODULE hmod = 0;
static char errmod[CCHMAXPATH];
static NSIMEDLL_ISEXISTIME* NSIMEDLL_IsExistIme = 0;
static NSIMEDLL_SETSTATUS* NSIMEDLL_SetStatus = 0;
static NSIMEDLL_INPUTPROC* NSIMEDLL_InputProc = 0;
static NSIMEDLL_SENDMSGPROC* NSIMEDLL_SendMsgProc = 0;


/* コマンドラインオプション */
static int english;  /* /E ... 英語モードで動作 */
static int minimize; /* /N ... 最小化状態で実行 */
static int hide;     /* /H ... ウィンドウ・リストに表示しない */

static void CommandOptionCheck(void);


/* 各種ウィンドウ・プロシージャ */
static MRESULT EXPENTRY MainWindowProc( HWND hwnd ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 );
static MRESULT EXPENTRY SettingDialogProc( HWND hdlg ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 );

/* システムメニュー更新 */
static void AddToSystemMenu( HWND hwframe );
/* コンボボックスのサイズ補正 */
static void ResizeComboBox( HWND hdlg ,ULONG idcombo );


/* INIファイル用文字列：ウィンドウ位置 */
static const char PROFILE_NAME[] ={ "NSIME.INI" };
static const char APP_WINDOWSPOS[] ={ "WINDOWSPOS" };
static const char KEY_WINDOWSPOS_X[] ={ "X" };
static const char KEY_WINDOWSPOS_Y[] ={ "Y" };
static const char KEY_WINDOWSPOS_CX[] ={ "CX" };
static const char KEY_WINDOWSPOS_CY[] ={ "CY" };
/* INIファイル用文字列：動作設定 */
static const char APP_MSLIKE[] ={ "MSLIKE" };
static const char KEY_MSLIKE_ENABLE[] ={ "ENABLE" };
static const char KEY_MSLIKE_DELAY[] ={ "DELAY" };
static const char APP_IMEMODE[] ={ "IMEMODE" };
static const char KEY_IMEMODE_ACTIVE[] ={ "ACTIVE" };
static const char KEY_IMEMODE_DEACTIVE[] ={ "DEACTIVE" };
static const char APP_VISIBLE[] ={ "VISIBLE" };
static const char KEY_VISIBLE_SELECT[] ={ "SELECT" };

/* INIファイル読み書き */
static void LoadProfile( HAB hab );
static void SetWindowPos( HWND hwnd );
static void SaveWindowPos( HAB hab ,HWND hwnd );
static void SaveStatus( HAB hab );


/* メイン・ウィンドウのクライアント領域に表示するテキスト */
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

/* 各種エラーメッセージ */
static char STR_ERROR_NOIME[128];
static char STR_ERROR_NODLL[128];
static char STR_ERROR_INVALVER[128];

/* 各種コンボボックスの選択肢の文字列 */
static char STR_NLS_ALPHANUM[32];
static char STR_NLS_HIRAGANA[32];
static char STR_NLS_KATAKANA[32];
static char STR_WIDTH_HALF[32];
static char STR_WIDTH_FULL[32];
static char STR_ROMAJI_OFF[32];
static char STR_ROMAJI_ON[32];

/* 文字列リソースのロード */
static void LoadString( HAB hab ,ULONG cp );


static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt );
static void __strcat__( char* dst ,const char* src );
static char* __strichr__( const char* str ,char chr );


/* メイン関数 */
int main(void)
{
	static const char SEM_NAME[] ={ "\\SEM32\\A.Y.DAYO\\NSIME.EVENT\\ALREADY.EXECUTED" };
	HEV hevexec;
	if( !DosCreateEventSem( (PSZ)SEM_NAME ,&hevexec ,DC_SEM_SHARED ,FALSE ) ){
		/* ↑多重実行を禁止するため、名前付きセマフォを利用する */
		/* 　セマフォを作成できなかった場合、既に NSIME が実行中である */
		/* 　と判断し、プログラムを終了する */

		HAB hab = WinInitialize(0);
		HMQ hmq = WinCreateMsgQueue(hab,0);
		if(hmq){
			/* コマンドラインオプション確認 */
			CommandOptionCheck();

			/* コードページを確認 */
			{
				ULONG cp[8];
				ULONG cr;
				DosQueryCp( sizeof(cp) ,cp ,&cr );
				japanese = (*cp==932 || *cp==942 || *cp==943);
				if( !japanese ){
					/* コードページが日本語でない場合は、強制的に英語モード */
					english = 1;
				}

				/* 文字列リソースのロード */
				LoadString( hab ,*cp );
			}

			/* INIファイルに保存してある情報を読み込む */
			LoadProfile( hab );

			/* ＤＬＬをロード */
			/* インポートライブラリを使った自動ロードを行わなず、 */
			/* わざわざＡＰＩを呼び出して後付けでロードするのは、 */
			/* ＩＭＥが無い環境での実行時にエラーメッセージＢＯＸを */
			/* 表示するのが目的 */
			/* （ＩＭＥ(OS2IM.DLL)がインストールされていない環境だと、 */
			/* NSIMEDLL.DLL のロードに失敗する。あらかじめ EXE と */
			/* DLL が分離されていれば、DLL のロードに失敗しても、 */
			/* EXE 単独で実行できる） */
			DosLoadModule( errmod ,sizeof(errmod) ,(PSZ)"NSIMEDLL" ,&hmod );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_IsExistIme"  ,(PFN*)&NSIMEDLL_IsExistIme );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_SetStatus"   ,(PFN*)&NSIMEDLL_SetStatus );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_InputProc"   ,(PFN*)&NSIMEDLL_InputProc );
			DosQueryProcAddr( hmod ,0 ,(PSZ)"NSIMEDLL_SendMsgProc" ,(PFN*)&NSIMEDLL_SendMsgProc );

			if( !__memcmp__(errmod,"OS2IM",6) || (NSIMEDLL_IsExistIme!=0&&!NSIMEDLL_IsExistIme()) ){
				/* ＩＭＥがインストールされていない */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_NOIME ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else
			if( !hmod || !NSIMEDLL_SetStatus || !NSIMEDLL_InputProc || !NSIMEDLL_SendMsgProc ){
				/* ＤＬＬロード失敗 */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_NODLL ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else
			if( !NSIMEDLL_SetStatus( &Status )
			 || !WinSetHook( hab ,0 ,HK_INPUT ,(PFN)NSIMEDLL_InputProc ,hmod )
			 || !WinSetHook( hab ,0 ,HK_SENDMSG ,(PFN)NSIMEDLL_SendMsgProc ,hmod )
			){
				/* ＤＬＬバージョン不正、または初期化/フック失敗 */
				WinMessageBox( HWND_DESKTOP ,0 ,(PSZ)STR_ERROR_INVALVER ,0 ,0 ,MB_CANCEL|MB_ERROR );
			}
			else{
				/* メイン・ウィンドウ作成 */
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

					/* 最小化及びウィンドウ位置復元 */
					SetWindowPos( hwframe );

					/* ＤＬＬ内変数のリフレッシュ */
					NSIMEDLL_SetStatus( &Status );

					/* システムメニュー更新 */
					AddToSystemMenu( hwframe );

					/* メッセージループ */
					while( WinGetMsg(hab,&qmsg,0,0,0) ){
						WinDispatchMsg(hab,&qmsg);
					}

					/* 最小化及びウィンドウ位置をINIファイルに保存 */
					SaveWindowPos( hab ,hwframe );

					/* ＩＭＥ制御ボタンが消えっぱなしの事があるので、 */
					/* 念のため再表示を行う */
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


/* メインウィンドウ・プロシージャ */
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


/* 「設定」ダイアログウィンドウ・プロシージャ */
static MRESULT EXPENTRY SettingDialogProc( HWND hdlg ,ULONG msg ,MPARAM mp1 ,MPARAM mp2 )
{
	switch( msg ){
	 case WM_INITDLG:
		/* コンボボックスに項目追加 */
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

		/* コンボボックスのサイズ補正 */
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_NLS );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_WIDTH );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_ACT_ROMAJI );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_NLS );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_WIDTH );
		ResizeComboBox( hdlg ,DID_NSIME_COMB_MSLIKE_DEA_ROMAJI );

		/* チェックボックスの設定 */
		WinCheckButton( hdlg ,DID_NSIME_CHKB_MSLIKE_ENABLE ,Status.mslike.enable );
		WinCheckButton( hdlg ,DID_NSIME_CHKB_MSLIKE_DELAY ,Status.mslike.delay );
		WinCheckButton( hdlg
		 ,Status.visible==NSIME_VISIBLE_CANUSE ? DID_NSIME_RADIO_VISIBLE_CANUSE
		 :Status.visible==NSIME_VISIBLE_ONLYUSE ? DID_NSIME_RADIO_VISIBLE_ONLYUSE
		 :DID_NSIME_RADIO_VISIBLE_ALWAYS ,TRUE );

		/* コンボボックスの項目選択 */
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

		/* 仮想的に「クリック」メッセージを送付し、ダイアログの有効/無効状態を設定する */
		WinSendMsg( hdlg ,WM_CONTROL ,MPFROM2SHORT(DID_NSIME_CHKB_MSLIKE_ENABLE,BN_CLICKED)
		 ,(MPARAM)(WinWindowFromID(hdlg,DID_NSIME_CHKB_MSLIKE_ENABLE)) );

		break;
	 case WM_CONTROL:
		switch( SHORT1FROMMP(mp1) ){
		 case DID_NSIME_CHKB_MSLIKE_ENABLE:
			{
				/* ダイアログアイテムの有効/無効をリアルタイムに切り替える */
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

			/* 設定を適用し、INIファイルに保存 */
			NSIMEDLL_SetStatus( &Status );
			SaveStatus( WinQueryAnchorBlock(hdlg) );
		}
		break;
	}
	return WinDefDlgProc( hdlg ,msg ,mp1 ,mp2 );
}


/* コマンドラインオプションで、最小化要求(/N)等の指定があるかを調べる */
/* （実際には、スラッシュの有無は調べていない・・・面倒くさいので(笑)） */
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


/* INIファイル読み込み */
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


/* ウィンドウ位置を設定 */
static void SetWindowPos( HWND hwnd )
{
	/* INIファイルにウィンドウ位置が保存されていれば、復元する */
	/* なお、ウィンドウ位置が保存されていたか否かの確認は、 */
	/* ウィンドウサイズ(cx,cy)の値が 0 以外であるか否かで行う */
	if( minimize ){
		/* 最小化指示有り */
		HWND hwactive = WinQueryWindow(hwnd,QW_NEXTTOP);
		WinSetWindowPos( hwnd ,0 ,0,0,0,0 ,SWP_MINIMIZE|SWP_SHOW );
		/* 最小化後、手頃なウィンドウにフォーカスを譲る */
		WinSetActiveWindow( HWND_DESKTOP ,hwactive );
		if( Status.pos.cx && Status.pos.cy ){
			/* ウィンドウ位置情報有り → ウィンドウ復元位置を上書きする */
			WinSetWindowUShort( hwnd ,QWS_XRESTORE ,Status.pos.x );
			WinSetWindowUShort( hwnd ,QWS_YRESTORE ,Status.pos.y );
			WinSetWindowUShort( hwnd ,QWS_CXRESTORE ,Status.pos.cx );
			WinSetWindowUShort( hwnd ,QWS_CYRESTORE ,Status.pos.cy );
		}
	}
	else{
		/* 最小化指示無し */
		if( Status.pos.cx && Status.pos.cy ){
			/* ウィンドウ位置情報有り → ウィンドウ位置を設定する */
			WinSetWindowPos( hwnd ,0 ,Status.pos.x ,Status.pos.y ,Status.pos.cx ,Status.pos.cy ,SWP_MOVE|SWP_SIZE|SWP_ACTIVATE|SWP_SHOW );
		}
		else{
			/* ウィンドウ位置情報無し → ウィンドウの表示だけを行う */
			WinShowWindow( hwnd ,TRUE );
		}
	}
}


/* INIファイルにウィンドウ位置を保存 */
static void SaveWindowPos( HAB hab ,HWND hwnd )
{
	HINI hini = PrfOpenProfile( hab ,(PSZ)PROFILE_NAME );
	if( hini ){
		SWP swp;
		WinQueryWindowPos( hwnd ,&swp );
		if( (swp.fl&SWP_MINIMIZE) ){
			/* 最小化時は、ウィンドウ位置の取得法方が異なる */
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


/* INIファイルに動作設定を保存 */
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


/* システムメニュー更新 */
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

	/* システムメニューに、NSIME独自のメニュー項目を挿入する */
	/* 挿入位置は、「クローズ」の直前 */
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


/* コンボボックスのサイズ補正 */
static void ResizeComboBox( HWND hdlg ,ULONG idcombo )
{
	HWND hcombo = WinWindowFromID( hdlg ,idcombo );
	USHORT ay;
	SWP swp;
	WinQueryWindowPos( hcombo ,&swp );

	/* コンボボックスのリストをドロップダウンさせた際に、リスト */
	/* ボックスの中に項目がピッタリ収まるサイズに補正する。 */

	/* 追加サイズ＝元のサイズ×項目数×１０÷１２ */
	/* ただし、元のサイズはあらかじめ「ダイアログ・エディター」上で */
	/* １２に設定しておくこと */
	/* （１２＝単一行入力フィールドのデフォルトの高さと同じ） */
	ay = (USHORT)(swp.cy * SHORT1FROMMR(WinSendMsg(hcombo,LM_QUERYITEMCOUNT,0,0)) * 10 / 12);

	/* ドロップダウン・リストが横スクロールバーを含んでいる場合は */
	/* その分縦長にする */
	if( (WinQueryWindowULong(hcombo,QWL_STYLE)&LS_HORZSCROLL) ){
		ay += swp.cy * 10 / 12;
	}

	WinSetWindowPos( hcombo ,0 ,swp.x ,swp.y-ay ,swp.cx ,swp.cy+ay
	 ,SWP_MOVE|SWP_SIZE );
}


/* 文字列リソースのロード */
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
		 case 934: case 944: /* 韓国 */
			id = NSIME_STRID_NLS_HANGEUL_EN;
			break;
		 case 936: case 946: /* 中国 */
			id = NSIME_STRID_NLS_PRC_EN;
			break;
		 case 938: case 948: /* 台湾 */
			id = NSIME_STRID_NLS_TAIWAN_EN;
			break;
		 default: /* その他 */
			id = NSIME_STRID_NLS_ANYTHING_EN;
		}
		WinLoadString( hab ,0 ,id ,sizeof(STR_NLS_HIRAGANA) ,STR_NLS_HIRAGANA );
	}

	__strcat__( NSIME_CLIANT_AA ,CLIENT_SIGNATURE );
}


/* Ｃライブラリを使わないので、自前で用意 */
static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt )
{
	int rc = 0;
	for( ;!rc && cnt ;cnt-- ){
		rc = (int)(*(buf1++) - *(buf2++));
	}
	return rc;
}


/* Ｃライブラリを使わないので、自前で用意 */
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


/* 大小文字を同一視する文字検索 */
static char* __strichr__( const char* str ,char chr )
{
	if( chr>='A' && chr<='Z' ){ chr |= 0x20; }
	for( ;*str && (*str|0x20)!=chr ;str++ ){}
	return (!*str && chr) ? 0 : (char*)str;
}

