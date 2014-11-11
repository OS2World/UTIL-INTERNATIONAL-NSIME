/* ＤＬＬソースファイル */
#define INCL_NLS
#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <os2im.h>
#include "nsimever.h"
#include "nsimedll.h"


NSIMEDLL_ISEXISTIME NSIMEDLL_IsExistIme;
NSIMEDLL_SETSTATUS NSIMEDLL_SetStatus;
NSIMEDLL_INPUTPROC NSIMEDLL_InputProc;
NSIMEDLL_SENDMSGPROC NSIMEDLL_SendMsgProc;


#ifndef __OS2NLS__
/* 日本語関係のシンボルを定義していない開発環境のための定義 */
#define VK_DBE_FIRST			0x80
#define VK_DBE_LAST				0xFF

#define VK_DBE_KATAKANA			0x81
#define VK_DBE_HIRAGANA			0x82
#define VK_DBE_SBCSCHAR			0x83
#define VK_DBE_DBCSCHAR			0x84

#define VK_DBE_IMEACTIVATE		0xA0
#define VK_DBE_IMEDEACTIVATE	0xAA
#endif


/* NSIMEの動作設定などの情報を保存する構造体 */
static NSIMESTATUS_T Status = NSIMESTATUS_DEFAULT;

static int NSIMEDLL_ToggleIME( HWND hwapp ,int set );
static int NSIMEDLL_IsEnableIME( HWND hwapp );
static int SearchImeStatusWindow( HAB hab ,HWND* phwnd ,const char* name ,unsigned int nlen );
static void ShowImeStatusWindow( HWND hwnd ,BOOL flg );

static void __memcpy__( void* dst ,const void* src ,unsigned int cnt );
static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt );

/* ＩＭＥモード表示のウィンドウ・クラス名 */
static const char WC_IME_STATUS_MODE[] ={ "WC_IME_STATUS_MODE" };
/* ＩＭＥ制御ボタンのウィンドウ・クラス名 */
static const char WC_IME_STATUS_SHIFT[] ={ "WC_IME_STATUS_SHIFT" };

static int imeset = 0;      /* 最後のＩＭＥ操作（0=不明、+1=オン、-1=オフ） */
static HWND hwimeshift = 0;		/* ＩＭＥ制御ボタンのウィンドウ・ハンドル */
static HWND hwimemode = 0;		/* ＩＭＥモード表示のウィンドウ・ハンドル */
static int imevis = 0;      /* ＩＭＥ表示・非表示設定フラグ */
static USHORT lastimevkey = KC_NONE;	/* ＩＭＥキー押し下げの記録 */


/* ＤＬＬ初期化（何もしない） */
ULONG EXPENTRY _DLL_InitTerm( HMODULE modhandle ,BOOL flag )
{
	return !0;
}


/* システムにＩＭＥがインストールされているか否かを調査する */
BOOL EXPENTRY NSIMEDLL_IsExistIme(void)
{
	ULONG imcnt = 0;
	ImQueryIMEList(0,0,&imcnt);
	return imcnt!=0;
}


/* ＤＬＬバージョンチェック、及び動作設定 */
BOOL EXPENTRY NSIMEDLL_SetStatus( const NSIMESTATUS_T* sta )
{
	BOOL rc = FALSE;
	if( sta->siz==sizeof(NSIMESTATUS_T)
	 && sta->version==NSIME_VERSION_VAL
	 && sta->revision==NSIME_REVISION_CHR ){
		if( sta->visible==NSIME_VISIBLE_ALWAYS ){
			/* ＩＭＥ制御ボタンが消えっぱなしの事があるので、 */
			/* 念のため再表示を行う */
			ShowImeStatusWindow( Status.hwnd ,TRUE );
		}

		__memcpy__( &Status ,sta ,sizeof(NSIMESTATUS_T) );
		rc = TRUE;
	}
	return rc;
}


/* 入力待ち行列フック */
BOOL EXPENTRY NSIMEDLL_InputProc( HAB hab ,PQMSG pqmsg ,ULONG fs )
{
	static BOOL ignnext = FALSE;	/* 次のＩＭＥオン/オフ操作は無視するフラグ */
	static BOOL afterc = FALSE; /* ＩＭＥオン/オフ操作後のＩＭＥキー入力の監視フラグ */
	static BOOL prevdn = FALSE;	/* KC_PREVDOWN を伴うＩＭＥキー入力の監視フラグ */

	switch( pqmsg->msg ){
	 case WM_NULL:
		/* ダミーの WM_NULL メッセージの確認 */
		/* →ＩＭＥの表示をリフレッシュする。 */
		if( imevis ){
			imevis = FALSE;
			if( !WinIsWindowVisible(hwimemode) ){
				ShowImeStatusWindow( pqmsg->hwnd ,Status.visible==NSIME_VISIBLE_ONLYUSE?FALSE:NSIMEDLL_IsEnableIME(pqmsg->hwnd) );
			}
		}
		break;
	 case WM_CHAR:
		/* キー入力 */
		if( !Status.mslike.enable ){
			if(Status.visible!=NSIME_VISIBLE_ONLYUSE) break;
			else goto skip_delay_imeset;
		}
		else
		if( !Status.mslike.delay ) goto skip_delay_imeset;

		/* ここから、「Virtual PC」用の遅延入力処理 */

		if( CHARMSG(&pqmsg->msg)->vkey>=VK_DBE_FIRST && CHARMSG(&pqmsg->msg)->vkey<=VK_DBE_LAST ){
			/* ＩＭＥ関係のキー入力 → ＩＭＥキー押し下げを記録する */
			/* ただし、スキャンコードと仮想キーコードの双方が有効な場合のみ */
			/* （それで、正常なキー入力だと判断している） */
			if( (CHARMSG(&pqmsg->msg)->fs&(KC_SCANCODE|KC_VIRTUALKEY))==(KC_SCANCODE|KC_VIRTUALKEY) ){
				if( !(CHARMSG(&pqmsg->msg)->fs&KC_KEYUP) ){
					lastimevkey = CHARMSG(&pqmsg->msg)->vkey;
				}
				else
				if( lastimevkey==CHARMSG(&pqmsg->msg)->vkey ){
					lastimevkey = KC_NONE;
				}
			}
		}
		else{
			/* ＩＭＥ関係以外のキー入力 → ＩＭＥキー押し下げ記録をリセット */
			lastimevkey = KC_NONE;
		}
		/* KC_PREVDOWN を伴うＩＭＥキー入力が止まらなくなる現象のフォロー */
		if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_PREVDOWN))==(KC_VIRTUALKEY|KC_PREVDOWN)
		 && CHARMSG(&pqmsg->msg)->vkey>=VK_DBE_FIRST && CHARMSG(&pqmsg->msg)->vkey<=VK_DBE_LAST )
		{
			/* 最初の１回だけ、ＩＭＥをリフレッシュ */
			if( !prevdn ){
				afterc = FALSE;
				prevdn = TRUE;
				NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
			}
		}
		else{
			prevdn = FALSE;
		}

		if( afterc ){
			/* ＩＭＥオン/オフ操作後のＩＭＥキー入力監視中 */
			if( !(CHARMSG(&pqmsg->msg)->fs&KC_VIRTUALKEY) || CHARMSG(&pqmsg->msg)->vkey<VK_DBE_FIRST || CHARMSG(&pqmsg->msg)->vkey>VK_DBE_LAST ){
				/* ＩＭＥと無関係のキー押し下げ */
				/* → 慌ててＩＭＥ状態をリフレッシュして、監視解除 */
				afterc = FALSE;
				NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
			}
			else{
				/* ＩＭＥ関係のキー押し下げ */
				/* → ＩＭＥのリフレッシュを要求する */
				NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
				if( !(CHARMSG(&pqmsg->msg)->fs&KC_DBCSRSRVD1) ){
					afterc = FALSE;
				}
				break;
			}
		}
		else{
			/* ＩＭＥキー入力監視中ではない */
			if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_KEYUP|KC_ALT))==KC_VIRTUALKEY ){
				switch( CHARMSG(&pqmsg->msg)->vkey ){
				 case VK_DBE_SBCSCHAR: /* 半角/全角キー */
				 case VK_DBE_DBCSCHAR: /* 同上 */
					if( ((ULONG)(WinGetPhysKeyState(HWND_DESKTOP,0x38))&0x8000)
					 || ((ULONG)(WinGetPhysKeyState(HWND_DESKTOP,0x3A))&0x8000) )
					{
						/* WM_CHARのAltキー押し下げフラグはオフだが、物理キーの */
						/* 押し下げフラグはオンになっており、矛盾している場合、 */
						/* 念のため、ＩＭＥ状態をリフレッシュする。 */
						/* （「Virtual PC」において、押してもいないＩＭＥ関係の */
						/* キー入力メッセージが発生する事があるので、その対策） */
						NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
					}
					break;
				}
			}
		}
		/* ここまで、「Virtual PC」用の遅延入力処理 */
skip_delay_imeset:

		if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_KEYUP))==(KC_VIRTUALKEY|KC_KEYUP) ){
			/* ＩＭＥキー押し下げ記録がある場合は、記録の方のキーを優先する */
			/* 何故こんな事をするかと言うと、「Virtual PC」にて、押したキー */
			/* と違う仮想キーコードを伴った WM_CHAR メッセージが生成される */
			/* 事があったので、そのフォローのために行っている */
			USHORT vkey;
			if( Status.mslike.delay && lastimevkey!=KC_NONE ){
				vkey = lastimevkey;
				lastimevkey = KC_NONE;
			}
			else{
				vkey = CHARMSG(&pqmsg->msg)->vkey;
			}
			switch( vkey ){
			 case VK_ALT:
				if(!Status.mslike.enable) break;

				if( ((ULONG)WinGetPhysKeyState(HWND_DESKTOP,0x29)&0x8000) ){
					/* Alt+半角/全角キー押下状態から、Altキーが離された */
					/* 正規のＩＭＥオン/オフ操作 → フラグを同期させるだけで、制御は行わない */
					ignnext = TRUE;

					if( Status.visible==NSIME_VISIBLE_ONLYUSE && NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
						/* ＩＭＥ制御ボタンを表示/非表示を切り替える */
						imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;

						/* ＩＭＥモード表示のウィンドウ・ハンドルを取得 */
						SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );

						ShowImeStatusWindow( pqmsg->hwnd ,imeset==+1 );
					}
				}
				break;
			 case VK_DBE_SBCSCHAR: /* 半角/全角キー */
			 case VK_DBE_DBCSCHAR: /* 同上 */
			 case VK_DBE_HIRAGANA: /* ひらがなキー */
				if(!Status.mslike.enable) break;

				if( (CHARMSG(&pqmsg->msg)->fs&KC_ALT) && vkey!=VK_DBE_HIRAGANA ){
					/* Alt+半角/全角キー押下状態から、半角/全角キーが離された */
					/* 正規のＩＭＥオン/オフ操作 → フラグを同期させるだけで、制御は行わない */
					ignnext = TRUE;
					break;
				}
				else
				if(ignnext){
					/* 次のＩＭＥオン/オフ操作は無視するフラグがオン */
					/* → フラグを同期させるだけで、制御は行わない */
					ignnext = FALSE;
					imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;
					break;
				}
				else
				if( (CHARMSG(&pqmsg->msg)->fs&(KC_ALT|KC_CTRL|KC_SHIFT)) ){
					/* Alt/Ctrl/Shift の制御キーが押されている場合は、無視する */
					break;
				}
				else{
					/* ＩＭＥオン/オフ操作が認められているウィンドウであるか、 */
					/* あるいは VIO ウィンドウであるかチェック */
					if( NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
						/* ＩＭＥ制御ボタンが消えっぱなしの事があるので、 */
						/* 念のため再表示を行う */
						ShowImeStatusWindow( pqmsg->hwnd ,TRUE );

						imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,vkey==VK_DBE_HIRAGANA ? +1 : +2 );

						/* ＩＭＥモード表示のウィンドウ・ハンドルを取得 */
						SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );

						/* ＩＭＥオフ時にＩＭＥ制御ボタンを消す */
						if( imeset==-1 && Status.visible==NSIME_VISIBLE_ONLYUSE ){
							ShowImeStatusWindow( pqmsg->hwnd ,FALSE );
						}
						/* ＩＭＥオン/オフ操作後のＩＭＥキー入力監視フラグを立てる */
						afterc = TRUE;
					}
				}
				break;
			 case VK_DBE_IMEACTIVATE:
			 case VK_DBE_IMEDEACTIVATE:
				if( Status.visible==NSIME_VISIBLE_ONLYUSE
				 && (CHARMSG(&pqmsg->msg)->fs&(KC_ALT|KC_CTRL|KC_SHIFT|KC_LONEKEY))==(KC_ALT|KC_LONEKEY)
				 && NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
					/* ＩＭＥ制御ボタン非表示の設定で、正規のＩＭＥオン/オフ操作が行われた */
					/* →ＩＭＥ制御ボタンを表示/非表示を切り替える */
					imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;

					/* ＩＭＥモード表示のウィンドウ・ハンドルを取得 */
					SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );

					WinShowWindow(hwimemode,imeset==+1);
					ShowImeStatusWindow( pqmsg->hwnd ,imeset==+1 );
				}
				break;
			}
		}
		break;
	}
	return FALSE;
}


/* 送信メッセージフック */
VOID EXPENTRY NSIMEDLL_SendMsgProc( HAB hab ,PSMHSTRUCT psmh ,BOOL fInterTask )
{
	switch( psmh->msg ){
	 case WM_ACTIVATE:
		if( SHORT1FROMMP(psmh->mp1) ){
			/* ＩＭＥモード表示のウィンドウ・ハンドルを取得 */
			SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );
			/* ＩＭＥ制御ボタンのウィンドウ・ハンドルを取得 */
			SearchImeStatusWindow( hab ,&hwimeshift ,WC_IME_STATUS_SHIFT ,sizeof(WC_IME_STATUS_SHIFT)-1 );
		}
		break;
	 case WM_SETFOCUS:
		if( SHORT1FROMMP(psmh->mp2) ){
			/* フォーカス・ウィンドウが変化した */
			/* 最後のＩＭＥ操作フラグを無効にする */
			imeset = 0;
			/* ＩＭＥ制御ボタンの表示/非表示を切り替える */
			if( hwimeshift && Status.visible!=NSIME_VISIBLE_ALWAYS ){
				/* ＩＭＥ表示・非表示設定フラグをTRUEに設定した状態で、 */
				/* 擬似メッセージ WM_NULL を通知し、ＩＭＥ制御ボタンの */
				/* 表示/非表示切り替えは、WM_NULL を受け取った入力フック */
				/* ルーチンに任せる。 */
				/* 何故こんな面倒な事をやるのかと言うと、WM_SETFOCUS */
				/* メッセージ処理の段階では、フォーカス・ウィンドウにおける */
				/* ＩＭＥの有効/無効が確定していない事があるので、時間稼ぎ */
				/* を行うのが目的。 */
				imevis = TRUE;
				WinPostMsg( psmh->hwnd ,WM_NULL ,0 ,0 );
			}
		}
		break;
	}
}


/* ＩＭＥのオン/オフを切り替える */
/* 　set==0  = 現行のＩＭＥのオン/オフ状態の参照 */
/* 　set==+1 = オン */
/* 　set==-1 = オフ */
/*   set==+2 = オン/オフ反転 */
/* 　　　　VK_DBE_SBCSCHAR      = フラグのみトグル */
/* 　　　　VK_DBE_DBCSCHAR      = 同上 */
/* 　　　　VK_DBE_HIRAGANA      = フラグのみオン */
static int NSIMEDLL_ToggleIME( HWND hwapp ,int set )
{
	static int togglebusy = 0; /* 多重呼び出し防止 */

	int rc = 0; /* ＩＭＥをオンした際に +1、オフした際に -1 を返す */
	HIMI himi;


	if( togglebusy ){
		return togglebusy;
	}

	togglebusy = set==0 ? imeset : set==+2 ? -imeset : set;

	/* ＩＭＥとのやりとりを行うための準備 */
	if( !ImGetInstance( hwapp ,&himi ) ){
		/* ＩＭＥの現在の状態を取得 */
		IMMODE immode;
		ImQueryIMMode( himi ,&immode.ulInputMode ,&immode.ulConversionMode );
		if( !(immode.ulInputMode&IMI_IM_IME_DISABLE) ){
			/* ＩＭＥが使用可能である */
			if( set==+2 ) set = (immode.ulInputMode&IMI_IM_IME_ON) ? -1 : +1;
			switch( set ){
			 case 0: /* 現行のＩＭＥのオン/オフ状態を返す */
				rc = (immode.ulInputMode&IMI_IM_IME_ON) ? +1 : -1;
				break;
			 case +1: /* ＩＭＥをオンにする */
				immode.ulInputMode = IMI_IM_IME_ON | Status.mslike.mode.active;
				immode.ulConversionMode = IMI_CM_NONE;
				ImSetIMMode( himi ,immode.ulInputMode ,immode.ulConversionMode );
				rc = +1;
				break;
			 case -1: /* ＩＭＥをオフにする */
				immode.ulInputMode = Status.mslike.mode.deactive;
				immode.ulConversionMode = IMI_CM_NONE;
				ImSetIMMode( himi ,immode.ulInputMode ,immode.ulConversionMode );
				rc = WinIsWindowShowing(hwimemode) ? +1 : -1;
				/* ↑ＩＭＥオフ操作後に、実際にオフになったかどうかを確認し、 */
				/* 　その結果を戻り値にする。 */
				break;
			}
		}
		ImReleaseInstance( hwapp ,himi );
	}

	togglebusy = 0;
	return rc;
}


/* ＩＭＥオン/オフ操作が認められているウィンドウであるかチェック */
static int NSIMEDLL_IsEnableIME( HWND hwapp )
{
	RECTL rect;
	int rc = SHORT1FROMMR(WinSendMsg( hwapp ,WM_QUERYCONVERTPOS ,MPFROMP(&rect) ,0 ))!=QCP_NOCONVERT;
	if(rc){
		char cls[5];
		if( WinQueryClassName( hwapp ,sizeof(cls) ,cls )==3 && cls[0]=='#' ){
			/* マルチメディア(MMPM)関係のウィンドウ・クラスは、無意味に */
			/* 「ＩＭＥ操作を認める」というレスポンスを返してくるので、 */
			/* 「認めない」というレスポンスを捏造する */
			int val = ((cls[1]-'0') * 10 + (cls[2]-'0'));
			rc = val<0x40 || val>0x4F; /* WC_MMPMFIRST～WC_MMPMLAST */
		}
	}
	return rc;
}


/* ＩＭＥ制御ボタン及びＩＭＥモード表示のウィンドウを検索する */
static int SearchImeStatusWindow( HAB hab ,HWND* phwnd ,const char* name ,unsigned int nlen )
{
	if(*phwnd){
		if( !WinIsWindow(hab,*phwnd) ) *phwnd = 0;
	}
	if(!*phwnd){
		HENUM he = WinBeginEnumWindows(HWND_DESKTOP);
		if(he){
			HWND hnext;
			while( (hnext=WinGetNextWindow(he))!=0 ){
				char buf[32];
				int rlen = WinQueryClassName( hnext ,sizeof(buf) ,buf );
				if( rlen==nlen && !__memcmp__( buf ,name ,nlen ) ){
					*phwnd = hnext;
					break;
				}
			}
			WinEndEnumWindows(he);
		}
	}
	return *phwnd!=0;
}


/* ＩＭＥ制御ボタンを表示/非表示する */
static void ShowImeStatusWindow( HWND hwnd ,BOOL flg )
{
	if(flg){
		/* 表示は、ＩＭＥ用のＡＰＩを用いて行う */
		HIMI himi;
		if( !ImGetInstance( hwnd ,&himi ) ){
			ImShowStatusWindow( himi ,flg );
			ImReleaseInstance( hwnd ,himi );
		}
	}
	else{
		/* 非表示は、普通のウィンドウＡＰＩを用いて行う */
		/* ナゼかというと、ＩＭＥ用のＡＰＩを用いて消去すると、 */
		/* NSIME を終了させた後も非表示時状態が解除されないため */
		WinShowWindow(hwimeshift,FALSE);
	}
}


/* Ｃライブラリを使わないので、自前で用意 */
static void __memcpy__( void* dst ,const void* src ,unsigned int cnt )
{
	const char* sp;
	char* dp;
	for( sp = (const char*)src ,dp = (char*)dst ;cnt ;cnt-- ){
		*(dp++) = *(sp++);
	}
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
