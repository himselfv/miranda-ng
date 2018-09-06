#ifndef __54294385_3fdd_4f0c_98c3_c583a96e7fb4_DBUtils_h__
#define __54294385_3fdd_4f0c_98c3_c583a96e7fb4_DBUtils_h__

std::string CurrencyRates_DBGetStringA(MCONTACT hContact, const char* szModule, const char* szSetting, const char* pszDefValue = NULL);
std::wstring CurrencyRates_DBGetStringW(MCONTACT hContact, const char* szModule, const char* szSetting, const wchar_t* pszDefValue = NULL);


#define CurrencyRates_DBGetStringT CurrencyRates_DBGetStringW

bool CurrencyRates_DBWriteDouble(MCONTACT hContact, const char* szModule, const char* szSetting, double dValue);
bool CurrencyRates_DBReadDouble(MCONTACT hContact, const char* szModule, const char* szSetting, double& rdValue);

#endif //__54294385_3fdd_4f0c_98c3_c583a96e7fb4_DBUtils_h__
