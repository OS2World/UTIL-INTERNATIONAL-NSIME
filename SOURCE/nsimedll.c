/* �c�k�k�\�[�X�t�@�C�� */
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
/* ���{��֌W�̃V���{�����`���Ă��Ȃ��J�����̂��߂̒�` */
#define VK_DBE_FIRST			0x80
#define VK_DBE_LAST				0xFF

#define VK_DBE_KATAKANA			0x81
#define VK_DBE_HIRAGANA			0x82
#define VK_DBE_SBCSCHAR			0x83
#define VK_DBE_DBCSCHAR			0x84

#define VK_DBE_IMEACTIVATE		0xA0
#define VK_DBE_IMEDEACTIVATE	0xAA
#endif


/* NSIME�̓���ݒ�Ȃǂ̏���ۑ�����\���� */
static NSIMESTATUS_T Status = NSIMESTATUS_DEFAULT;

static int NSIMEDLL_ToggleIME( HWND hwapp ,int set );
static int NSIMEDLL_IsEnableIME( HWND hwapp );
static int SearchImeStatusWindow( HAB hab ,HWND* phwnd ,const char* name ,unsigned int nlen );
static void ShowImeStatusWindow( HWND hwnd ,BOOL flg );

static void __memcpy__( void* dst ,const void* src ,unsigned int cnt );
static int __memcmp__( const char* buf1 ,const char* buf2 ,unsigned int cnt );

/* �h�l�d���[�h�\���̃E�B���h�E�E�N���X�� */
static const char WC_IME_STATUS_MODE[] ={ "WC_IME_STATUS_MODE" };
/* �h�l�d����{�^���̃E�B���h�E�E�N���X�� */
static const char WC_IME_STATUS_SHIFT[] ={ "WC_IME_STATUS_SHIFT" };

static int imeset = 0;      /* �Ō�̂h�l�d����i0=�s���A+1=�I���A-1=�I�t�j */
static HWND hwimeshift = 0;		/* �h�l�d����{�^���̃E�B���h�E�E�n���h�� */
static HWND hwimemode = 0;		/* �h�l�d���[�h�\���̃E�B���h�E�E�n���h�� */
static int imevis = 0;      /* �h�l�d�\���E��\���ݒ�t���O */
static USHORT lastimevkey = KC_NONE;	/* �h�l�d�L�[���������̋L�^ */


/* �c�k�k�������i�������Ȃ��j */
ULONG EXPENTRY _DLL_InitTerm( HMODULE modhandle ,BOOL flag )
{
	return !0;
}


/* �V�X�e���ɂh�l�d���C���X�g�[������Ă��邩�ۂ��𒲍����� */
BOOL EXPENTRY NSIMEDLL_IsExistIme(void)
{
	ULONG imcnt = 0;
	ImQueryIMEList(0,0,&imcnt);
	return imcnt!=0;
}


/* �c�k�k�o�[�W�����`�F�b�N�A�y�ѓ���ݒ� */
BOOL EXPENTRY NSIMEDLL_SetStatus( const NSIMESTATUS_T* sta )
{
	BOOL rc = FALSE;
	if( sta->siz==sizeof(NSIMESTATUS_T)
	 && sta->version==NSIME_VERSION_VAL
	 && sta->revision==NSIME_REVISION_CHR ){
		if( sta->visible==NSIME_VISIBLE_ALWAYS ){
			/* �h�l�d����{�^�����������ςȂ��̎�������̂ŁA */
			/* �O�̂��ߍĕ\�����s�� */
			ShowImeStatusWindow( Status.hwnd ,TRUE );
		}

		__memcpy__( &Status ,sta ,sizeof(NSIMESTATUS_T) );
		rc = TRUE;
	}
	return rc;
}


/* ���͑҂��s��t�b�N */
BOOL EXPENTRY NSIMEDLL_InputProc( HAB hab ,PQMSG pqmsg ,ULONG fs )
{
	static BOOL ignnext = FALSE;	/* ���̂h�l�d�I��/�I�t����͖�������t���O */
	static BOOL afterc = FALSE; /* �h�l�d�I��/�I�t�����̂h�l�d�L�[���͂̊Ď��t���O */
	static BOOL prevdn = FALSE;	/* KC_PREVDOWN �𔺂��h�l�d�L�[���͂̊Ď��t���O */

	switch( pqmsg->msg ){
	 case WM_NULL:
		/* �_�~�[�� WM_NULL ���b�Z�[�W�̊m�F */
		/* ���h�l�d�̕\�������t���b�V������B */
		if( imevis ){
			imevis = FALSE;
			if( !WinIsWindowVisible(hwimemode) ){
				ShowImeStatusWindow( pqmsg->hwnd ,Status.visible==NSIME_VISIBLE_ONLYUSE?FALSE:NSIMEDLL_IsEnableIME(pqmsg->hwnd) );
			}
		}
		break;
	 case WM_CHAR:
		/* �L�[���� */
		if( !Status.mslike.enable ){
			if(Status.visible!=NSIME_VISIBLE_ONLYUSE) break;
			else goto skip_delay_imeset;
		}
		else
		if( !Status.mslike.delay ) goto skip_delay_imeset;

		/* ��������A�uVirtual PC�v�p�̒x�����͏��� */

		if( CHARMSG(&pqmsg->msg)->vkey>=VK_DBE_FIRST && CHARMSG(&pqmsg->msg)->vkey<=VK_DBE_LAST ){
			/* �h�l�d�֌W�̃L�[���� �� �h�l�d�L�[�����������L�^���� */
			/* �������A�X�L�����R�[�h�Ɖ��z�L�[�R�[�h�̑o�����L���ȏꍇ�̂� */
			/* �i����ŁA����ȃL�[���͂��Ɣ��f���Ă���j */
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
			/* �h�l�d�֌W�ȊO�̃L�[���� �� �h�l�d�L�[���������L�^�����Z�b�g */
			lastimevkey = KC_NONE;
		}
		/* KC_PREVDOWN �𔺂��h�l�d�L�[���͂��~�܂�Ȃ��Ȃ錻�ۂ̃t�H���[ */
		if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_PREVDOWN))==(KC_VIRTUALKEY|KC_PREVDOWN)
		 && CHARMSG(&pqmsg->msg)->vkey>=VK_DBE_FIRST && CHARMSG(&pqmsg->msg)->vkey<=VK_DBE_LAST )
		{
			/* �ŏ��̂P�񂾂��A�h�l�d�����t���b�V�� */
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
			/* �h�l�d�I��/�I�t�����̂h�l�d�L�[���͊Ď��� */
			if( !(CHARMSG(&pqmsg->msg)->fs&KC_VIRTUALKEY) || CHARMSG(&pqmsg->msg)->vkey<VK_DBE_FIRST || CHARMSG(&pqmsg->msg)->vkey>VK_DBE_LAST ){
				/* �h�l�d�Ɩ��֌W�̃L�[�������� */
				/* �� �Q�ĂĂh�l�d��Ԃ����t���b�V�����āA�Ď����� */
				afterc = FALSE;
				NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
			}
			else{
				/* �h�l�d�֌W�̃L�[�������� */
				/* �� �h�l�d�̃��t���b�V����v������ */
				NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
				if( !(CHARMSG(&pqmsg->msg)->fs&KC_DBCSRSRVD1) ){
					afterc = FALSE;
				}
				break;
			}
		}
		else{
			/* �h�l�d�L�[���͊Ď����ł͂Ȃ� */
			if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_KEYUP|KC_ALT))==KC_VIRTUALKEY ){
				switch( CHARMSG(&pqmsg->msg)->vkey ){
				 case VK_DBE_SBCSCHAR: /* ���p/�S�p�L�[ */
				 case VK_DBE_DBCSCHAR: /* ���� */
					if( ((ULONG)(WinGetPhysKeyState(HWND_DESKTOP,0x38))&0x8000)
					 || ((ULONG)(WinGetPhysKeyState(HWND_DESKTOP,0x3A))&0x8000) )
					{
						/* WM_CHAR��Alt�L�[���������t���O�̓I�t�����A�����L�[�� */
						/* ���������t���O�̓I���ɂȂ��Ă���A�������Ă���ꍇ�A */
						/* �O�̂��߁A�h�l�d��Ԃ����t���b�V������B */
						/* �i�uVirtual PC�v�ɂ����āA�����Ă����Ȃ��h�l�d�֌W�� */
						/* �L�[���̓��b�Z�[�W���������鎖������̂ŁA���̑΍�j */
						NSIMEDLL_ToggleIME( pqmsg->hwnd ,imeset );
					}
					break;
				}
			}
		}
		/* �����܂ŁA�uVirtual PC�v�p�̒x�����͏��� */
skip_delay_imeset:

		if( (CHARMSG(&pqmsg->msg)->fs&(KC_VIRTUALKEY|KC_KEYUP))==(KC_VIRTUALKEY|KC_KEYUP) ){
			/* �h�l�d�L�[���������L�^������ꍇ�́A�L�^�̕��̃L�[��D�悷�� */
			/* ���̂���Ȏ������邩�ƌ����ƁA�uVirtual PC�v�ɂāA�������L�[ */
			/* �ƈႤ���z�L�[�R�[�h�𔺂��� WM_CHAR ���b�Z�[�W����������� */
			/* �����������̂ŁA���̃t�H���[�̂��߂ɍs���Ă��� */
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
					/* Alt+���p/�S�p�L�[������Ԃ���AAlt�L�[�������ꂽ */
					/* ���K�̂h�l�d�I��/�I�t���� �� �t���O�𓯊������邾���ŁA����͍s��Ȃ� */
					ignnext = TRUE;

					if( Status.visible==NSIME_VISIBLE_ONLYUSE && NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
						/* �h�l�d����{�^����\��/��\����؂�ւ��� */
						imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;

						/* �h�l�d���[�h�\���̃E�B���h�E�E�n���h�����擾 */
						SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );

						ShowImeStatusWindow( pqmsg->hwnd ,imeset==+1 );
					}
				}
				break;
			 case VK_DBE_SBCSCHAR: /* ���p/�S�p�L�[ */
			 case VK_DBE_DBCSCHAR: /* ���� */
			 case VK_DBE_HIRAGANA: /* �Ђ炪�ȃL�[ */
				if(!Status.mslike.enable) break;

				if( (CHARMSG(&pqmsg->msg)->fs&KC_ALT) && vkey!=VK_DBE_HIRAGANA ){
					/* Alt+���p/�S�p�L�[������Ԃ���A���p/�S�p�L�[�������ꂽ */
					/* ���K�̂h�l�d�I��/�I�t���� �� �t���O�𓯊������邾���ŁA����͍s��Ȃ� */
					ignnext = TRUE;
					break;
				}
				else
				if(ignnext){
					/* ���̂h�l�d�I��/�I�t����͖�������t���O���I�� */
					/* �� �t���O�𓯊������邾���ŁA����͍s��Ȃ� */
					ignnext = FALSE;
					imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;
					break;
				}
				else
				if( (CHARMSG(&pqmsg->msg)->fs&(KC_ALT|KC_CTRL|KC_SHIFT)) ){
					/* Alt/Ctrl/Shift �̐���L�[��������Ă���ꍇ�́A�������� */
					break;
				}
				else{
					/* �h�l�d�I��/�I�t���삪�F�߂��Ă���E�B���h�E�ł��邩�A */
					/* ���邢�� VIO �E�B���h�E�ł��邩�`�F�b�N */
					if( NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
						/* �h�l�d����{�^�����������ςȂ��̎�������̂ŁA */
						/* �O�̂��ߍĕ\�����s�� */
						ShowImeStatusWindow( pqmsg->hwnd ,TRUE );

						imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,vkey==VK_DBE_HIRAGANA ? +1 : +2 );

						/* �h�l�d���[�h�\���̃E�B���h�E�E�n���h�����擾 */
						SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );

						/* �h�l�d�I�t���ɂh�l�d����{�^�������� */
						if( imeset==-1 && Status.visible==NSIME_VISIBLE_ONLYUSE ){
							ShowImeStatusWindow( pqmsg->hwnd ,FALSE );
						}
						/* �h�l�d�I��/�I�t�����̂h�l�d�L�[���͊Ď��t���O�𗧂Ă� */
						afterc = TRUE;
					}
				}
				break;
			 case VK_DBE_IMEACTIVATE:
			 case VK_DBE_IMEDEACTIVATE:
				if( Status.visible==NSIME_VISIBLE_ONLYUSE
				 && (CHARMSG(&pqmsg->msg)->fs&(KC_ALT|KC_CTRL|KC_SHIFT|KC_LONEKEY))==(KC_ALT|KC_LONEKEY)
				 && NSIMEDLL_IsEnableIME(pqmsg->hwnd) ){
					/* �h�l�d����{�^����\���̐ݒ�ŁA���K�̂h�l�d�I��/�I�t���삪�s��ꂽ */
					/* ���h�l�d����{�^����\��/��\����؂�ւ��� */
					imeset = NSIMEDLL_ToggleIME( pqmsg->hwnd ,0 )<0 ? -1 : +1;

					/* �h�l�d���[�h�\���̃E�B���h�E�E�n���h�����擾 */
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


/* ���M���b�Z�[�W�t�b�N */
VOID EXPENTRY NSIMEDLL_SendMsgProc( HAB hab ,PSMHSTRUCT psmh ,BOOL fInterTask )
{
	switch( psmh->msg ){
	 case WM_ACTIVATE:
		if( SHORT1FROMMP(psmh->mp1) ){
			/* �h�l�d���[�h�\���̃E�B���h�E�E�n���h�����擾 */
			SearchImeStatusWindow( hab ,&hwimemode ,WC_IME_STATUS_MODE ,sizeof(WC_IME_STATUS_MODE)-1 );
			/* �h�l�d����{�^���̃E�B���h�E�E�n���h�����擾 */
			SearchImeStatusWindow( hab ,&hwimeshift ,WC_IME_STATUS_SHIFT ,sizeof(WC_IME_STATUS_SHIFT)-1 );
		}
		break;
	 case WM_SETFOCUS:
		if( SHORT1FROMMP(psmh->mp2) ){
			/* �t�H�[�J�X�E�E�B���h�E���ω����� */
			/* �Ō�̂h�l�d����t���O�𖳌��ɂ��� */
			imeset = 0;
			/* �h�l�d����{�^���̕\��/��\����؂�ւ��� */
			if( hwimeshift && Status.visible!=NSIME_VISIBLE_ALWAYS ){
				/* �h�l�d�\���E��\���ݒ�t���O��TRUE�ɐݒ肵����ԂŁA */
				/* �[�����b�Z�[�W WM_NULL ��ʒm���A�h�l�d����{�^���� */
				/* �\��/��\���؂�ւ��́AWM_NULL ���󂯎�������̓t�b�N */
				/* ���[�`���ɔC����B */
				/* ���̂���Ȗʓ|�Ȏ������̂��ƌ����ƁAWM_SETFOCUS */
				/* ���b�Z�[�W�����̒i�K�ł́A�t�H�[�J�X�E�E�B���h�E�ɂ����� */
				/* �h�l�d�̗L��/�������m�肵�Ă��Ȃ���������̂ŁA���ԉ҂� */
				/* ���s���̂��ړI�B */
				imevis = TRUE;
				WinPostMsg( psmh->hwnd ,WM_NULL ,0 ,0 );
			}
		}
		break;
	}
}


/* �h�l�d�̃I��/�I�t��؂�ւ��� */
/* �@set==0  = ���s�̂h�l�d�̃I��/�I�t��Ԃ̎Q�� */
/* �@set==+1 = �I�� */
/* �@set==-1 = �I�t */
/*   set==+2 = �I��/�I�t���] */
/* �@�@�@�@VK_DBE_SBCSCHAR      = �t���O�̂݃g�O�� */
/* �@�@�@�@VK_DBE_DBCSCHAR      = ���� */
/* �@�@�@�@VK_DBE_HIRAGANA      = �t���O�̂݃I�� */
static int NSIMEDLL_ToggleIME( HWND hwapp ,int set )
{
	static int togglebusy = 0; /* ���d�Ăяo���h�~ */

	int rc = 0; /* �h�l�d���I�������ۂ� +1�A�I�t�����ۂ� -1 ��Ԃ� */
	HIMI himi;


	if( togglebusy ){
		return togglebusy;
	}

	togglebusy = set==0 ? imeset : set==+2 ? -imeset : set;

	/* �h�l�d�Ƃ̂��Ƃ���s�����߂̏��� */
	if( !ImGetInstance( hwapp ,&himi ) ){
		/* �h�l�d�̌��݂̏�Ԃ��擾 */
		IMMODE immode;
		ImQueryIMMode( himi ,&immode.ulInputMode ,&immode.ulConversionMode );
		if( !(immode.ulInputMode&IMI_IM_IME_DISABLE) ){
			/* �h�l�d���g�p�\�ł��� */
			if( set==+2 ) set = (immode.ulInputMode&IMI_IM_IME_ON) ? -1 : +1;
			switch( set ){
			 case 0: /* ���s�̂h�l�d�̃I��/�I�t��Ԃ�Ԃ� */
				rc = (immode.ulInputMode&IMI_IM_IME_ON) ? +1 : -1;
				break;
			 case +1: /* �h�l�d���I���ɂ��� */
				immode.ulInputMode = IMI_IM_IME_ON | Status.mslike.mode.active;
				immode.ulConversionMode = IMI_CM_NONE;
				ImSetIMMode( himi ,immode.ulInputMode ,immode.ulConversionMode );
				rc = +1;
				break;
			 case -1: /* �h�l�d���I�t�ɂ��� */
				immode.ulInputMode = Status.mslike.mode.deactive;
				immode.ulConversionMode = IMI_CM_NONE;
				ImSetIMMode( himi ,immode.ulInputMode ,immode.ulConversionMode );
				rc = WinIsWindowShowing(hwimemode) ? +1 : -1;
				/* ���h�l�d�I�t�����ɁA���ۂɃI�t�ɂȂ������ǂ������m�F���A */
				/* �@���̌��ʂ�߂�l�ɂ���B */
				break;
			}
		}
		ImReleaseInstance( hwapp ,himi );
	}

	togglebusy = 0;
	return rc;
}


/* �h�l�d�I��/�I�t���삪�F�߂��Ă���E�B���h�E�ł��邩�`�F�b�N */
static int NSIMEDLL_IsEnableIME( HWND hwapp )
{
	RECTL rect;
	int rc = SHORT1FROMMR(WinSendMsg( hwapp ,WM_QUERYCONVERTPOS ,MPFROMP(&rect) ,0 ))!=QCP_NOCONVERT;
	if(rc){
		char cls[5];
		if( WinQueryClassName( hwapp ,sizeof(cls) ,cls )==3 && cls[0]=='#' ){
			/* �}���`���f�B�A(MMPM)�֌W�̃E�B���h�E�E�N���X�́A���Ӗ��� */
			/* �u�h�l�d�����F�߂�v�Ƃ������X�|���X��Ԃ��Ă���̂ŁA */
			/* �u�F�߂Ȃ��v�Ƃ������X�|���X��s������ */
			int val = ((cls[1]-'0') * 10 + (cls[2]-'0'));
			rc = val<0x40 || val>0x4F; /* WC_MMPMFIRST�`WC_MMPMLAST */
		}
	}
	return rc;
}


/* �h�l�d����{�^���y�тh�l�d���[�h�\���̃E�B���h�E���������� */
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


/* �h�l�d����{�^����\��/��\������ */
static void ShowImeStatusWindow( HWND hwnd ,BOOL flg )
{
	if(flg){
		/* �\���́A�h�l�d�p�̂`�o�h��p���čs�� */
		HIMI himi;
		if( !ImGetInstance( hwnd ,&himi ) ){
			ImShowStatusWindow( himi ,flg );
			ImReleaseInstance( hwnd ,himi );
		}
	}
	else{
		/* ��\���́A���ʂ̃E�B���h�E�`�o�h��p���čs�� */
		/* �i�[���Ƃ����ƁA�h�l�d�p�̂`�o�h��p���ď�������ƁA */
		/* NSIME ���I�������������\������Ԃ���������Ȃ����� */
		WinShowWindow(hwimeshift,FALSE);
	}
}


/* �b���C�u�������g��Ȃ��̂ŁA���O�ŗp�� */
static void __memcpy__( void* dst ,const void* src ,unsigned int cnt )
{
	const char* sp;
	char* dp;
	for( sp = (const char*)src ,dp = (char*)dst ;cnt ;cnt-- ){
		*(dp++) = *(sp++);
	}
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
