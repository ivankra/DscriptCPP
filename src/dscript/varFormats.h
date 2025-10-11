
extern int
getCPfromLCID( LCID lcid );

extern BSTR 
toBSTR( int codepage, char * lp, int nLen = -1);

extern HRESULT 
_VAR_GetLocaleInfo( LCID lcid, int which, BSTR *pbstrOut );

extern HRESULT 
_VAR_WeekdayName( LCID lcid, int iWeekday, int fAbbrev, int iFirstDay, ULONG dwFlags, BSTR *pbstrOut );

extern HRESULT 
_VAR_MonthName( LCID lcid, int iMonth, int fAbbrev, ULONG dwFlags, BSTR *pbstrOut );

extern HRESULT 
_VAR_FormatNumber( LCID lcid,
				   LPVARIANT pvarIn, 
				   int iNumDig, 
				   int ilncLead, 
				   int iUseParens, 
				   int iGroup, 
				   ULONG dwflags, 
				   BSTR *pbstrOut );

extern HRESULT 
_VAR_FormatPercent( LCID lcid,
				   LPVARIANT pvarIn, 
				   int iNumDig, 
				   int ilncLead, 
				   int iUseParens, 
				   int iGroup, 
				   ULONG dwflags, 
				   BSTR *pbstrOut );

extern HRESULT 
_VAR_FormatCurrency(
				  LCID lcid,
				  LPVARIANT pvarIn, 
				  int iNumDig, 
				  int iIncLead, 
				  int iUseParens, 
				  int iGroup, 
				  ULONG dwFlags, 
				  BSTR *pbstrOut);

extern HRESULT
_VAR_FormatDateTime( LCID lcid,
					 LPVARIANT pvarIn, 
					 int iNamedFormat, 
					 ULONG dwFlags, 
					 BSTR * pbstrOut
					);
