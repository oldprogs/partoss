#ifndef XM_H

/**
*  �㭪樨 ����஫� �� �ᯮ�짮������ �������᪮� �����.
*  ������� ����䨪�樥� �ணࠬ� �� ����
*    Extended Dynamic Memory Control Module,
*     Dr. Dobb's Journal, #154 August 1989.
*
*  xm_Trace
*  xm_totalMemory
*  xm_totalAlloc
*  xm_userAlloc
*  xm_heapWng
*  x_malloc
*  x_calloc
*  x_free
*  x_checkMem
*  x_freeMem
*  x_mark
*  x_release
**/

/*
*  INCLUDE-䠩��
*/

#include <malloc.h>
/* size_t, calloc, malloc, free */

#include <string.h>
/* strcpy, strlen */

#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
*  ��� �࠭��樨 � ����祭�� ����஫�� �ᯮ�짮����� 娯�
*   �㦭� 㯮�ॡ��� ��ப� #define XM_USED ��। ����祭��� 䠩�� XM.H.
*  � ��⨢��� ��砥 x_calloc, x_malloc � x_free
*   ���� ����묨 ���饭�ﬨ � �⠭����� �㭪�� �� -
*   ����筮, ��� ����஫�. ��⠫�� ������� ����� �㭪樨
*   ࠡ���� ����, �� � ���������묨 १���⠬�.
*/
#ifndef XM_USED

#define x_calloc        calloc
#define x_malloc        malloc
#define x_free          free
#define x_strdup	strdup
#define x_checkMem()	1
#define x_freeMem()	1
#define x_release()	1
#define x_mark(p)	1

#else

#define x_calloc        xm_Xcalloc
#define x_malloc        xm_Xmalloc
#define x_free          xm_Xfree
#define x_strdup(s)	strcpy(xm_Xmalloc(strlen(s)+1),s)
#define x_checkMem()    xm_XMem(0,0)
#define x_freeMem()     xm_XMem(1,0)
#define x_release(p)    xm_XMem(xm_Trace==FREE_CTL,p)
#define x_mark(p)       p=xm_totalAlloc
#define xm_setOwnWarn(OwnWrn)   (xm_heapWng =(OwnWrn))

#endif

/***************************************
*   ����⠭�� � �������� ��६����  *
****************************************/

/**
*  ���� �訡�� �ᯮ�짮����� 娯�:
**/
#define OVERWRITE_WNG     1
#define FREEING_FREE_WNG  2
#define FREEING_BAD_WNG   3
#define FREEING_NULL_WNG  4
#define CHANGED_FREE_WNG  5
#define UNFREED_WNG       6

#define NO_HEAPCTL 0
#define FREE_CTL   1
#define FULL_CTL   2
  extern unsigned int _Cdecl xm_Trace;
/**
*  ��⠭�������� �஢��� ����஫� �� �ᯮ�짮������ 娯�. ���祭��:
*    NO_HEAPCTL - �������� ����஫�
*	(����� ���饭�� � �⠭����� �㭪�� � ������ ����⨪�;
*	 䨪������ ⮫쪮 �᢮�������� NULL);
*    FREE_CTL - ����஫� �� ������ �᢮��������� � �᢮���������
*     	����।���襩�� �����;
*    FULL_CTL - � ��, �� FREE_CTL, ���� ����஫� �� �������
*     ������ �࠭�� ��।������� ���⪮� � � 㦥 �᢮��������
*     ���⪨.
*  �� 㬮�砭�� ��⠭������ � FULL_CTL.
**/

  extern unsigned long _Cdecl xm_userMemory;
/**
*  ������⢮ �� ⥪�騩 ������ �����,
*   �뤥������ ���짮��⥫� �� ����� �㭪権 �⮣� �����
*	(��� ��� �㦥���� ����� �� � �⮣� �����;
*   	 �᢮��������� ������ �� ������ �᪫�砥���).
**/

  extern unsigned long _Cdecl xm_totalMemory;
/**
*  ������⢮ �� ⥪�騩 ������ �����,
*   �ᯮ��㥬�� �㭪�ﬨ �⮣� �����:
*	xm_userMemory + ������, ����室���� ��� ࠡ��� xm-�����
*   (�.�. ��� ��� �㦥���� ����� ��,
*   �� � ��⮬ �����, ����室���� ��� ࠡ��� �⮣� �����;
*   �᢮��������� ������ �� ������ �᪫�砥���).
**/


  extern unsigned long _Cdecl xm_userAlloc;
/**
*  ������⢮ �� ⥪�騩 ������ �뤥������ ���짮��⥫� ���⪮� 娯�
*   (�᢮�������� ���⪨ �� ������ �᪫������).
**/

  extern unsigned long _Cdecl xm_totalAlloc;
/*
*  ��饥 �᫮ �뤥������ ���㫥� ���⪮� 娯�
*    (�᢮�������� ���⪨ �� ������ �� �᪫������)
*   ��६����� ��⠭���������� � ��� � �㭪樨 xm_XfreeMem.
*   �஬� ⮣�, ����� ���� 㬥��襭� � �㭪樨 xm_Xrelease.
*/


  extern void (*_Cdecl xm_heapWng) (unsigned int code, void *ptr);
/*
*  �㭪�� ॠ�樨 �� �訡�� � �ᯮ�짮����� 娯�.
*  ��ࠬ��� code - ��� �訡��. �⠭����� ⥪�� ᮮ�饭��
*   ����� ������� �� ���ᨢ� xm_wngText; ������ - ��� �訡��.
*  ��ࠬ��� ptr - "�ਬ������" 㪠��⥫�.
*
*  �⠭���⭠� ॠ��� (��砫쭠� ��⠭���� xm_heapWng) - �뤠�
*   ᮮ�饭�� � 㪠��⥫� (� 16-�筮� ����) � 䠩� stderr.
*/

  extern char *_Cdecl xm_wngText[];
/*
*  ������ ᮮ�饭�� �� �訡���. �訡�� � ����� i ᮮ⢥�����
*   ��ப� xm_wngText[i].
*/
#define XM_WNGTEXT  {	          			\
       NULL,                                            \
       "������ ��� �⢥������� �����",                  \
       "�᢮�������� ࠭�� �᢮���������� �����",       \
       "�᢮�������� ���뤥������� �����",              \
       "�᢮�������� � 㪠��⥫��, ࠢ�� NULL",        \
       "���� ������ ��᫥ �᢮��������",               \
       "���᢮�������� ����"                           \
}

    /**/
/*
*   ������� �㭪権
*/
   extern void *_Cdecl xm_Xmalloc (size_t);
/*
*  �� ��, �� malloc
*/

  extern void *_Cdecl xm_Xcalloc (size_t, size_t);
/*
*  �� ��, �� calloc
*/

  extern void _Cdecl xm_Xfree (void *);
/*
*  xm-������ �������筮� �㭪樨 free.
*  �������� �맮� xm_heapWng � ᫥����� ������:
*   - �᢮�������� NULL (��� FREEING_NULL_WNG, �뤠���� �� ���� ���祭���
*      xm_Trace);
*   - �᢮�������� �����४⭮�� 㪠��⥫� (FREEING_BAD_WNG, �뤠���� ��
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - ����୮� �᢮�������� ���⪠ (FREEING_FREE_WNG, �뤠���� ��
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - �ந��������� ������ �� �࠭��� �᢮���������� ���⪠
*      (OVERWRITE_WNG, xm_Trace == FULL_CTL).
*/

#define xm_strdup(s) strcpy(xm_malloc(strlen(s)+1),s)
/*
*  xm-������ �������筮� �㭪樨 strdup
*/

  extern int _Cdecl xm_XMem (int freeing, unsigned long start);
/*
*  �஢�ઠ ���ﭨ� 娯�. ��⠥���, �� � ������� ���饭�� �
*   xm_XMem ��, �� ��।��﫮�� �१ ��� �����, ������
*   ���� �᢮�������.
*  ��뢠���� xm_heapWng �� ᫥���騬 �������:
*   - ���⮪ �� �᢮������ (��� FREEING_FREE, �뤠���� ��
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - ���⮪ ������� ��᫥ �᢮�������� (��� CHANGED_FREE_WNG,
*      �뤠���� �� xm_Trace == FULL_CTL).
*  �᫨ ��ࠬ��� freeing ࠢ����� 1, �᢮������� ������,
*    ��墠祭��� ��᫥ "������", ����� ��।���� ��ࠬ��஬ start
*    (�� �⮬ �᫥ �᢮��������� � ������, �ᯮ��㥬�� ���㫥�
*    ��� "����७���" �㦤).
*  � ��⨢��� ��砥 ��࠭�稢����� �஢�મ� ⠪�� �����.
*  ��୮� ���� xm_mark, ����� �ᯮ������ ��� ⮣�, �⮡�
*    ��������� "����", ��稭�� � ���ண� (� �� ����) �㦭� �᢮�������.
*  �㭪�� �����頥� 1, �᫨ �� �뫮 � ���浪� (�� ࠧ� �� ��諮��
*	�맢��� xm_heapWng).
*  � ��⨢��� ��砥 �㭪�� �����頥� 0.
*/

#ifdef __cplusplus
}
#endif

#define  XM_H

#endif
