#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "astring.h"

//constructors
astring::astring (const char *src)
{
  length = strlen (src);
  Data = (char *)calloc (length + 1, 1);
  strncpy (Data, src, length);
}

astring::astring (const char src)
{
  length = 1;
  Data = (char *)calloc (length + 1, 1);
  Data[0] = src;
}

astring::astring (const char *src, int l)
{
  length = l;
  if (l > (int)strlen (src))
    length = strlen (src);
  Data = (char *)calloc (length + 1, 1);
  strncpy (Data, src, length);
}

astring::astring (const astring & src)
{
  length = src.len ();
  Data = (char *)calloc (length + 1, 1);
  strncpy (Data, src.c_str (), length);
}

astring::astring (const astring & src, int l)
{
  length = l;
  if (l > src.len ())
    length = src.len ();
  Data = (char *)calloc (length + 1, 1);
  strncpy (Data, src.c_str (), length);
}

astring::astring ()
{
  Data = NULL;
  length = 0;
}

//destructors
astring::~astring ()
{
  if (Data)
    free (Data);
}

//assignments
astring & astring::operator = (const astring & rhs)
{
  Data = (char *)realloc (Data, rhs.len () + 1);
  length = rhs.len ();
  memset (Data, 0, length + 1);
  strncpy (Data, rhs.c_str (), length);
  return *this;
}

astring & astring::operator += (const astring & rhs)
{
  Data = (char *)realloc (Data, length + rhs.len () + 1);
  memset (Data + length, 0, rhs.len () + 1);
  strncpy (Data + length, rhs.c_str (), rhs.len ());
  length += rhs.len ();
  return *this;
}

//compare
bool astring::operator == (const astring & rhs)
     const {
       if (length != rhs.len ())
	 return false;
       if (strncmp (Data, rhs.c_str (), length))
	 return false;
       return true;
     }

     bool astring::operator != (const astring & rhs) const
     {
       return !operator == (rhs);
     }

//index access
     char astring::operator[] (const int idx)
     const
     {
       if ((idx < 1) || (idx > length))
	 return 0;
       return Data[idx - 1];
     }

     astring astring::operator + (const astring & rhs) const
     {
       astring tmp (*this);
       tmp += rhs;
       return tmp;
     }

     bool astring::compareic (const astring & rhs)
     const
     {
       if (length != rhs.len ())
	 return false;
       if (strncasecmp (Data, rhs.c_str (), length))
	 return false;
       return true;
     }

     int astring::pos (const astring & rhs) const
     {
       int i;
       if (length <= rhs.len ())
	 return 0;
       for (i = 0; i < (length - rhs.len ()); i++)
	 if (Data[i] == rhs[1])
	   if (!strncmp (Data + i, rhs.c_str (), rhs.len ()))
	     return i + 1;
       return 0;
     }

     int astring::icpos (const astring & rhs)
     const
     {
       int i;
       if (length <= rhs.len ())
	 return 0;
       for (i = 0; i < (length - rhs.len ()); i++)
	 if (Data[i] == rhs[1])
	   if (!strncasecmp (Data + i, rhs.c_str (), rhs.len ()))
	     return i + 1;
       return 0;
     }

     astring astring::substring (int index, int count) const
     {
       if ((index < 1) || (index > length))
	 return "";
       if ((index + count + 1) > length)
	 count = length - index + 1;
       astring tmp (Data + index - 1, count);
       return tmp;
     }

     astring astring::lower ()
     const
     {
       char *tmp;
       int i;
       tmp = (char *)calloc (length, 1);
       for (i = 0; i < length; i++)
	 tmp[i] = tolower (Data[i]);
       astring tmp1 (tmp, length);
       free (tmp);
       return tmp1;
     }

     astring astring::upper () const
     {
       char *tmp;
       int i;
       tmp = (char *)calloc (length, 1);
       for (i = 0; i < length; i++)
	 tmp[i] = toupper (Data[i]);
       astring tmp1 (tmp, length);
       free (tmp);
       return tmp1;
     }

     bool astring::isempty ()
     const
     {
       if (length)
	 return false;
       return true;
     }

     astring astring::word (int index) const
     {
       int i, j, cidx;
       if (index < 1)
	 return "";
       if (index == 1)
	 {
	   for (i = 0; i < length; i++)
	     if (Data[i] == ' ')
	       return astring (Data, i);
	   return *this;
	 }
       cidx = 1;
       for (i = 0; i < length; i++)
	 {
	   if (Data[i] == ' ')
	     cidx++;
	   if (cidx == index)
	     {
	       for (j = i + 1; j < length; j++)
		 if (Data[j] == ' ')
		   break;
	       return astring (Data + i + 1, j - i - 1);
	     }
	 }
       return "";
     }

     astring astring::getword (char Delimiter, int Cnt, bool Soft)
     const
     {
//TastringList *Params=new TastringList;
       astring ret;
       astring ParamStr = *this;

/* Разбоp стpоки паpаметpов */
/* CopyLeft и все дела */

/* Стpока паpаметpов */
//ParamStr='param1, param2,param3,,"param5, param5a",param6'
/* Разделитель паpаметpов */
//Delimiter=','
/* Огpаничители многословного паpаметpа*/
       char Limiter = '"';
/* Добавляем один, нyлевой паpаметp.
Этим самым заставляем алгоpитм `подхватить` последний паpаметp. */
       ParamStr += Delimiter;
/* Флажок `в паpаметpе` (для многословных паpаметpов) */
       int AtParam = 0;
/* Hачальная позиция паpаметpа в стpоке оных */
       int BeginPos = 1;
/* Последняя позиция */
       int EndPos = 0;
/* Hомеp паpаметpа */
       int NumParam = 0;
/* Позиция yказателя в стpоке (тек. символ)*/
       int CurrPos = 0;
/* Длина стpоки паpаметpов */
       int LenParamStr = ParamStr.len ();
/* Содеpжание текyщего паpаметpа */
       astring CurrParam = "";
       char CurrChar;

/* По всем символам... можно на for заменить. Пpосто y меня, в pексе, это
несколько гемоppойно. */
       while (CurrPos < LenParamStr)
	 {
	   CurrPos = CurrPos + 1;

	   /* Беpем символ */
	   CurrChar = (char)ParamStr[CurrPos];

	   /* Если символ - огpаничитель, то считаем что вошли в сложный паpаметp.
	      Если мы yже там находились, то - вышли. Флаг соответсвенно
	      откоppектиpован. */
	   if (CurrChar == Limiter)
	     if (AtParam == 1)
	       AtParam = 0;
	     else
	       AtParam = 1;

	   /* Если символ - pазделитель... */
	   if (CurrChar == Delimiter)
	     {
	       /* Это пpосто заглyшка :)
	          Hазывается: `мы в сложном паpаметpе и нифига не воспpинимаем` */
	       if (AtParam == 1)
		 continue;
	       else
		 {
		   /* Иначе (на pазделителе и вне сложного паpаметpа)... */
		   NumParam = NumParam + 1;
		   BeginPos = EndPos + 1;
		   EndPos = CurrPos;
		   int LenParam = EndPos - BeginPos;
		   /* Режем нyжный кyсок со стpоки */
		   CurrParam = ParamStr.substring (BeginPos, LenParam);

		   /* И пишем кyда надо. Можно Lines.Add. В Soft-pежиме
		      паpаметp Null игноpиpyется. */
		   if (CurrParam.isempty ())
		     {
		       if (!Soft)
			 ret = "";
		       else
			 NumParam--;
		     }
		   else
		     ret = CurrParam;
		   if (NumParam == Cnt)
		     break;
		 }
	     }
	 }
       if (ret.len ())
	 {
	   if ((ret[1] == Limiter) && (ret[ret.len ()] == Limiter))
	     {
	       ret.del (1, 1);
	       ret.del (ret.len (), 1);
	     }
	 }
       return ret;
     }

     astring & astring::del (int index, int count)
     {
       char *tmp;
       if ((index < 1) || (index > length))
	 return *this;
       if (index + count > length)
	 count = length - index;
       tmp = (char *)calloc (length - index - count + 1, 1);
       strncpy (tmp, Data + index + count - 1, length - index - count + 1);
       memset (Data + index - 1, 0, length - index + 1);
       strncpy (Data + index - 1, tmp, length - index - count + 1);
       free (tmp);
       return *this;
     }

     astring astring::trim ()
     const
     {
       int pos1 = 0, pos2 = 0, i;
       for (i = 0; i < length; i++)
	 if (Data[i] <= ' ')
	   pos1 = i;
	 else
	   break;
       for (i = length - 1; i > 0; i--)
	 if (Data[i] <= ' ')
	   pos2 = i;
	 else
	   break;
       return astring (Data + pos1 + 1, pos2 - pos1 - 1);
     }

     astring astring::rtrim () const
     {
       int pos2 = 0, i;
       for (i = length - 1; i > 0; i--)
	 if (Data[i] <= ' ')
	   pos2 = i;
	 else
	   break;
       return astring (Data, pos2);
     }

     astring astring::ltrim ()
     const
     {
       int pos1 = 0, i;
       for (i = 0; i < length; i++)
	 if (Data[i] <= ' ')
	   pos1 = i;
	 else
	   break;
       return astring (Data + pos1 + 1, length - pos1);
     }

     astring astring::cvtslash () const
     {
       int i;
       for (i = 0; i < length; i++)
	 if (Data[i] == '\\')
	   Data[i] = '/';
       return *this;
     }


     stringlist::stringlist ()
     {
       strings = NULL;
       count = 0;
     }

     stringlist::~stringlist ()
     {
       int i;
       if (strings)
	 for (i = 0; i < count; i++)
	   free (*strings);
       free (strings);
     }

     int stringlist::add (const astring & rhs)
     {
       strings = (char **)realloc (strings, sizeof (char *) * (count + 1));
       strings[count] = (char *)calloc (rhs.len () + 1, 1);
       strncpy (strings[count], rhs.c_str (), rhs.len ());
       count++;
       return count - 1;
     }

     astring stringlist::operator[] (const int index)
     const
     {
       if ((index < 0) || (index >= count))
	 return "";
       return astring (strings[index]);
     }

     void stringlist::del (int index)
     {
       int i;
       if ((index < 0) || (index >= count))
	 return;
       free (strings[index]);
       strings[index] = NULL;
       for (i = index; i < count - 1; i++)
	 strings[i] = strings[i + 1];
       strings = (char **)realloc (strings, sizeof (char *) * (count - 1));
     }
