/**************************************************************************
 * File: linkarea.cpp
 *
 * Copyright (C) 1998 by Vadim Belman (voland@plab.ku.dk)
 * Licensed for use in The Parma Tosser by Serge Koghin.
 *
 * Links a Squish(TM)-style message base in four different ways with the
 * highest possible speed.
 *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#if !defined (__LNX__) && !defined (__FreeBSD__)
#include <io.h>
#endif
#include <sys/timeb.h>
#include "globext.h"
#include "partoss.h"

#define LINKTYPE_REPLY          1
#define LINKTYPE_SUBJ           2
#define LINKTYPE_FLAT           3
#define LINKTYPE_TIMED          4
#define LINKTYPE_NONE           5

// Здесь число заменяется на значение из конфига при встраивании в
// Портоса. bcfg.linklength, так кажется.
#define LINK_LENGTH             bcfg.linklength
// Если subj в заголовке может не оканчиваться 0, то здесь надо вписать 72
#define SUBJ_MAXLEN             71

// myalloc и myfree - временные заглушки, чтобы не привязываться к главным сырцам.
// Несколько напрягает отсутсвие myrealloc - потенциальная проблема для дос-версии.
#define myalloc( sz, f, l)  malloc( sz)
#define myfree( pptr, f, l) free( *( pptr))
#define myrealloc( ptr, sz, f, l) realloc( ( ptr), ( sz))

// O_BINARY может быть не определен для не-DOS-derived библиотек (гарантированно -
// в libc BSD-семейства, например). Чтобы не пудрить себе мозги потом - определим
// как пустышку.
#ifndef O_BINARY
#define O_BINARY            0x0000
#endif

// Та же фигня - с _MAX_PATH. Пока берем определения из BSD'шного libc.
// Там видно будет.
#ifndef _MAX_PATH
#define _MAX_PATH           MAXPATHLEN
#endif

// Используется для проверки целостности заголовка фрейма в .SQD
#define SQHDRID       0xafae4453L

/*
// time_diff и time_point могут быть использованы для профилирования
// кусков кода, когда счет времени идет на миллисекунды.
float timeb_diff( struct timeb &t1, struct timeb &t2)
{
    float tfm = ( float( t1.millitm) - float( t2.millitm)) / 1000., tft = float( t1.time) - float( t2.time);
    return tfm + tft;
}

float time_point()
{
    static struct timeb pt;
    struct timeb curt;
    float diff;

    ftime( &curt);

    diff = timeb_diff( curt, pt);

    memcpy( &pt, &curt, sizeof( struct timeb));

    return diff;
}
*/

//=========================================================================
// Working code
//=========================================================================

// Для заполнения хэша в качестве ключа достаточно использовать MSGID сам
// по себе - он достаточно случаен.

typedef struct MsgInfo
{
  sqdmshead hdr;		// Собственно заголовок мессаги
  unsigned long iidx;		// Смещение в массиве индексов
  // Нижеследующее берется из индекса - чтоб мозги себе потом не пудрить
  // с увязкой индексов.
  sqifile index_info;
  // Теперь то, что просто может пригодиться.
  unsigned long msgid, reply_id;
  int changed;
  int processed;
  // Далее поля для типов линковки Flat/Reply и Subj. В общем - для
  // линейных, где нужны списки.
  MsgInfo *next, *prev;		// Указатели на следующее и предыдущее письма в
  // цепочке.
  MsgInfo *head;		// Указатель на голову списка.
  MsgInfo *last;		// Указатель на последний элемент в списке.
  // Действителен только у головного элемента списка.
}
 *PMsgInfo;

typedef struct HashElem
{
  PMsgInfo msg_info;
  HashElem *next;
}
 *PHashElem;

typedef struct Hash
{
  unsigned long size;		// Размер таблицы.
  unsigned long used;		// Число использованных слотов.
  unsigned long dupcount;	// Число слотов с более чем одним элементом.
  unsigned long elements;	// Число занесенных элементов.
  // Последние два поля - чисто для статистики.
  PHashElem *table;		// Собственно хэш-таблица, она же - массив
  // указателей на списки элементов.
  unsigned long (*hashFunc) (unsigned long, PMsgInfo);
  int (*cmpFunc) (PMsgInfo, PMsgInfo);
}
 *PHash;

//-------------------------------------------------------------------------
// Куски работы с хэшем
//-------------------------------------------------------------------------

// Генерация простого числа для размера хэша.
unsigned long gen_prime (unsigned long base)
{
  unsigned long i;
  // Нет смысла искать среди четных чисел, поэтому подгоняем минимум
  // под следующее нечетное или оставляем как есть.
  unsigned long atLeast = base + (base % 2 ? 0 : 1);

  // Перебираем кандидатов. Редко когда доведется перебрать больше
  // десяти чисел. Собственно, прямой подсчет для все простых меньше
  // 100000 дает среднюю разницу между двумя соседними ~10.4.
  // Правда для миллиона это значение уже ~12.7, но покажите мне такую
  // базу! 8)
  for (i = atLeast;; i += 2)
    {
      // Если число и имеет делитель - то он явно меньше корня квадратного
      // этого числа.
      unsigned long divisor = (unsigned long)sqrt (i);
      // Подгоняем делитель под ближайшее сверху значение, которое допустимо
      // для перебора делителей. Допустимо - значи не кратно ни 3, ни 2.
      // Поизгалявшись можно было бы еще и кратные 5 отфутболивать, но "и
      // так неплохо". Как результат divisor может стать чуть больше корня
      // квадратного проверяемого числа, но это уже не страшно.
      divisor = (divisor / 3) * 3 + 2;
      // Шаг подобран как раз такой, чтобы пропускать все, что кратно 2 и 3.
      // Другими словами, для любого не кратного 2 и 3 числа D число меньшее
      // D на произведение на 2 остатка от деления D на 3 - число не кратное
      // 2 и 3. Доказательство оставляется в качестве упражнения. 8)
      for (; divisor > 3; divisor -= 2 * (divisor % 3))
	{
	  // Если вдруг проверяемое число разделилось без остатка, значит
	  // оно нам не подходит.
	  if ((i % divisor) == 0)
	    {
	      break;
	    }
	}
      // Все делители перебраны, а число не разделилось нацело; значит -
      // простое.
      if (divisor <= 3)
	{
	  break;
	}
    }
  return i;
}

PHash createHash (unsigned long expectedElements,
		  unsigned long (*hashFunc) (unsigned long, PMsgInfo),
		  int (*cmpFunc) (PMsgInfo, PMsgInfo))
{
  PHash newHash;
  // Ориентировочно число элементов в хэш-таблице втрое большее чем
  // ожидаемое число элементов дает ~20% дубликатов (считая _все_ элементы).
  // Теория сообщает, что наивысшая эффективность хэша достигается при
  // его размере равном какому-либо простому числу.
  unsigned long hashSize = gen_prime (expectedElements * 3);

  if (expectedElements == 0)
    {
      return NULL;
    }

  newHash = (PHash) myalloc (sizeof (Hash), __FILE__, __LINE__);
  newHash->size = hashSize;
  newHash->dupcount = newHash->used = newHash->elements = 0;
  newHash->table =
    (PHashElem *) myalloc (hashSize * sizeof (PHashElem), __FILE__, __LINE__);
  newHash->hashFunc = hashFunc;
  newHash->cmpFunc = cmpFunc;
  memset (newHash->table, 0, hashSize * sizeof (PHashElem));

  return newHash;
}

void destroyHash (PHash hash)
{
  int i;

  for (i = 0; i < hash->size; i++)
    {
      PHashElem curelem = hash->table[i];
      if (curelem != NULL)
	{
	  PHashElem next;
	  do
	    {
	      next = curelem->next;
	      myfree ((void **)&curelem, __FILE__, __LINE__);
	      curelem = next;
	    }
	  while (curelem);
	}
    }

  myfree ((void **)&hash->table, __FILE__, __LINE__);
  myfree ((void **)&hash, __FILE__, __LINE__);
}

void insertElement (PHash hash, PMsgInfo msgInfo)
{
  PHashElem hashElem;
  unsigned long offset;

  hashElem = (PHashElem) myalloc (sizeof (HashElem), __FILE__, __LINE__);
  hashElem->msg_info = msgInfo;

  // Смещение в хэш-таблице принимаем равным остатку от деления MSGID на
  // размер таблицы. С учетом уникальности MSGID, его, вообще говоря,
  // независимости от остальных MSGID в базе и вообще в идеале - полной
  // случайности, такой вариант хэш-функции дает весьма неплохие результаты,
  // которые вполне нас устраивают с точки зрения как эффективности
  // распределения элементов, так и скорости работы.
  offset = hash->hashFunc (hash->size, msgInfo);

  if (hash->table[offset] == NULL)
    {
      // Отмечаем, что число исползованных слотов сейчас увеличится.
      hash->used++;
    }
  else
    {
      hash->dupcount++;
    }
  // Либо у нас ячейка свободна - и тогда next просто обнуляется. Либо
  // она кем-то уже занята - и тогда это дает нам вставку нового элемента
  // в голову списка.
  hashElem->next = hash->table[offset];
  hash->table[offset] = hashElem;
  hash->elements++;
}

// Поиск нужного элемента по MSGID.
PMsgInfo findElement (PHash hash, PMsgInfo msgInfo)
{
  unsigned long offset = hash->hashFunc (hash->size, msgInfo);

  PHashElem elem = hash->table[offset];

  while (elem && (!hash->cmpFunc (elem->msg_info, msgInfo)))
    {
      elem = elem->next;
    }

  // На левые вопросы даем левые ответы: если такого msgid в слоте нет,
  // так ему и взяться там неоткуда. Разве только в результате баги разные
  // хэш-функции были заюзаны.
  return (elem == NULL ? NULL : elem->msg_info);
}

// Hash-функции для pаботы по MSGID
unsigned long hfMSGID (unsigned long hashSize, PMsgInfo msgInfo)
{
  return msgInfo->msgid % hashSize;
}

int cmpMSGID (PMsgInfo m1, PMsgInfo m2)
{
  return m1->msgid == m2->msgid;
}

// Hash-функции для работы с umsgid
// Особенность: вообще говоря, я так понимаю, что umsgid == смещение
// в массиве индексов + 1. Однако, поскольку уверенности в этом нет,
// то лучше иметь хэш для поиска по ним, хотя практически наверняка
// они в этом хэше расположатся линейно.
unsigned long hfumsgid (unsigned long hashSize, PMsgInfo msgInfo)
{
  return msgInfo->index_info.umsgid % hashSize;
}

int cmpumsgid (PMsgInfo m1, PMsgInfo m2)
{
  return m1->index_info.umsgid == m2->index_info.umsgid;
}

// Hash-функции для работы с Subj.

// Функция возвращает позицию в subj, на которой заканчиваются лидирующие
// пробелы и 'Re:', т.е. позицию, в которой собственно subj и сидит.
int cleanSubjPos (const char *subj)
{
  int i = 0;
  // Если subj должен заканчиваться 0, то оный 0 можно просто скипать.
  // Поэтому i < 71 а не 72. Впрочем, это ж какой subj должен быть, чтобы
  // условие сработало???
  while (i < SUBJ_MAXLEN && isspace (subj[i])
	 || (strnicmp (subj + i, "RE:", 3) == 0))
    {
      if (isspace (subj[i]))
	{
	  i++;
	}
      else
	{
	  i += 3;
	}
    }

  return i;
}

int subjLen (const char *subj, int *pos)
{
  *pos = cleanSubjPos (subj);
  return ((SUBJ_MAXLEN - *pos) <
	  LINK_LENGTH ? (SUBJ_MAXLEN - *pos) : LINK_LENGTH);
}

unsigned long hfSubj (unsigned long hashSize, PMsgInfo msgInfo)
{
  int pos, blen;
  unsigned long crc;

  blen = subjLen (msgInfo->hdr.subj, &pos);
  crc = crc32block (msgInfo->hdr.subj + pos, blen, (unsigned long)-1);

  return crc % hashSize;
}

int cmpSubj (PMsgInfo m1, PMsgInfo m2)
{
  int len1, pos1, len2, pos2;

  len1 = subjLen (m1->hdr.subj, &pos1);
  len2 = subjLen (m2->hdr.subj, &pos2);

  return (len1 == len2)
    && (strncmp (m1->hdr.subj + pos1, m2->hdr.subj + pos2, len1) == 0);
}

//-------------------------------------------------------------------------
// Куски работы с базой
//-------------------------------------------------------------------------

// str2MSGID - взамен strtoul. Быстpее получается.
unsigned long str2MSGID (const char *str)
{
  unsigned long idval = 0;
  int i;
  for (i = 0; i < 8 && isalnum (str[i]); i++)
    {
      idval =
	idval * 16 + (isalpha (str[i]) ? toupper (str[i]) - 'A' +
		      10 : str[i] - '0');
    }
  return idval;
}

// Вытаскиваем MSGID из кладжей.
unsigned long getMSGID (char *ctrl)
{
  char *p = strstr (ctrl, "\001MSGID:");
  if (!p)
    {
      return (unsigned int)0;
    }
  p += 8;
  p = strchr (p, ' ') + 1;
  // p здесь как pаз кажет на начало собственно ID. Случай глюкавых MSGID
  // не pассматpивается, хотя по зубам это может дать капитально, если
  // какой пpидуpошный софт попадется.
  return str2MSGID (p);
}

// Вытаскиваем REPLY из кладжей.
unsigned long get_reply_id (char *ctrl)
{
  char *p = strstr (ctrl, "\001REPLY:");
  if (!p)
    {
      return (unsigned int)0;
    }
  p += 8;
  p = strchr (p, ' ') + 1;
  return str2MSGID (p);
}

//-------------------------------------------------------------------------
// Реализация TimEd-стиля.
//-------------------------------------------------------------------------

void linkTimEd (PHash msghash, PHash umsghash, PMsgInfo msgs,
		unsigned long frames)
{
  unsigned long i, j;
  int firstempty, linked;
  MsgInfo tmpmsg;

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg;		// Сюда буем искать письмо, на которое текущее - ответ.
      if (msgs[i].hdr.nextreply[0] != 0)
	{
	  // На всякий случай проверим - корректна ли первая ссылка
	  // в nextreply с точки зрения TimEd-типа. Она может оказаться
	  // некорректной если ранее использовался другой тип. Остальные
	  // nextreply не проверяем исходя из следующих предположений:
	  // а) древовидная линковка одна;
	  // б) мы ее делаем правильно (надеюсь 8);
	  // в) возможные некорректности - результат чьей-то кривости,
	  //    гонятся за которой не будем;
	  // г) главное: это может заметно присадить скорость работы.
	  tmpmsg.index_info.umsgid = msgs[i].hdr.nextreply[0];
	  PMsgInfo nextmsg = findElement (umsghash, &tmpmsg);
	  if ((!nextmsg) || (nextmsg->reply_id != msgs[i].msgid))
	    {
	      msgs[i].hdr.nextreply[0] = 0;
	      msgs[i].changed = 1;
	    }
	}
      if (msgs[i].reply_id == 0)
	{
	  // Это вообще не ответ - так чего с ним возится?
	  continue;
	}
      tmpmsg.msgid = msgs[i].reply_id;
      rmsg = findElement (msghash, &tmpmsg);
      if (rmsg == NULL)
	{
	  // Нужной мессаги в базе нет - отваливаем.
	  if (msgs[i].hdr.replyto != 0)
	    {
	      // А раньше, похоже, была - replyto сохранился. Значит -
	      // сбрасываем в 0 и делаем пометку.
	      msgs[i].hdr.replyto = 0;
	      msgs[i].changed = 1;
	    }
	  continue;
	}
      // А теперь одно из двух: либо эти две мессаги уже были слинкованы,
      // либо это еще предстоит сделать - и тогда нужно место в nextreply
      // чтобы в него прописать ссылку.
      firstempty = -1;
      linked = 0;
      if (msgs[i].hdr.replyto != rmsg->index_info.umsgid)
	{
	  // Кажется, мессага еще не линковалась. Или кто-то побил
	  // заголовок. Или ранее линковалось иначе. В любом случае -
	  // выставляем replyto.
	  msgs[i].hdr.replyto = rmsg->index_info.umsgid;
	  msgs[i].changed = 1;
	}
      // Теперь выясняем или факт слинкованности мессаг, или местоположение
      // для ссылки в nextreply. По-пpежнему возможна ситуация несоpтиpованных
      // nextreply, но в обычной pаботе, похоже, это будет некpитично. В кpайнем
      // случае - можно будет "вpисовать" соpтиpовку пpи записи. Поскольку
      // в обычном случае менятся будет совсем небольшое число мессаг,
      // то этот оверхед не должен быть велик. Делать сортировку ранее записи
      // не стоит по соображениям скорости (чем меньше циклов гоняем - тем
      // оно лучше 8).
      for (j = 0; j < 10; j++)
	{
	  if (msgs[i].index_info.umsgid == rmsg->hdr.nextreply[j])
	    {
	      // Дальше продолжать нет смысла - тут "связисты уже были", и
	      // линки навели...
	      linked = 1;
	      break;
	    }
	  if (firstempty < 0 && rmsg->hdr.nextreply[j] == 0)
	    {
	      firstempty = j;
	    }
	}
      if (!linked && firstempty >= 0)
	{
	  rmsg->hdr.nextreply[firstempty] = msgs[i].index_info.umsgid;
	  rmsg->changed = 1;
	}
      // Готово. К этому моменту все повязаны...
    }
}

//-------------------------------------------------------------------------
// Реализация линейных типов линковки для @MSGID/@REPLY.
//-------------------------------------------------------------------------

// Пробежка от заданного письма по линкам и связывание мессаг в список ответов.
// Предполагается, что заданное письмо является головой списка.
void markList (PHash msghash, PHash umsghash, PMsgInfo msg)
{
  PMsgInfo curMsg = msg;
  MsgInfo tmpmsg;
  msg->head = msg;
  msg->last = msg;
  while (curMsg && (curMsg->hdr.nextreply[0] != 0))
    {
      tmpmsg.index_info.umsgid = curMsg->hdr.nextreply[0];
      curMsg->next = findElement (umsghash, &tmpmsg);
      if (curMsg->next)
	{
	  if (curMsg->next->reply_id == 0)
	    {
	      // Однако - выйти по списку на голову цепочки ответов! Явно
	      // нелады - рубить такую ссылку к чертям.
	      curMsg->hdr.nextreply[0] = 0;
	      curMsg->changed = 1;
	      continue;
	    }
	  if (curMsg->next->prev == NULL)
	    {
	      curMsg->next->prev = curMsg;
	    }
	  else
	    {
	      // Не то в цикл влетели, не то на другой список наткнулись.
	      //
	      // Три возможных ситуации: В первом случае prev у найденного
	      // элемента не кажет на curMsg. Если кажет, то это может
	      // быть результатом прохода от какой-то мессаги вверх до упора
	      // и сборки всех писем по пути в список. Тогда будет позван
	      // markList для найденной головы списка, когда собсвенно сам
	      // список уже есть.
	      // Если prev кажет не на curMsg, то это может быть как
	      // цикл так и другой список. Выяснить можно по содержимому
	      // head: если совпадает с curMsg->head, то цикл.
	      //
	      // Во втором случае next найденного письма кажет на curMsg.
	      // Это значит, что две мессаги одна на другую ссылаются.
	      // Явный цикл, конечно же. 8)
	      //
	      // В третьем случае и вовсе ссылка на себя же.

	      if (curMsg->next->next == curMsg)
		{
		  curMsg->next->hdr.nextreply[0] = 0;
		}
	      if ((curMsg->next->prev != curMsg) ||
		  (curMsg->next->next == curMsg) || (curMsg->next == curMsg))
		{
		  curMsg->hdr.nextreply[0] = 0;
		  curMsg->changed = 1;
		  if (curMsg->next->next == curMsg)
		    {
		    }
		  continue;
		}
	    }
	  curMsg->next->head = msg;
	  curMsg->next->processed = 1;
	  // Еще неизвестно - есть ли продолжение у списка, так что на
	  // всякий случай - запомним найденное письмо как последнее в
	  // списке. Если вдруг его nextreply[ 0] не пуст, а поиск
	  // такого umsgid обломался - значит в базе непорядок, и лучше
	  // просто закрыть на это глаза. Если список вдруг будет продолжаться -
	  // некорректность автоматом перетрется.
	  msg->last = curMsg->next;
	}
      curMsg = curMsg->next;
    }
}

void linkLinear (PHash msghash, PHash umsghash, PMsgInfo msgs,
		 unsigned long frames, int linkType)
{
  PMsgInfo *fakemsgs;		// Сюда будем забрасывать фиктивные мессаги для случая,
  // когда reply есть, а оргинал ушел. Вместо оригинала
  // будет создаваться фиктив и запихиваться в хэш.
  // Для экономии памяти держать будем массив укзателей,
  // а фиктивы распределять по необходимости.
  unsigned long fakesize, fakechunk;
  unsigned long fakecount = 0;	// Количество реально имеющихся фиктивных мессаг.
  unsigned long i;
  MsgInfo tmpmsg;

  // Не думаю, чтобы число убитых писем, на которые есть ответы, было больше
  // четверти имеющихся в базе. Скорее всего - не наберется и 10-й части.
  // Но если все же даже такого массива не хватит - перераспределим по необходимости.
  // fakechunk введен для возможности перераспределения чтобы меньше морочить
  // себе голову - какими кусками наращивать массив фейков.
  if (linkType == LINKTYPE_REPLY)
    {
      fakechunk = frames < 4 ? frames : frames / 4;
      fakemsgs =
	(PMsgInfo *) myalloc (sizeof (MsgInfo) * fakesize, __FILE__,
			      __LINE__);
    }

  for (i = 0; i < frames; i++)
    {
      PMsgInfo rmsg, curMsg;

      if (msgs[i].processed)
	{
	  // С этой мессагой уже поработали - нет смысла повторять.
	  continue;
	}
      msgs[i].processed = 1;
      if (msgs[i].reply_id == 0)
	{
	  // Имеем заведомую потенциальную голову списка. Имеет смысл
	  // проверить - а не слинкована ли она уже и быстренько пробежаться
	  // по линкам, промаркировать их и сформировать список.
	  // Заодно отбросится куча уже пролинкованных писем.
	  markList (msghash, umsghash, msgs + i);
	  continue;
	}
      // Письмо является ответом, поэтому надо пробежаться вверх по
      // reply-цепочке, пока не найдем голову или не наткнемся на уже имеющийся
      // список. В общем случе. Частности - за if'ом. 8)
      curMsg = msgs + i;
      tmpmsg.msgid = curMsg->reply_id;
      rmsg = findElement (msghash, &tmpmsg);
      if (rmsg != NULL)
	{
	  while (rmsg)
	    {
	      if (rmsg->processed)
		{
		  // Оппа - а ведь это уже имющийся список.
		  // Будем привязываться к его хвосту. curMsg сейчас указывает
		  // на первый элемент в новой подцепочке.
		  PMsgInfo head = rmsg->head;
		  PMsgInfo last = head->last;
		  last->next = curMsg;
		  curMsg->prev = last;
		  last->hdr.nextreply[0] = curMsg->index_info.umsgid;
		  last->changed = 1;
		  curMsg->hdr.replyto = last->index_info.umsgid;
		  curMsg->changed = 1;

		  // Отметим в голове списка, что последний элемент теперь
		  // другой.
		  head->last = msgs + i;

		  // А теперь прийдется пробежаться от curMsg вниз по новому
		  // подсписку и прописать везде head.
		  while (curMsg)
		    {
		      curMsg->head = head;
		      curMsg = curMsg->next;
		    }

		  // И хватит с этим. Пора двигать к следующей мессаге.
		  break;
		}

	      if (curMsg->hdr.replyto != rmsg->index_info.umsgid)
		{
		  // Письмо еще не линковалось или было слинковано некорректно.
		  // Это надо фиксить.
		  curMsg->hdr.replyto = rmsg->index_info.umsgid;
		  curMsg->changed = 1;
		}
	      if (rmsg->hdr.nextreply[0] != curMsg->index_info.umsgid)
		{
		  // Почти та же фигня: либо в nextreply сидят остатки
		  // ранее использовавшегося типа линковки, либо письмо
		  // вообще еще не подвергалось обработке.
		  rmsg->hdr.nextreply[0] = curMsg->index_info.umsgid;
		  rmsg->changed = 1;
		}

	      // Увязываем пару писем и топаем дальше вверх.
	      curMsg->prev = rmsg;
	      rmsg->next = curMsg;

	      curMsg = rmsg;
	      curMsg->processed = 1;
	      if (curMsg->reply_id != 0)
		{
		  // Это все еще ответ на какое-то другое письмо. Попробуем
		  // его найти.
		  tmpmsg.msgid = curMsg->reply_id;
		  rmsg = findElement (msghash, &tmpmsg);
		}
	      else
		{
		  // А вот теперь добрались до головы нового списка.
		  rmsg = NULL;
		}
	    }
	  // Здесь у нас возможны два случая. Либо rmsg не NULL - значит
	  // новый подсписок уже подлинковали к хвосту имеющегося. Значит
	  // пора переходить к следующей мессаге.
	  // Если же rmsg == NULL, то нашли новый список и curMsg сейчас
	  // является его головой. Значит далее ведем себя как если бы
	  // сразу не нашли оригинал для msgs[ i].
	  if (rmsg != NULL)
	    {
	      continue;
	    }
	}
      // Здесь curMsg - ответ без оригинала или голова нового списка.
      // В первом случае зависим от типа линковки.
      if (curMsg->hdr.replyto != 0)
	{
	  // Чистим возможные остатки после смены типа линковки.
	  // В самом деле: какой может быть replyto у головы списка?
	  curMsg->hdr.replyto = 0;
	  curMsg->changed = 1;
	}
      if (linkType == LINKTYPE_FLAT || curMsg->reply_id == 0)
	{
	  // Если Flat - то принимаем мессагу за голову и строим от этой
	  // головы новый список.
	  markList (msghash, umsghash, curMsg);
	}
      else
	{
	  // Если REPLY - то создаем fake-письмо для оригинала и
	  // оно выполнит роль головы нового списка.
	  if (fakecount == fakesize)
	    {
	      // Список fake'ов полон. Надо поднарастить storage.
	      fakesize += fakechunk;
	      fakemsgs =
		(PMsgInfo *) myrealloc (fakemsgs, fakesize, __FILE__,
					__LINE__);
	    }
	  fakemsgs[fakecount] =
	    (PMsgInfo) myalloc (sizeof (MsgInfo), __FILE__, __LINE__);
	  memset (fakemsgs[fakecount], 0, sizeof (MsgInfo));

	  fakemsgs[fakecount]->processed = 1;
	  fakemsgs[fakecount]->msgid = curMsg->reply_id;

	  fakemsgs[fakecount]->hdr.nextreply[0] = curMsg->index_info.umsgid;

	  // Теперь этот фиктив нужен нам в хэше. Точнее - может понадобится
	  // позже, для увязки и по @REPLY.
	  insertElement (msghash, fakemsgs[fakecount]);

	  markList (msghash, umsghash, fakemsgs[fakecount]);

	  fakecount++;
	}
    }

  // Подчищаем за собой память...
  if (linkType == LINKTYPE_REPLY)
    {
      for (i = 0; i < fakecount; i++)
	{
	  myfree ((void **)&fakemsgs[i], __FILE__, __LINE__);
	}
      myfree (&fakemsgs, __FILE__, __LINE__);
    }
}

//-------------------------------------------------------------------------
// Реализация линковки по Subj
//-------------------------------------------------------------------------

void linkSubj (PHash msghash, PHash umsghash, PMsgInfo msgs,
	       unsigned long frames)
{
  unsigned long i;
  // Хэш для голов списков линковки по Subj.
  PHash smsghash;

  // Расчитываем, что списков линков по Subj будет не больше, чем треть от
  // общего числа писем.
  smsghash = createHash (frames / 3, hfSubj, cmpSubj);

  for (i = 0; i < frames; i++)
    {
      PMsgInfo hmsg, curMsg;
      if (msgs[i].processed)
	{
	  // Тут мы уже побывали - можно и не проверять.
	  continue;
	}

      // Пытаемся найти голову списка писем для subj текущего письма.
      // Если облом - значит есть повод начать новый список. Нет - значит
      // вставляем новое письмо в хвост.
      curMsg = msgs + i;
      curMsg->processed = 1;
      hmsg = findElement (smsghash, curMsg);
      if (hmsg != NULL)
	{
	  PMsgInfo last = hmsg->last;

	  curMsg->head = hmsg;
	  curMsg->prev = last;
	  last->next = curMsg;

	  if (curMsg->hdr.replyto != last->index_info.umsgid)
	    {
	      // Письмо еще не линковалось или было слинковано некорректно.
	      // Это надо фиксить.
	      curMsg->hdr.replyto = last->index_info.umsgid;
	      curMsg->changed = 1;
	    }
	  if (last->hdr.nextreply[0] != curMsg->index_info.umsgid)
	    {
	      // Почти та же фигня: либо в nextreply сидят остатки
	      // ранее использовавшегося типа линковки, либо письмо
	      // вообще еще не подвергалось обработке.
	      last->hdr.nextreply[0] = curMsg->index_info.umsgid;
	      last->changed = 1;
	    }

	  hmsg->last = curMsg;
	}
      else
	{
	  // Списка с таким subj похоже нет. Значит будем начинать новый.

	  insertElement (smsghash, curMsg);

	  curMsg->head = curMsg;
	  curMsg->last = curMsg;
	  curMsg->hdr.replyto = 0;
	  curMsg->hdr.nextreply[0] = 0;
	  curMsg->changed = 1;
	  // Вообще говоря, инициализация head для линковки по subj излишня,
	  // но лучше ее иметь, если понадобится в будущем.
	}
    }

  destroyHash (smsghash);
}

//-------------------------------------------------------------------------
// Реализация линковки вообще... 8)
//-------------------------------------------------------------------------
// Собственно функция для линковки. Принимает имя файла с базой без расширения
// и тип линковки.
// Например: \fido\sqbase\A0001112
// Тогда ожидаем наличия файлов \fido\sqbase\A0001112.sqd и
// \fido\sqbase\A0001112.sqi.
// Функция возвращает 0 если все прошло успешно или код ошибки. Другой
// обработки ошибок предусматриваться не будет до момента встраивания
// в partoss.
int linkArea (const char *areaFile, int linkType)
{
  sqifile *index;		// Storage для индекса.
  long i;
  int hi, hd;			// Хендлы для файловых операций.
  // hi - для индекса, hd - для базы.
  char index_file[_MAX_PATH], base_file[_MAX_PATH];
  struct stat idxst;		// Сюда положим информацию о файле индексов.
  int rd;			// Сюда будем запихивать результат read'а.
  unsigned long frames;		// Число фреймов в базе
  PMsgInfo msgs;		// Массив данных о мессагах.
  PHash msghash, umsghash;	// Хэши с индексированием по MSGID и umsgid соотв.
  unsigned long zero_array[10] = { 0 };
  // Этого будем использовать при проверке наличия
  // чего-либо в последних 9 элементах nextreply[]
  // вычитанного заголовка.

  int ctrlsize = 1024;		// Размер буффера для кладжей
  char *ctrl;			// Собственно буффер. Пока не распределенный. 8)

  strcpy (index_file, areaFile);
  strcat (index_file, ".sqi");
  strcpy (base_file, areaFile);
  strcat (base_file, ".sqd");

  // Доступ на чтение не запрещаем - менять ничего в файле не будем.
  hi = sopen (index_file, O_RDONLY | O_BINARY, SH_DENYWR);
  if (hi == -1)
    {
      return 1;
    }
  // А здесь, от греха подальше, чтение рубим нафиг, поскольку позже сюда же
  // будем писать.
  hd = sopen (base_file, O_RDWR | O_BINARY, SH_DENYRW);
  if (hd == -1)
    {
      close (hi);
      return 2;
    }

  // Читаем индексы и получаем число pабочих фpеймов.
  fstat (hi, &idxst);
  index = (sqifile *) myalloc (idxst.st_size, __FILE__, __LINE__);
  rd = read (hi, index, idxst.st_size);
  // Все, начиная с этого момента файл индексов нас больше не интересует.
  close (hi);
  if (rd != idxst.st_size)
    {
      // Ну я же просил 400 капель, а здесь - 398! 8( 8)
      close (hd);
      return 3;
    }
  frames = idxst.st_size / sizeof (sqifile);

  // Соотв. и pаспpеделяем под инфоpмашку о мессагах ...
  msgs = (PMsgInfo) myalloc (sizeof (MsgInfo) * frames, __FILE__, __LINE__);

  // Теперь на нужен буффер для кладжей...
  // Здесь и далее ( ctrlsize + 1) поскольку ctrlsize - это _чистый_ размер
  // кладжей без учета концевого 0.
  ctrl = (char *)myalloc (ctrlsize + 1, __FILE__, __LINE__);
  // ... и хэши для работы, ...
  msghash = createHash (frames, hfMSGID, cmpMSGID);
  umsghash = createHash (frames, hfumsgid, cmpumsgid);

  // ... и погнали вычитку из базы.
  int zero = 0;
  for (i = 0; i < frames; i++)
    {
      lseek (hd, index[i].offset, SEEK_SET);
      // Достаем заголовок фрейма заодно с заголовком мессаги. Поскольку
      // в структуре sqdmshead поле text относится уже к кладжам, то его
      // из общего размера выбрасываем. И вообще в целях экономии использовать
      // его не будем.
      rd = read (hd, &msgs[i].hdr, sizeof (sqdmshead) - sizeof (char *));
      if (rd != sizeof (sqdmshead) - sizeof (char *))
	{
	  close (hd);
	  return 4;
	}
      if (msgs[i].hdr.ident != SQHDRID)
	{
	  close (hd);
	  return 5;
	}
      // Если вдруг размер кладжей в письме больше, чем ожидалось, то
      // нарастим буффер.
      if (msgs[i].hdr.clen > ctrlsize)
	{
	  ctrlsize = msgs[i].hdr.clen;
	  // Кстати, myrealloc бы пригодился, IMHO. Мало-ли...
	  ctrl = (char *)myrealloc (ctrl, ctrlsize + 1, __FILE__, __LINE__);
	}
      rd = read (hd, ctrl, msgs[i].hdr.clen);
      if (rd != msgs[i].hdr.clen)
	{
	  close (hd);
	  return 6;
	}
      ctrl[msgs[i].hdr.clen] = 0;
      // И запихиваем теперь все, что может впоследствии пригодиться.
      msgs[i].msgid = getMSGID (ctrl);
      msgs[i].reply_id = get_reply_id (ctrl);
      // Здесь не используем никаких memcpy и т.п., поскольку только
      // медленнее будет.
      msgs[i].index_info.offset = index[i].offset;
      msgs[i].index_info.umsgid = index[i].umsgid;
      msgs[i].index_info.hash = index[i].hash;
      msgs[i].processed = msgs[i].changed = 0;
      msgs[i].next = msgs[i].prev = msgs[i].head = msgs[i].last = NULL;
      // iidx может и не понадобится, поскольку информацию из индекса всю
      // и так извлекли.
      msgs[i].iidx = i;
      // На случай если вдруг тип линковки для базы был изменен с TimEd
      // на любой линейный - чистим все netxreply кроме первого. Чтобы
      // не лить потом все назад в базу сначала проверим - а есть ли
      // ненулевой nextreply в 9 последних.
      if ((linkType != LINKTYPE_TIMED)
	  &&
	  (memcmp
	   (msgs[i].hdr.nextreply + 1, zero_array,
	    9 * sizeof (unsigned long)) != 0))
	{
	  memset (msgs[i].hdr.nextreply + 1, 0, sizeof (unsigned long) * 9);
	  msgs[i].changed = 1;
	  zero++;
	}
      // И финальный аккорд вычитки - загоняем в хэши.
      insertElement (msghash, &msgs[i]);
      insertElement (umsghash, &msgs[i]);
    }
  // С кладжами больше не возимся - "плиз буффер нафиг".
  myfree (&ctrl, __FILE__, __LINE__);

  // Итого, к этому моменту имеем массив информации о мессагах msgs, коего
  // должно хватить для линковки за глаза, и хэш, забитый указателями на
  // на элементы этого массива.

  switch (linkType)
    {
    case LINKTYPE_SUBJ:
      linkSubj (msghash, umsghash, msgs, frames);
      break;
    case LINKTYPE_REPLY:
    case LINKTYPE_FLAT:
      linkLinear (msghash, umsghash, msgs, frames, linkType);
      break;
    case LINKTYPE_TIMED:
      linkTimEd (msghash, umsghash, msgs, frames);
      break;
    case LINKTYPE_NONE:
      // Не понял, а что мы тогда здесь делаем??? 8)
      break;
    }

  // Отлинковались. Пора сваливать все назад в базу.
  for (i = 0; i < frames; i++)
    {
      if (msgs[i].changed)
	{
	  lseek (hd, msgs[i].index_info.offset, SEEK_SET);
	  // С размером та же история, что и на вычитке - поле text
	  // отбрасываем за излишностью и вредностью - чтоб мусор
	  // в базу не вписать.
	  write (hd, &msgs[i].hdr, sizeof (sqdmshead) - sizeof (char *));
	}
    }

  destroyHash (msghash);
  destroyHash (umsghash);
  myfree (&index, __FILE__, __LINE__);
  myfree (&msgs, __FILE__, __LINE__);

  return 0;
}

/*
// Небольшой тестовый кусок - чисто для проверки хэша. Берет с stdin список
// MSGID и прогоняет на этом списке фиктивный хэш.
void main()
{
    static unsigned long msgids[ 100000];
    unsigned long n = 0, i;
    char buf[ 32];
    PMsgInfo msgs, msgInfo;
    PHash hash;
    float t1, t2, t3;

    while ( !feof( stdin)) {
        scanf( "%s", buf);
        msgids[ n++] = str2MSGID( buf);
    }

    hash = createHash( n, hfMSGID, cmpMSGID);

    msgs = ( PMsgInfo) myalloc( sizeof( MsgInfo) * n, __FILE__, __LINE__);
    time_point();
    for ( i = 0; i < n; i++) {
        msgs[ i].msgid = msgids[ i];
        msgs[ i].iidx = i;
        insertElement( hash, &msgs[ i]);
    }
    t1 = time_point();

    ccprintf( "Time used to fill: %f\n", t1);
    ccprintf( "Size: %lu, elements: %lu, used: %lu\n", hash->size, hash->elements, hash->used);

    // Проверим результаты. Сначала - проверим, что все MSGID находяся корректно.
    MsgInfo tmpmsg;
    for ( i = 0; i < n; i++) {
        tmpmsg.msgid = msgids[ i];
        msgInfo = findElement( hash, &tmpmsg);
        if ( msgInfo == NULL || msgInfo->iidx != i) {
            ccprintf( "Check failed for %08x\n", msgids[ i]);
        }
    }

    // Теперь - тайминги.

    // Для начала - получим последовательность случайных индексов.
    #define SEQUENCE_LEN 1000000
    static int ridx[ SEQUENCE_LEN];
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        ridx[ i] = rand() % n;
    }

    // Теперь смотрим, сколько времени занимает обращение к хэш-таблице.
    // Для абсолютной чистоты делаем три прохода. Первые два дадут нам
    // время, нужное на обработку проверки: разница времен первого и
    // второго циклов даст чистое время, затрачиваемое на проверку условия,
    // а время, затраченное на сам цикл, так уж и быть, 8) проигнорируем.
    MsgInfo mi;
    msgInfo = &mi;
    time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo->iidx = ridx[ i];
    }
    t1 = time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo->iidx = ridx[ i];
        if ( msgInfo == NULL || msgInfo->iidx != ridx[ i]) {
            ccprintf( "Test lookup failed for %08x\n", msgids[ ridx[ i]]);
        }
    }
    t2 = time_point();
    for ( i = 0; i < SEQUENCE_LEN; i++) {
        tmpmsg.msgid = msgids[ ridx[ i]];
        msgInfo = findElement( hash, &tmpmsg);
        if ( msgInfo == NULL || msgInfo->iidx != ridx[ i]) {
            ccprintf( "Timing test failed for %08x\n", msgids[ ridx[ i]]);
        }
    }
    t3 = time_point();
    ccprintf( "Time used for %lu findElements: %f. I.e. %lf per element\n", SEQUENCE_LEN, t3 - ( t2 - t1), double( t3 - ( t2 - t1)) / SEQUENCE_LEN);
    ccprintf( "This excludes %f seconds used for condition check\n", t2 - t1);
}
*/

int main (int argc, char *argv[])
{
  int linkType = LINKTYPE_TIMED;
  if (argc < 2)
    {
      return 1;
    }
  if (argc > 2)
    {
      linkType = atoi (argv[2]);
    }
  return linkArea (argv[1], linkType);
}
