#ifndef XM_H

/**
*  Функции контроля за использованием динамической памяти.
*  Сделано модификацией программ из статьи
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
*  INCLUDE-файлы
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
*  Для трансляции с включенным контролем использования хипа
*   нужно употребить строку #define XM_USED перед включением файла XM.H.
*  В противном случае x_calloc, x_malloc и x_free
*   будут простыми обращениями к стандартным функциям Си -
*   конечно, без контроля. Остальные объявленные здесь функции
*   работать будут, но с малоинтересными результатами.
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
*   Константы и глобальные переменные  *
****************************************/

/**
*  Коды ошибок использования хипа:
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
*  Устанавливает уровень контроля за использованием хипа. Значения:
*    NO_HEAPCTL - никакого контроля
*	(простые обращения к стандартным функциям и подсчет статистики;
*	 фиксируется только освобождение NULL);
*    FREE_CTL - контроль за повторным освобождением и освобождением
*     	нераспределявшейся памяти;
*    FULL_CTL - то же, что FREE_CTL, плюс контроль за записью
*     поверх границ распределенных участков и в уже освобожденные
*     участки.
*  По умолчанию установлена в FULL_CTL.
**/

  extern unsigned long _Cdecl xm_userMemory;
/**
*  Количество на текущий момент памяти,
*   выделенной пользователю при помощи функций этого модуля
*	(без учета служебной памяти Си и этого модуля;
*   	 освобождаемая память из подсчета исключается).
**/

  extern unsigned long _Cdecl xm_totalMemory;
/**
*  Количество на текущий момент памяти,
*   используемой функциями этого модуля:
*	xm_userMemory + память, необходимая для работы xm-модуля
*   (т.е. без учета служебной памяти Си,
*   но с учетом памяти, необходимой для работы этого модуля;
*   освобождаемая память из подсчета исключается).
**/


  extern unsigned long _Cdecl xm_userAlloc;
/**
*  Количество на текущий момент выделенных пользователю участков хипа
*   (освобождаемые участки из подсчета исключаются).
**/

  extern unsigned long _Cdecl xm_totalAlloc;
/*
*  Общее число выделенных модулем участков хипа
*    (освобождаемые участки из подсчета НЕ исключаются)
*   Переменная устанавливается в нуль в функции xm_XfreeMem.
*   Кроме того, может быть уменьшена в функции xm_Xrelease.
*/


  extern void (*_Cdecl xm_heapWng) (unsigned int code, void *ptr);
/*
*  Функция реакции на ошибку в использовании хипа.
*  Параметр code - код ошибки. Стандартный текст сообщения
*   можно получить из массива xm_wngText; индекс - код ошибки.
*  Параметр ptr - "криминальный" указатель.
*
*  Стандартная реакция (начальная установка xm_heapWng) - выдача
*   сообщения и указателя (в 16-ричном виде) в файл stderr.
*/

  extern char *_Cdecl xm_wngText[];
/*
*  Тексты сообщений об ошибках. Ошибке с кодом i соответствует
*   строка xm_wngText[i].
*/
#define XM_WNGTEXT  {	          			\
       NULL,                                            \
       "Запись вне отведенного блока",                  \
       "Освобождение ранее освобожденного блока",       \
       "Освобождение невыделенного блока",              \
       "Освобождение с указателем, равным NULL",        \
       "Блок менялся после освобождения",               \
       "Неосвобожденный блок"                           \
}

    /**/
/*
*   Объявления функций
*/
   extern void *_Cdecl xm_Xmalloc (size_t);
/*
*  То же, что malloc
*/

  extern void *_Cdecl xm_Xcalloc (size_t, size_t);
/*
*  То же, что calloc
*/

  extern void _Cdecl xm_Xfree (void *);
/*
*  xm-аналог библиотечной функции free.
*  Возможен вызов xm_heapWng в следующих ситуациях:
*   - освобождение NULL (код FREEING_NULL_WNG, выдается при любых значениях
*      xm_Trace);
*   - освобождение некорректного указателя (FREEING_BAD_WNG, выдается при
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - повторное освобождение участка (FREEING_FREE_WNG, выдается при
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - производилась запись за границы освобождаемого участка
*      (OVERWRITE_WNG, xm_Trace == FULL_CTL).
*/

#define xm_strdup(s) strcpy(xm_malloc(strlen(s)+1),s)
/*
*  xm-аналог библиотечной функции strdup
*/

  extern int _Cdecl xm_XMem (int freeing, unsigned long start);
/*
*  Проверка состояния хипа. Считается, что к моменту обращения к
*   xm_XMem все, что распределялось через этот модуль, должно
*   быть освобождено.
*  Вызывается xm_heapWng по следующим поводам:
*   - участок не освобожден (код FREEING_FREE, выдается при
*      xm_Trace == FREE_CTL || xm_Trace == FULL_CTL);
*   - участок изменен после освобождения (код CHANGED_FREE_WNG,
*      выдается при xm_Trace == FULL_CTL).
*  Если параметр freeing равняется 1, освобождает память,
*    захваченную после "момента", который определен параметром start
*    (при этом числе освобождается и память, используемая модулем
*    для "внутренних" нужд).
*  В противном случае ограничивается проверкой такой памяти.
*  Парной является xm_mark, которая используется для того, чтобы
*    запомнить "место", начиная с которого (и до конца) нужно освобождать.
*  Функция возвращает 1, если все было в порядке (ни разу не пришлось
*	вызвать xm_heapWng).
*  В противном случае функция возвращает 0.
*/

#ifdef __cplusplus
}
#endif

#define  XM_H

#endif
