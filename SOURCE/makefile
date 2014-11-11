# nsime.exe 及び nsimedll.dll の両方をビルドするためのメイクファイル
# （IBM-C Set++用）
#
# 前提としている開発環境
#
# 　アセンブラ：MASM 6.00
# 　　　　　（市販品）
# 　Ｃコンパイラ：IBM-C Set++ 2.01
# 　　　　　（同上）
# 　リンカ：LINK386
# 　　　　　（ツールキットまたはＯＳに付属）
# 　リソースコンパイラ：RC
# 　　　　　（同上）
# 　メイク：NMAKE
# 　　　　　（ツールキットに付属、あるいは Hobbes 等で入手可能）
# 　その他：日本語版 WARP4 に付属の J-Pocket に含まれているＩＭ用ツールキット
# 　　　　　（os2im.h 及び os2im.lib を、それぞれインクルード/ライブラリの
# 　　　　　ディレクトリにコピーする）
# 　　　　　REXX
# 　　　　　（ＯＳに付属）
# 　　　　　SED
# 　　　　　（Hobbes 等で入手可能）
!include "version.mak"

# ツール定義
#
AS		=	ml.exe
CC		=	icc.exe
LINK	=	link386.exe
# !undef AS
# ↑この行のコメントを取り除くと、アセンブラが必要なソース(_start2.asm)を除いて
# 　ビルドを行います。
# 　アセンブラを持っていない方、あるいはＣライブラリを使いたい方のために設けて
# 　あります。
# 　なお、その場合はＣライブラリのデフォルトのスタートアップコードを用いるので、
# 　実行ファイルサイズが少し大きくなります。

CFLAGS	=	/C /O- /Q /Sn /Ss \
			/Wall /Wppt- /Wgnr- /Wcpy- /Wcls- /Wtrd- /Wvft- /Wcmp- /Wuni- /Wobs- /Wpor- /Wext- /Wpar-
LFLAGS	=	/NOLOGO /ALIGN:2 /NOE /NOI /BATCH
!ifdef AS
AFLAGS	=	/c /nologo
CFLAGS	=	$(CFLAGS) /Dmain=MAIN
LFLAGS	=	$(LFLAGS) /NOD
!endif


# オブジェクトファイル定義
#
EXEOBJS	=	nsime.obj
DLLOBJS	=	nsimedll.obj
!ifdef AS
EXEOBJS	=	_start2.obj $(EXEOBJS)
!endif


# ビルド手順定義
#
nsime.exe: $(EXEOBJS) nsimedll.dll nsime.def nsimetmp.res
	$(LINK) $(LFLAGS) $(EXEOBJS) ,$@ ,nul.map ,os2386.lib ,nsime.def
	rc -p -x2 nsimetmp.res $@

nsimedll.dll: $(DLLOBJS) nsimedll.def
	$(LINK) $(LFLAGS) $(DLLOBJS) ,$@ ,nul.map ,os2386.lib os2im.lib ,nsimedll.def

.SUFFIXES:
.SUFFIXES: .asm .c

.asm.obj:
	$(AS) $(AFLAGS) $?

.c.obj:
	$(CC) $(CFLAGS) $?

nsime.c: nsimedll.h nsimeres.h nsimedlg.h nsimever.h

nsimedll.c: nsimedll.h nsimever.h

nsimetmp.res: nsimetmp.rc nsimeres.h nsimedlg.h nsimeico.ico
	rc -r nsimetmp.rc $@

nsimetmp.rc: nsimeres.rc nsimedlg.dlg
	sed -e "s/DLGINCLUDE [0-9]*/#include/" nsimeres.rc nsimedlg.dlg > $@
#
# 　nsimedlg.h 及び nsimedlg.dlg（及び nsimedlg.res）は、OS/2 ツール
# キットの「ダイアログ・エディター(DLGEDIT.EXE)」で作成したファイル
# です。
# 　nsimedlg.res は、メイクには必要無いのですが、「ダイアログ・エデ
# ィター」で編集する事を念頭において同梱してあります。
#

!if "$(REVISION)"==""
REVISIONCHR	=	'\0'
!else
REVISIONCHR	=	'$(REVISION)'
!endif

nsimever.h: version.mak
	if exist $@ del $@
	rexxtry s=left('$(VERSION)',1)'.'substr('$(VERSION)',2); \
		if right(s,1)='0' then s=delstr(s,length(s)); \
		call lineout '$@' ,'#define NSIME_VERSION_VAL 0x$(VERSION)'; \
		call lineout '$@' ,'#define NSIME_VERSION_STR "'s'"'; \
		call lineout '$@' ,"#define NSIME_REVISION_CHR $(REVISIONCHR)"; \
		call lineout '$@' ,'#define NSIME_REVISION_STR "$(REVISION)"'; \
		>nul
#
# 　バージョン番号の管理は、メイクファイルのインクルードファイル
# version.mak で行っており、上の記述は、それを nsimever.h に反映
# させる手順です。
# 　何故わざわざメイクファイルを介してバージョン番号管理を行って
# いるのかは、作者の個人的都合と言う事で御容赦を。(^^ゞ
#
# 　なお、emx/gcc(GNU-MAKE) でメイクする場合は、version.mak は
# 参照せずにメイクを行います。
# （詳細は makefile.emx を参照のこと）
#

