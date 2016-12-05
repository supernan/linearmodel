#include <iostream>
//#include <glog/logging.h>
//#include "tinyxml.h"
//#include "tinystr.h"
#include "linearmodel.h"
//#define FLAGS_log_dir = "../logs"
using namespace std;
using namespace linear_model;

int fn_iInitWeiboDataFromFile(const char *dataPath, vector<Weibo> &weibo)
{
		//check path
	if(NULL == dataPath)
	{
		cerr<<"weibo data path error"<<endl;
		return 1;
	}

		//open file
	ifstream in(dataPath);
	if(!in)
		return 1;

		//parse data
	string record;
	int counter = 0;
	while(getline(in, record))
	{
		if(record == "")
			continue;
		Weibo new_doc;
		new_doc.id = counter;
		counter++;
		new_doc.source = record;
		weibo.push_back(new_doc);
	}
	in.close();
	return 0;
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout<<"miss file path" << endl;
        return 1;
    }
    string path = "../conf/linearmodel.xml";
    string name = "model_test";
    CClassifier *pC =  new CClassifier(path, name);
    vector<Weibo> text;
    vector<vector<pstWeibo> > res;
    string fpath = argv[1];
    fn_iInitWeiboDataFromFile(fpath.c_str(), text);
	vector<pstWeibo> corpus;
    for (int i = 0; i < text.size(); i++)
        corpus.push_back(&text[i]);
    if (!pC->BatchPredict(corpus, res))
    {
        cout << "Batch Error" << endl;
        return 1;
    }
    for (int i = 1; i < res.size(); i++)
    {
        //if (i == 57)
        //{
            vector<pstWeibo> docs = res[i];
            cout<<docs.size()<<endl;
            for (int j = 0; j < docs.size(); j++)
            {
                cout<<docs[j]->source<<endl;
            }
        //}
    }
     //   cout<<i<<" "<<res[i].size()<<endl;



    /*string src = "  aasd asdad asdas   d a a ";
    string sep = " ";
    vector<string> words;
    tools::Split(src, sep, words);
    for (int i = 0; i < words.size(); i++)
        cout<<words[i]<<endl;
    cout<<endl;*/
    delete pC;
    return 0;
}
