#ifndef _ASTRING_H_
#define _ASTRING_H_

class astring;
class stringlist;

class astring
{
private:
  char *Data;
  int length;

public:

//constructors
    astring (const char *src);
    astring (const astring & src);
    astring (const char *src, int l);
    astring (const astring & src, int l);
    astring (const char src);
    astring ();

//destructors
   ~astring ();

//assignments
    astring & operator = (const astring & rhs);
    astring & operator += (const astring & rhs);

//compare
  bool operator == (const astring & rhs)const;
  bool operator != (const astring & rhs)const;

//index access
  char operator[] (const int idx)const;

  astring operator + (const astring & rhs)const;

  const char *c_str () const
  {
    return (const char *)(Data) ? Data : "";
  }

  int len () const
  {
    return length;
  }

  bool compareic (const astring & rhs)const;

  int pos (const astring & rhs) const;

  int icpos (const astring & rhs) const;

  astring substring (int index, int count) const;

  astring lower () const;

  astring upper () const;

  bool isempty () const;

  astring word (int index) const;

    astring & del (int index, int count);

  astring getword (char Delimiter, int Cnt, bool Soft) const;

  astring trim () const;
  astring rtrim () const;
  astring ltrim () const;
  astring cvtslash () const;

};

class stringlist
{
private:
  char **strings;
public:
    stringlist ();
   ~stringlist ();
  int add (const astring & rhs);
  void del (int index);
  astring operator[] (const int index)const;
  int count;
};
#endif
