#include "featengine.h"
using namespace feature_engine;

set<string> IFeatureEngine::sm_StopWords;
vector<string> IFeatureEngine::sm_PosTagBlacklist;

bool IFeatureEngine::InitFeatureEngine(const featureEngineParam &param)
{
    if (param.stopWords.empty())
        return false;
    IFeatureEngine::sm_StopWords = param.stopWords;
    IFeatureEngine::sm_PosTagBlacklist.push_back("a");
    IFeatureEngine::sm_PosTagBlacklist.push_back("b");
    IFeatureEngine::sm_PosTagBlacklist.push_back("c");
    IFeatureEngine::sm_PosTagBlacklist.push_back("f");
    IFeatureEngine::sm_PosTagBlacklist.push_back("i");
    IFeatureEngine::sm_PosTagBlacklist.push_back("j");
    IFeatureEngine::sm_PosTagBlacklist.push_back("m");
    IFeatureEngine::sm_PosTagBlacklist.push_back("n");
    IFeatureEngine::sm_PosTagBlacklist.push_back("o");
    IFeatureEngine::sm_PosTagBlacklist.push_back("p");
    IFeatureEngine::sm_PosTagBlacklist.push_back("q");
    IFeatureEngine::sm_PosTagBlacklist.push_back("r");
    IFeatureEngine::sm_PosTagBlacklist.push_back("w");
    IFeatureEngine::sm_PosTagBlacklist.push_back("x");
    return true;
}


bool IFeatureEngine::FeatExtractByController(const int &rFeatID, const vector<string> &vWords,
                                             pstWeibo pDoc, string &sErInfo)
{
    bool bRet = false;
    switch (rFeatID)
    {
        case BASIC_TF_FEAT:
        {
            bRet = __FeatExtractByWords(vWords, pDoc, sErInfo);
            break;
        }
        case TIME_KEY_TF_FEAT:
        {
            bRet = __FeatExtractByTimeKey(vWords, pDoc, sErInfo);
            break;
        }
        default:
        {
            bRet = false;
            sErInfo = "feat id is out of range";
            break;
        }
    }
    return bRet;
}


bool IFeatureEngine::__FeatExtractByTimeKey(const vector<string> &vWords, pstWeibo pDoc, string &sErInfo)
{
    if (vWords.empty())
    {
        sErInfo = "vWords is empty";
        return false;
    }
    if (pDoc == NULL)
    {
        sErInfo = "pDoc is NULL";
        return false;
    }
    stringstream sstr;
    map<string, int> mWordTf;
    mWordTf.clear();
    for (int i = 0; i < vWords.size(); i++)
    {
        string sWordPair = vWords[i];
        string sWord, sTag;
        int offset = sWordPair.rfind("/");
        if (offset != string::npos)
        {
            sWord.assign(sWordPair.begin(), sWordPair.begin() + offset);
            sTag.assign(sWordPair.begin() + offset + 1, sWordPair.end());
        }
        else
        {
            sWord = sWordPair;
            sTag = "";
        }
        basic_tools::Strip(sWord, "\t");
        basic_tools::Strip(sWord, "\r\n");
        basic_tools::Strip(sTag, "\t");
        basic_tools::Strip(sTag, "\r\n");

        bool bRemove = false;
        for (int j = 0; j < sm_PosTagBlacklist.size(); j++)
        {
            if (basic_tools::StartWith(sTag, sm_PosTagBlacklist[j]))
            {
                bRemove = true;
                break;
            }
        }
        if (bRemove)
            continue;
        if (IFeatureEngine::sm_StopWords.find(sWord) == sm_StopWords.end())
        {
            mWordTf[sWord] += 1;
        }
    }

    if (mWordTf.empty())
    {
        sErInfo = "mWordTf is empty";
        return false;
    }
    vector<Word> vWordFeats;
    string sSrc, sKeyVec;
    map<string, int>::iterator it;
    for (it = mWordTf.begin(); it != mWordTf.end(); ++it)
    {
        sstr.clear();
        Word iTempWord;
		iTempWord.m_sWord = it->first;
	    iTempWord.m_fTf = it->second;
        sstr << it->second;
		vWordFeats.push_back(iTempWord);
		sKeyVec += "(" + it->first + ")" + "[" + sstr.str() + "];";
		for(int i = 0; i < it->second; ++i)
			sSrc += it->first + " ";
    }

    //pDoc->source = sSrc;
    pDoc->keywords = vWordFeats;
    pDoc->keyvect = sKeyVec;
    return true;
    return true;
}


bool IFeatureEngine::__FeatExtractByWords(const vector<string> &vWords, pstWeibo pDoc, string &sErInfo)
{
    if (vWords.empty())
    {
        sErInfo = "vWords is empty";
        return false;
    }
    if (pDoc == NULL)
    {
        sErInfo = "pDoc is NULL";
        return false;
    }
    stringstream sstr;
    map<string, int> mWordTf;
    mWordTf.clear();
    for (int i = 0; i < vWords.size(); i++)
    {
        string sWordPair = vWords[i];
        string sWord, sTag;
        int offset = sWordPair.rfind("/");
        if (offset != string::npos)
        {
            sWord = sWordPair.substr(0, offset);
            //if (offset < sWordPair.length())
             //   sTag = sWordPair.substr(offset+1);
            //else
                sTag = "";
            //sWord.assign(sWordPair.begin(), sWordPair.begin() + offset);
            //sTag.assign(sWordPair.begin() + offset + 1, sWordPair.end());
        }
        else
        {
            sWord = sWordPair;
            sTag = "";
        }
        basic_tools::Strip(sWord, "\t");
        basic_tools::Strip(sWord, "\r\n");
        if (IFeatureEngine::sm_StopWords.find(sWord) == sm_StopWords.end())
        {
            mWordTf[sWord] += 1;
        }
    }
    //cout<<mWordTf.size()<<endl;
    if (mWordTf.empty())
    {
        sErInfo = "mWordTf is empty";
        return false;
    }
    vector<Word> vWordFeats;
    string sSrc, sKeyVec;
    map<string, int>::iterator it;
    for (it = mWordTf.begin(); it != mWordTf.end(); ++it)
    {
        sstr.clear();
        Word iTempWord;
		iTempWord.m_sWord = it->first;
		iTempWord.m_fTf = it->second;
        sstr << it->second;
		vWordFeats.push_back(iTempWord);
		sKeyVec += "(" + it->first + ")" + "[" + sstr.str() + "];";
		for(int i = 0; i < it->second; ++i)
			sSrc += it->first + " ";
    }

    //pDoc->source = sSrc;
    pDoc->keywords = vWordFeats;
    pDoc->keyvect = sKeyVec;
    return true;
}
