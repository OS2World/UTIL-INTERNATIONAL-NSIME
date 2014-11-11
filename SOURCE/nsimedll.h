/* NSIME�̓���ݒ�Ȃǂ̏���ۑ�����\���� */
typedef struct{
	ULONG siz;				/* �\���̂̃T�C�Y */
	ULONG version;			/* �o�[�W�����ԍ� */
	ULONG revision;			/* �����ԍ� */
	struct{
		ULONG enable;		/* MS-IME�݊��̂h�l�d�I��/�I�t���� */
		ULONG delay;		/* IME����{�^���̏�Ԃ��Ď� */
		struct{
			ULONG active;		/* �h�l�d�I�����̓��̓��[�h */
			ULONG deactive;		/* �h�l�d�I�t���̓��̓��[�h */
		} mode;
	} mslike;
	ULONG visible;			/* �h�l�d����{�^���\�� */
	HWND hwnd;				/* �N���C�A���g�E�E�B���h�E�E�n���h�� */
	struct{
		ULONG x;			/* �t���[���E�E�B���h�E�̈ʒu */
		ULONG y;
		ULONG cx;
		ULONG cy;
	} pos;
} NSIMESTATUS_T;


/* �h�l�d����{�^���\�� */
#define NSIME_VISIBLE_ALWAYS 0	/* ��ɕ\�� */
#define NSIME_VISIBLE_CANUSE 1	/* �h�l�d�g�p�\������юg�p�� */
#define NSIME_VISIBLE_ONLYUSE 2	/* �h�l�d�g�p������ */

#define NSIMESTATUS_DEFAULT { sizeof(NSIMESTATUS_T) ,NSIME_VERSION_VAL ,NSIME_REVISION_CHR ,{ 1 ,0 ,{ 0 ,0 } } ,NSIME_VISIBLE_CANUSE ,(HWND)NULL ,{ 0 ,0 ,0 ,0 } }


typedef BOOL (EXPENTRY NSIMEDLL_ISEXISTIME)(void);
typedef BOOL (EXPENTRY NSIMEDLL_SETSTATUS)( const NSIMESTATUS_T* sta );
typedef BOOL (EXPENTRY NSIMEDLL_INPUTPROC)( HAB hab ,PQMSG pqmsg ,ULONG fs );
typedef VOID (EXPENTRY NSIMEDLL_SENDMSGPROC)( HAB hab ,PSMHSTRUCT psmh ,BOOL fInterTask );

