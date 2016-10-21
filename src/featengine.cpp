#include "featengine.h"
using namespace feature_engine;

set<string> IFeatureEngine::sm_StopWords;

bool IFeatureEngine::InitFeatureEngine(const featureEngineParam &param)
{
    if (param.stopWords.empty())
        return false;
    IFeatureEngine::sm_StopWords = param.stopWords;
    return true;
}


bool IFeatureEngine::FeatExtractByWords(const vector<string> &vWords, pstWeibo pDoc, string &sErInfo)
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
