# nsime.exe �y�� nsimedll.dll �̗������r���h���邽�߂̃��C�N�t�@�C��
# �iIBM-C Set++�p�j
#
# �O��Ƃ��Ă���J����
#
# �@�A�Z���u���FMASM 6.00
# �@�@�@�@�@�i�s�̕i�j
# �@�b�R���p�C���FIBM-C Set++ 2.01
# �@�@�@�@�@�i����j
# �@�����J�FLINK386
# �@�@�@�@�@�i�c�[���L�b�g�܂��͂n�r�ɕt���j
# �@���\�[�X�R���p�C���FRC
# �@�@�@�@�@�i����j
# �@���C�N�FNMAKE
# �@�@�@�@�@�i�c�[���L�b�g�ɕt���A���邢�� Hobbes ���œ���\�j
# �@���̑��F���{��� WARP4 �ɕt���� J-Pocket �Ɋ܂܂�Ă���h�l�p�c�[���L�b�g
# �@�@�@�@�@�ios2im.h �y�� os2im.lib ���A���ꂼ��C���N���[�h/���C�u������
# �@�@�@�@�@�f�B���N�g���ɃR�s�[����j
# �@�@�@�@�@REXX
# �@�@�@�@�@�i�n�r�ɕt���j
# �@�@�@�@�@SED
# �@�@�@�@�@�iHobbes ���œ���\�j
!include "version.mak"

# �c�[����`
#
AS		=	ml.exe
CC		=	icc.exe
LINK	=	link386.exe
# !undef AS
# �����̍s�̃R�����g����菜���ƁA�A�Z���u�����K�v�ȃ\�[�X(_start2.asm)��������
# �@�r���h���s���܂��B
# �@�A�Z���u���������Ă��Ȃ����A���邢�͂b���C�u�������g���������̂��߂ɐ݂���
# �@����܂��B
# �@�Ȃ��A���̏ꍇ�͂b���C�u�����̃f�t�H���g�̃X�^�[�g�A�b�v�R�[�h��p����̂ŁA
# �@���s�t�@�C���T�C�Y�������傫���Ȃ�܂��B

CFLAGS	=	/C /O- /Q /Sn /Ss \
			/Wall /Wppt- /Wgnr- /Wcpy- /Wcls- /Wtrd- /Wvft- /Wcmp- /Wuni- /Wobs- /Wpor- /Wext- /Wpar-
LFLAGS	=	/NOLOGO /ALIGN:2 /NOE /NOI /BATCH
!ifdef AS
AFLAGS	=	/c /nologo
CFLAGS	=	$(CFLAGS) /Dmain=MAIN
LFLAGS	=	$(LFLAGS) /NOD
!endif


# �I�u�W�F�N�g�t�@�C����`
#
EXEOBJS	=	nsime.obj
DLLOBJS	=	nsimedll.obj
!ifdef AS
EXEOBJS	=	_start2.obj $(EXEOBJS)
!endif


# �r���h�菇��`
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
# �@nsimedlg.h �y�� nsimedlg.dlg�i�y�� nsimedlg.res�j�́AOS/2 �c�[��
# �L�b�g�́u�_�C�A���O�E�G�f�B�^�[(DLGEDIT.EXE)�v�ō쐬�����t�@�C��
# �ł��B
# �@nsimedlg.res �́A���C�N�ɂ͕K�v�����̂ł����A�u�_�C�A���O�E�G�f
# �B�^�[�v�ŕҏW���鎖��O���ɂ����ē������Ă���܂��B
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
# �@�o�[�W�����ԍ��̊Ǘ��́A���C�N�t�@�C���̃C���N���[�h�t�@�C��
# version.mak �ōs���Ă���A��̋L�q�́A����� nsimever.h �ɔ��f
# ������菇�ł��B
# �@���̂킴�킴���C�N�t�@�C������ăo�[�W�����ԍ��Ǘ����s����
# ����̂��́A��҂̌l�I�s���ƌ������Ō�e�͂��B(^^�U
#
# �@�Ȃ��Aemx/gcc(GNU-MAKE) �Ń��C�N����ꍇ�́Aversion.mak ��
# �Q�Ƃ����Ƀ��C�N���s���܂��B
# �i�ڍׂ� makefile.emx ���Q�Ƃ̂��Ɓj
#
