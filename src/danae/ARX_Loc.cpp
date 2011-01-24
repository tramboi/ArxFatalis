/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
#include "EERIEApp.h"
#include "HERMESMain.h"
#include "ARX_LocHash.h"
#include "ARX_Loc.h"
#include <tchar.h>
#include <list>
#include "ARX_Menu2.h"
#include "EERIETexture.h"
#include <hermes/PakManager.h>

using std::sprintf;

extern long GERMAN_VERSION;
extern long FRENCH_VERSION;
extern long CHINESE_VERSION;
extern long FINAL_COMMERCIAL_GAME;

extern PROJECT			Project;
extern long FINAL_COMMERCIAL_DEMO;
using namespace std;
extern CMenuConfig * pMenuConfig;
#define MAX_LINE_SIZE 8096

CLocalisationHash * pHashLocalisation = NULL;

//-----------------------------------------------------------------------------
bool isSection(_TCHAR * _lpszUText)
{
	ULONG i			 = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
	bool bFirst = false;
	bool bLast  = false;

	while (i < ulTextSize)
	{
		if (_lpszUText[i] == _T('['))
		{
			if (bFirst)
				return false;
			else
				bFirst = true;
		}
		else if (_lpszUText[i] == _T(']'))
		{
			if (bFirst)
				if (bLast)
					return false;
				else
					bLast = true;
			else
				return false;
		}
		else if (isalnum(_lpszUText[i]))
		{
			if (!bFirst)
				return false;
			else if (bFirst && bLast)
				return false;
		}

		i++;
	}

	if (bFirst && bLast) return true;

	return false;
}

//-----------------------------------------------------------------------------
bool isKey(const _TCHAR * _lpszUText)
{
	unsigned long i = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
 
 
	bool bSpace = false;
	bool bAlpha = false;

	while (i < ulTextSize)
	{
		if (_lpszUText[i] == _T('='))
		{
			if (bSpace)
				return false;
			else
				bSpace = true;
		}
		else if (isalnum(_lpszUText[i]))
		{
			if (!bAlpha)
				bAlpha = true;
		}

		i++;
	}

	if (bSpace && bAlpha) return true;

	return false;
}

//-----------------------------------------------------------------------------
bool isNotEmpty(_TCHAR * _lpszUText)
{
	ULONG i			 = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);

	while (i < ulTextSize)
	{
		if (isalnum(_lpszUText[i]))
		{
			return true;
		}

		i++;
	}

	return false;
}

//-----------------------------------------------------------------------------
_TCHAR * CleanSection(const _TCHAR * _lpszUText)
{
	_TCHAR * lpszUText = (_TCHAR *) malloc((_tcslen(_lpszUText) + 2) * sizeof(_TCHAR));
	ZeroMemory(lpszUText, (_tcslen(_lpszUText) + 2)*sizeof(_TCHAR));

	unsigned long ulPos = 0;
	bool bFirst = false;
	bool bLast = false;

	for (unsigned long ul = 0; ul < _tcslen(_lpszUText); ul++)
	{
		if (_lpszUText[ul] == _T('['))
		{
			bFirst = true;
		}
		else if (_lpszUText[ul] == _T(']'))
		{
			bLast = true;
		}

		if (bFirst)
		{
			lpszUText[ulPos] = _lpszUText[ul];
			ulPos ++;

			if (bLast)
			{
				break;
			}
		}
	}

	return lpszUText;
}

//-----------------------------------------------------------------------------
_TCHAR * CleanKey(const _TCHAR * _lpszUText)
{
	_TCHAR * lpszUText = (_TCHAR *) malloc((_tcslen(_lpszUText) + 2) * sizeof(_TCHAR));
	ZeroMemory(lpszUText, (_tcslen(_lpszUText) + 2)*sizeof(_TCHAR));

	unsigned long ulPos = 0;
	unsigned long ulTextSize = _tcslen(_lpszUText);
	bool bAlpha = false;
	bool bEqual = false;

	for (unsigned long ul = 0; ul < ulTextSize; ul++)
	{
		if (_lpszUText[ul] == _T('='))
		{
			bEqual = true;
		}
		else if (bEqual && (isalnum(_lpszUText[ul]) ||
		                    ((_lpszUText[ul] != _T(' ')) &&
		                     (_lpszUText[ul] != _T('"')))
		                   ))
		{
			bAlpha = true;
		}
		else if ((_lpszUText[ul] == _T('\"')) && (!bAlpha))
		{
			continue;
		}

		if (bEqual && bAlpha)
		{
			lpszUText[ulPos] = _lpszUText[ul];
			ulPos ++;
		}
	}

	while (ulPos--)
	{
		if (isalnum(lpszUText[ulPos]))
		{
			break;
		}
		else if (lpszUText[ulPos] == _T('"'))
		{
			lpszUText[ulPos] = 0;
		}
	}

	return lpszUText;
}

//-----------------------------------------------------------------------------
void ParseFile(_TCHAR * _lpszUTextFile, const unsigned long _ulFileSize)
{
	_TCHAR * pULine;
 

	//-------------------------------------------------------------------------
	// on skip l'entete unicode
	_TCHAR * pFile = _lpszUTextFile;
	pFile ++;
	unsigned long ulFileSize = _ulFileSize - 1;

	//-------------------------------------------------------------------------
	//clean up comments
	for (unsigned long i = 0; i < (ulFileSize - 1); i++)
	{
		if ((pFile[i] == _T('/')) && (pFile[i+1] == _T('/')))
		{
			unsigned long j = i;

			while ((j < (ulFileSize - 1)) && (pFile[j] != _T('\r')) && (pFile[j+1] != _T('\n')))
			{
				pFile[j] = _T(' ');
				j++;
			}

			i = j;
		}
	}

	//-------------------------------------------------------------------------
	// get all lines into list

	list<_TCHAR *> lUText;
	pULine = _tcstok(pFile, _T("\r\n"));

	long t = 0;

	while (pULine != NULL)
	{
		if (isNotEmpty(pULine))
		{
			t++;
			lUText.insert(lUText.end(), (pULine)); //_tcsdup
		}

		pULine = _tcstok(NULL, _T("\r\n"));
	}

	list<_TCHAR *>::iterator it;

	//-------------------------------------------------------------------------
	// look up for sections and associated keys
	it = lUText.begin();
	t = 0;

	while (it != lUText.end())
	{
		if (isSection(*it))
		{
			CLocalisation * pLoc = new CLocalisation();
			_TCHAR * lpszUT = CleanSection(*it);
			pLoc->SetSection(lpszUT);

			free(lpszUT);

			it++;
			t++;

			while ((it != lUText.end()) && (!isSection(*it)))
			{
				if (isKey(*it))
				{
					_TCHAR * lpszUTk = CleanKey(*it);
					pLoc->AddKey(lpszUTk);
					free(lpszUTk);
					it++;
					t++;
				}
				else
				{
					break;
				}
			}

			pHashLocalisation->AddElement(pLoc);
			continue;
		}

		it++;
		t++;
	}
}

//-----------------------------------------------------------------------------
char LocalisationLanguage = -1;

//-----------------------------------------------------------------------------
// TODO parameter not really used.
void ARX_Localisation_Init(const char * _lpszExtension) 
{
	if (_lpszExtension == NULL)
		return;

	// nettoyage
	if (pHashLocalisation)
	{
		ARX_Localisation_Close();
	}

	char tx[256];
	ZeroMemory(tx, 256);
	sprintf(tx, "localisation\\utext_%s.ini", Project.localisationpath);

	size_t LocalisationSize = 0;
	_TCHAR * Localisation = NULL;

	Localisation = (_TCHAR *)PAK_FileLoadMallocZero(tx, &LocalisationSize);

	if (Localisation == NULL)
	{
		if (GERMAN_VERSION || FRENCH_VERSION)
		{
			delete pMenuConfig;	
			pMenuConfig = NULL;
			exit(0);
		}

		ZeroMemory(tx, 256);
		strcpy(Project.localisationpath, "english");
		sprintf(tx, "localisation\\utext_%s.ini", Project.localisationpath);
		Localisation = (_TCHAR *)PAK_FileLoadMallocZero(tx, &LocalisationSize);

	}

	if (Localisation && LocalisationSize)
	{
		pHashLocalisation = new CLocalisationHash(1 << 13);
		LocalisationSize = _tcslen(Localisation);
		ParseFile(Localisation, LocalisationSize);
		free((void *)Localisation);
		Localisation = NULL;
		LocalisationSize = 0;
	}

	//CD Check
	if (FINAL_COMMERCIAL_DEMO)
	{
		std::string szMenuText;
		PAK_UNICODE_GetPrivateProfileString( "system_menus_main_cdnotfound", "", szMenuText, 256);

		if (!szMenuText[0]) //warez
		{
			ARX_Localisation_Init();
		}
	}

	if (FINAL_COMMERCIAL_GAME)
	{
		std::string szMenuText;
		PAK_UNICODE_GetPrivateProfileString( "unicode", "", szMenuText, 256);

		if (szMenuText[0]) //warez
		{
			if (!szMenuText.compare( "chinese" ) )
			{
				CHINESE_VERSION = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void ARX_Localisation_Close()
{
	LocalisationLanguage = -1;

	delete pHashLocalisation;
	pHashLocalisation = NULL;
}

//-----------------------------------------------------------------------------
long HERMES_UNICODE_GetProfileString(   const std::string&  sectionname,
                                        const std::string&  defaultstring,
                                        std::string&        destination,
                                        unsigned long       maxsize)
{

    destination.clear();

    if (pHashLocalisation)
    {
        std::string* t = pHashLocalisation->GetPtrWithString(sectionname);

        if (t)
            destination = *t;
        else
            destination = defaultstring;
    }
    else
        destination = defaultstring;

    return 0;
}

//-----------------------------------------------------------------------------
long HERMES_UNICODE_GetProfileSectionKeyCount(const std::string& sectionname)
{
    if (pHashLocalisation)
        return pHashLocalisation->GetKeyCount(sectionname);

    return 0;
}

static long ltNum = 0;

//-----------------------------------------------------------------------------
int PAK_UNICODE_GetPrivateProfileString(    const std::string&  _lpszSection,
                                            const std::string&  _lpszDefault,
                                            std::string&        _lpszBuffer,
                                            unsigned long       _lBufferSize )
{
    ltNum ++;
    _lpszBuffer.clear();

    if ( _lpszSection.empty() )
    {
        _lpszBuffer = _lpszDefault + ": NOT FOUND";
        return 0;
    }

    std::string szSection = "[" + _lpszSection + "]";

    HERMES_UNICODE_GetProfileString( szSection,
                                    _lpszDefault,
                                    _lpszBuffer,
                                    _lBufferSize);

    return 1;
}

