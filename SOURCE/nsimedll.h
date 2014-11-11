/* NSIMEの動作設定などの情報を保存する構造体 */
typedef struct{
	ULONG siz;				/* 構造体のサイズ */
	ULONG version;			/* バージョン番号 */
	ULONG revision;			/* 改訂番号 */
	struct{
		ULONG enable;		/* MS-IME互換のＩＭＥオン/オフ操作 */
		ULONG delay;		/* IME制御ボタンの状態を監視 */
		struct{
			ULONG active;		/* ＩＭＥオン時の入力モード */
			ULONG deactive;		/* ＩＭＥオフ時の入力モード */
		} mode;
	} mslike;
	ULONG visible;			/* ＩＭＥ制御ボタン表示 */
	HWND hwnd;				/* クライアント・ウィンドウ・ハンドル */
	struct{
		ULONG x;			/* フレーム・ウィンドウの位置 */
		ULONG y;
		ULONG cx;
		ULONG cy;
	} pos;
} NSIMESTATUS_T;


/* ＩＭＥ制御ボタン表示 */
#define NSIME_VISIBLE_ALWAYS 0	/* 常に表示 */
#define NSIME_VISIBLE_CANUSE 1	/* ＩＭＥ使用可能時および使用時 */
#define NSIME_VISIBLE_ONLYUSE 2	/* ＩＭＥ使用時だけ */

#define NSIMESTATUS_DEFAULT { sizeof(NSIMESTATUS_T) ,NSIME_VERSION_VAL ,NSIME_REVISION_CHR ,{ 1 ,0 ,{ 0 ,0 } } ,NSIME_VISIBLE_CANUSE ,(HWND)NULL ,{ 0 ,0 ,0 ,0 } }


typedef BOOL (EXPENTRY NSIMEDLL_ISEXISTIME)(void);
typedef BOOL (EXPENTRY NSIMEDLL_SETSTATUS)( const NSIMESTATUS_T* sta );
typedef BOOL (EXPENTRY NSIMEDLL_INPUTPROC)( HAB hab ,PQMSG pqmsg ,ULONG fs );
typedef VOID (EXPENTRY NSIMEDLL_SENDMSGPROC)( HAB hab ,PSMHSTRUCT psmh ,BOOL fInterTask );

