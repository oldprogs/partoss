/*
Я не в курсе, что и как делается на WatcomC, но во всех моих программах просто
словами пишется, что за облом произошел где бы то ни было. Слова получаются в
DOS'е путем вызова функции 59h и расшифровке в текстовый вид, а в Win32 - при
помощи процедуры, которая идет ниже:
*/
/*
;
; Get system error name via error code
;
*/
String GetSystemErrorNamePrim (unsigned ErrorCode)
{

  LPVOID MsgBuf;		// Message buffer
  unsigned D;			// Temporary variable
  String S;			// Temporary result area

  D = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |	// Message creation style
		     FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,	// Fromatting style
		     NULL,	// No sub-strings
		     ErrorCode,	// Message code
		     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
		     (LPTSTR) & MsgBuf,	// Buffer address
		     0,		// Buffer size
		     NULL);	// Extra arguments
  if (D == 0)			// Undefined error code?
    S = Cs ("Unresolved error code ") + DecU (ErrorCode) + ".";
  else				// Known error code
    S = (char *)MsgBuf;		// Convert result to the string
  LocalFree (MsgBuf);		// Free the buffer
  return S;			// Return result
}
