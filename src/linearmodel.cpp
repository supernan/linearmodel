#include <sstream>
#include "linearmodel.h"

// get the core number of system
int GetCoreNum()
{
    int count = 0;
	count = sysconf(_SC_NPROCESSORS_CONF);
    return count;
}


// compare function of feature_node
bool IsGreater(feature_node n1, feature_node n2)
{
    return n1.index < n2.index;
}


void linear_model::Lock(CClassifier *hClassifier)
{
    pthread_mutex_lock(&(hClassifier->m_ModelMutex));
}


void linear_model::UnLock(CClassifier *hClassifier)
{
	pthread_mutex_unlock(&(hClassifier->m_ModelMutex));
}


void* linear_model::ClassifyThreadFunc(void *arg)
{
    classifierThreadParam *pParam = (classifierThreadParam*)arg;
    vector<pstWeibo> *pDocs = pParam->m_pCorpus;
    vector<vector<pstWeibo> > *pLabelRes = pParam->m_pRes;
    CClassifier *hClassifier = pParam->m_pClassifier;
    int nStart = pParam->m_nStart;
    int nEnd = pParam->m_nEnd;

    vector<pstWeibo> vTemp;
    vector<vector<pstWeibo> > vTempRes(pLabelRes->size(), vTemp);
    for (int i = nStart; i < nEnd; i++)
    {
        int nLaebl = -1;
        if (!hClassifier->PredictDocument((*pDocs)[i], nLaebl))
        {
            DLOG(WARNING) << "ClassifyThreadFunc WARNING Predict Failed in thread function " << endl;
            continue;
        }
        if (nLaebl >= vTempRes.size())
        {
            LOG(ERROR) << "ClassifyThreadFunc Label is out of boundry" << endl;
            continue;
        }
        vTempRes[nLaebl].push_back((*pDocs)[i]);
    }

    Lock(hClassifier);
    for (int i = 0; i < pLabelRes->size(); i++)
		(*pLabelRes)[i].insert((*pLabelRes)[i].end(), vTempRes[i].begin(), vTempRes[i].end());
    UnLock(hClassifier);
}


linear_model::CClassifier::CClassifier(const string &rConfigPath, string sName)
{
    m_sModelName = sName;
    //FLAGS_log_dir = "../logs/" + sName + "/";
    //google::InitGoogleLogging(m_sModelName.c_str());
    __InitClassifier(rConfigPath);
    pthread_mutex_init(&m_ModelMutex, NULL);
}


linear_model::CClassifier::~CClassifier()
{
    WordSegExit(m_wordSegApi);
    pthread_mutex_destroy(&m_ModelMutex);
    delete m_iModel;
}


bool linear_model::CClassifier::__LoadParam(const string &rConfigPath, classifierInitParam &iClassifierParam)
{
    TiXmlDocument *pDocument = new TiXmlDocument(rConfigPath.c_str());
    pDocument->LoadFile();
    if (pDocument == NULL)
    {
        LOG(FATAL) << "__LoadParam Failed Config file is not found" << rConfigPath << endl;
        return false;
    }
    TiXmlElement *pRootElement = pDocument->RootElement();
    if (pRootElement == NULL)
    {
        LOG(FATAL) << "__LoadParam Failed Root Node is NUll Config file is error" <<rConfigPath<< endl;
        return false;
    }
    TiXmlElement *pModelNode = pRootElement->FirstChildElement();
    if (pModelNode == NULL)
    {
        LOG(FATAL) << "Model path is not set" << endl;
        return false;
    }
    iClassifierParam.m_sModelPath = pModelNode->FirstChild()->Value();

    TiXmlElement *pWordNode = pModelNode->NextSiblingElement();
    if (pWordNode == NULL)
    {
        LOG(FATAL) << "WordSpace path is not set" << endl;
        return false;
    }
    iClassifierParam.m_sWordSpacePath = pWordNode->FirstChild()->Value();

    TiXmlElement *pStopNode = pWordNode->NextSiblingElement();
    if (pStopNode == NULL)
    {
        LOG(FATAL) << "Stopwords path is not set" << endl;
        return false;
    }
    iClassifierParam.m_sStopWordsPath = pStopNode->FirstChild()->Value();

    TiXmlElement *pLabelNode = pStopNode->NextSiblingElement();
    if (pLabelNode == NULL)
    {
        LOG(FATAL) << "Label path is not set" << endl;
        return false;
    }
    iClassifierParam.m_sLabelPath = pLabelNode->FirstChild()->Value();

    TiXmlElement *pWordSegNode = pLabelNode->NextSiblingElement();
    if (pWordSegNode == NULL)
    {
        LOG(FATAL) << "WordSeg module path is not set" << endl;
        return false;
    }
    iClassifierParam.m_sWordSegPath = pWordSegNode->FirstChild()->Value();

    TiXmlElement *pFeatNode = pWordSegNode->NextSiblingElement();
    if (pFeatNode == NULL)
    {
        LOG(FATAL) << "feat id is not set" << endl;
        return false;
    }
    string featStr = pFeatNode->FirstChild()->Value();
    stringstream sstr;
    sstr << featStr;
    sstr >> m_nFeatID;
    cout<<"featid " <<featStr<<" "<< m_nFeatID<<endl;

    LOG(INFO) << "Load Config file succeed" << endl;
    return true;
}


bool linear_model::CClassifier::__InitWordsSpace(const string &rWordsPath)
{
    if ((access(rWordsPath.c_str(), F_OK)) == -1)
    {
        LOG(FATAL) << "__InitWordsSpace Failed file does not exit " << rWordsPath <<endl;
        return false;
    }

    ifstream iInput(rWordsPath.c_str());
    string sWord;
    int nIdx;
    while (iInput >> sWord >> nIdx)
    {
        m_mWordsSpace[sWord] = nIdx;
    }
    iInput.close();
    if (m_mWordsSpace.empty())
    {
        LOG(FATAL)<<"__InitWordsSpace Failed wordSpace is empty!" << endl;
        return false;
    }
    LOG(INFO) << "WordsSpace init succeed" << endl;
    return true;
}


bool linear_model::CClassifier::__InitStopWords(const string &rWordsPath)
{
     if ((access(rWordsPath.c_str(), F_OK)) == -1)
     {
         LOG(FATAL) << "__InitStopWords Failed file does not exit " << rWordsPath << endl;
         return false;
     }
     ifstream iInput(rWordsPath.c_str());
    string sWord;
    while (iInput >> sWord)
        m_sStopWords.insert(sWord);
    iInput.close();
    LOG(INFO) << "StopWords init succeed" << endl;
    return true;
}


bool linear_model::CClassifier::__InitLabels(const string &rLabelPath)
{
     if ((access(rLabelPath.c_str(), F_OK)) == -1)
     {
         LOG(FATAL) << "__InitLabels Failed file does not exit " << rLabelPath << endl;
         return false;
     }
    ifstream iInput(rLabelPath.c_str());
    string sLabel;
    while (iInput >> sLabel)
        m_vLabels.push_back(sLabel);
    iInput.close();
    LOG(INFO) << "Labels init succeed" << endl;
    return true;
}


bool linear_model::CClassifier::__InitModel(const string &rModelPath)
{
     if ((access(rModelPath.c_str(), F_OK)) == -1)
     {
         LOG(FATAL) << "__InitModel Failed file does not exit " << rModelPath << endl;
         return false;
     }
     m_iModel = load_model(rModelPath.c_str());
    if (m_iModel)
    {
        LOG(INFO) << "Model init succeed" <<endl;
        return true;
    }
    else
    {
        LOG(FATAL) << "Model init failed" << endl;
        return false;
    }

}


bool linear_model::CClassifier::__InitClassifier(const string &rConfigPath)
{
    classifierInitParam param;
    __LoadParam(rConfigPath, param);
    if (!__InitModel(param.m_sModelPath.c_str()))
        return false;
    if (!__InitLabels(param.m_sLabelPath.c_str()))
        return false;
    if (!__InitWordsSpace(param.m_sWordSpacePath.c_str()))
        return false;
    if (!__InitStopWords(param.m_sStopWordsPath.c_str()))
        return false;
    m_wordSegApi = WordSegInit(param.m_sWordSegPath.c_str());
	if (m_wordSegApi == NULL)
    {
        LOG(FATAL) << "WordSeg Module is Failed" << endl;
        return false;
    }

    featureEngineParam featParam;
    featParam.stopWords = m_sStopWords;
    if (!IFeatureEngine::InitFeatureEngine(featParam))
    {
        LOG(FATAL) << "Feature engine init Failed" << endl;
        return false;
    }

    LOG(INFO) << "Init Classifier succeed" << endl;
    return true;
}


bool linear_model::CClassifier::__WordSegment(pstWeibo pDoc, vector<string> &vWords)
{
    if (pDoc == NULL)
    {
        DLOG(WARNING) << "Doc is NULL when WordSeg" << endl;
        return false;
    }
    int nLen = pDoc->source.size();
    if (nLen == 0)
    {
        DLOG(WARNING) << "Len of Doc is 0" << endl;
        return true;
    }

    //char *pRes = new char[nLen * 10];
    WordSegmentStr(m_wordSegApi, pDoc->source.c_str(), vWords);
    return true;
}


bool linear_model::CClassifier::__Document2Feature(pstWeibo pDoc)
{
    vector<string> vWords;
    string sErInfo = "";
    if (pDoc == NULL)
    {
        DLOG(WARNING) << "__Document2Feature Failed Doc is NULL when extract feature" << endl;
        return false;
    }
    if (!__WordSegment(pDoc, vWords))
    {
        DLOG(WARNING) << "__Document2Feature Failed WordSeg Failed when extract feature" << endl;
        return false;
    }
    if (!IFeatureEngine::FeatExtractByController(m_nFeatID, vWords, pDoc, sErInfo))
    {
        DLOG(WARNING) << "__Document2Feature Failed feat extract by words Failed " << sErInfo << endl;
        return false;
    }
    return true;
}


bool linear_model::CClassifier::__PredictByModel(vector<feature_node> vFeatures, int &rLabel)
{
    if (vFeatures.empty())
    {
        DLOG(WARNING) << "__PredictByModel Features is empty" << endl;
        return false;
    }
	sort(vFeatures.begin(), vFeatures.end(), IsGreater);
	feature_node lastFeature;
	lastFeature.index = -1;
	lastFeature.value = 0.0;
	vFeatures.push_back(lastFeature);

	feature_node *pFeatures = new feature_node[vFeatures.size()];
	double *pRes = new double[m_vLabels.size()];
    if (pFeatures == NULL || pRes == NULL)
    {
        DLOG(WARNING) << "__PredictByModel Memory allocate error when predict" << endl;
        return false;
    }
	memset(pRes, 0, sizeof(double) * m_vLabels.size());
    for (int i = 0; i < vFeatures.size(); i++)
        pFeatures[i] = vFeatures[i];

	double dLabelRes = predict_probability(m_iModel, pFeatures, pRes);
    rLabel = int(dLabelRes);

    delete pFeatures;
    delete[] pRes;
    pFeatures = NULL;
    pRes = NULL;
    return true;

}


bool linear_model::CClassifier::PredictDocument(pstWeibo pDoc, int &rLabel)
{
    if (!__Document2Feature(pDoc))
    {
        DLOG(WARNING) << "PredictDocument Doc2Feat Failed " << endl;
        return false;
    }
	vector<feature_node> vFeatures;
    if (pDoc == NULL)
    {
        DLOG(WARNING) << "PredictDocument Doc is NULL when predict" << endl;
        return false;
    }
	for(int i = 0; i < pDoc->keywords.size(); ++i)
	{
		if(m_mWordsSpace.count((pDoc->keywords)[i].m_sWord) > 0)
		{
			feature_node feature;
			feature.index = m_mWordsSpace[ (pDoc->keywords)[i].m_sWord ];
			feature.value = (pDoc->keywords)[i].m_fTf;
			vFeatures.push_back(feature);
		}
	}

    if (!__PredictByModel(vFeatures, rLabel))
    {
        DLOG(WARNING) << "PredictDocument PredictByModel failed" << endl;
        return false;
    }

    if (rLabel < 0)
    {
        LOG(WARNING) << "PredictDocument Predict label is illegal" << endl;
        return false;
    }
    return true;
}


bool linear_model::CClassifier::BatchPredict(vector<pstWeibo> &rCorpus, vector<vector<pstWeibo> > &rRes)
{
    rRes.clear();
    if (rCorpus.empty())
    {
        LOG(WARNING) << "BatchPredict Corpus to BatchPredict is empty" << endl;
        return false;
    }
    vector<pstWeibo> vTemp;
    rRes.assign(m_vLabels.size() + 1, vTemp);
    int nCores = GetCoreNum();
    int nThreads = 0;
    if (nCores < 2)
        nThreads = 1;
    else
        nThreads = nCores / 2; //TODO
    int nPatchSize = rCorpus.size() / nThreads;
	THPOOL::CThreadPool iClassifierThreadPool(nThreads);
	iClassifierThreadPool.fn_iInitThread();

    if (nPatchSize <= 0) //if the number of docs is too small
    {
        nThreads = 1;
        nPatchSize = rCorpus.size();
    }

    vector<classifierThreadParam*> vParams;
    for (int i = 0; i < nThreads; i++)
    {
        classifierThreadParam *param = new classifierThreadParam;
        if (param == NULL)
        {
            LOG(ERROR) << "BatchPredict Memory allocate error when BatchPredict" << endl;
            return false;
        }
        param->m_pCorpus = &rCorpus;
        param->m_pRes = &rRes;
        param->m_pClassifier = this;
        param->m_nStart = i * nPatchSize;
        if (i == nThreads - 1)
            param->m_nEnd = rCorpus.size();
        else
            param->m_nEnd = (i+1) * nPatchSize;
        vParams.push_back(param);
		iClassifierThreadPool.thpool_add_work(ClassifyThreadFunc, (void*)param);
    }

	iClassifierThreadPool.thpool_wait_work();
    for (int i = 0; i < vParams.size(); i++)
    {
        delete vParams[i];
        vParams[i] = NULL;
    }
    LOG(INFO) << "BatchPredict succeed" << endl;
    return true;
}
