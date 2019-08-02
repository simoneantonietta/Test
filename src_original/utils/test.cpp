/*
 *-----------------------------------------------------------------------------
 * PROJECT: WUtilities
 * PURPOSE: see module test.h file
 *-----------------------------------------------------------------------------
 */
#include "SimpleCfgFile.h"
#include "TaggedBinFile.h"
#include "TaggedTxtFile.h"
#include "Utils.h"
#if 0
int main(int argc, char* argv[])
{
TaggedBinFile tb;
int dat1=5;
float dat2=123.45;
string dat3="test for a std string";
vector<double> dat4;

int rdat1=0;
float rdat2=0.0;
string rdat3="......................";
vector<double> rdat4;

dat4.push_back(1.1);
dat4.push_back(2.2);
dat4.push_back(3.3);
dat4.push_back(4.4);
dat4.push_back(5.5);

tb.setFilename("test.bin");
tb.open('w');
tb.setHeader("test","1.0","Wood","Ebanister");
tb.writeHeader();
tb.writeField("dat1",sizeof(int),&dat1);
tb.writeField("dat2",sizeof(float),&dat2);
tb.writeField("dat3",dat3.size(),(char*)dat3.c_str());
tb.writeArrayDef("dat4",dat4.size());
FOR_EACH(it,double,dat4)
	{
	//printf("val %f\n",*it);
	tb.writeArrayElem(sizeof(double),(char*)&*it);
	}
tb.close();

tb.open('r');
tb.load();
if(tb.checkVersion(2,0)==TBF_FILEVERSION_LOWER) cout << "file version lower" <<endl;
if(tb.checkVersion(1,0)==TBF_FILEVERSION_EQUAL) cout << "file version equal" <<endl;
if(tb.checkVersion(0,1)==TBF_FILEVERSION_HIGHER) cout << "file version higher" <<endl;
tb.readField("dat1",&rdat1);
tb.readField("dat2",&rdat2);
tb.readField("dat3",(char *)rdat3.c_str());
tb.readField("dat4",NULL);
int n=tb.getSize("dat4");
double v;
for(int i=0;i<n;i++)
	{
	tb.readField(tb.getElementUniqueName(i),&v);
	rdat4.push_back(v);
	printf("val %f\n",v);
	}
tb.close();
#endif

#if 0
int n1=1234;
char n2[]="pluto e topolino";
int v[4]={10,20,30,40};
TaggedBinFile tb;
tb.setFilename("test.bin");
tb.initFile("Prova",1,"binario");
tb.addField("pippo",1,'I',sizeof(int),&n1);
tb.addField("cippa",1,'S',strlen(n2),n2);
tb.addField("vector",4,'I',sizeof(int),NULL);
for(int i=0;i<4;i++)
	{
	tb.addVectorElem(&v[i]);
	}

tb.save();

for(int i=0;i<4;i++)
	{
	v[i]=0;
	}

tb.load();

TaggedBinFile::field_t f;


cout << tb.getHeader() << endl;
cout << tb.getFileType() << endl;

f=tb.getField("pippo");
cout << *(int*)f.data << endl;

f=tb.getField("cippa");
cout << (char*)f.data << endl;

for(int i=0;i<4;i++)
	{
	printf("vector value: %i\n",v[i]);
	}
#endif

#if 0
TaggedFile tf("prova.tf");
vector<string> d1,d2;
char b[]="aergbgsertgbfghbsrthgthi2950jfoq3o48ufyq39uybqf394ybtrgaioehrb3q8g49pq9uerhybqh3894pgtbwaher";
char *d=new char(strlen(b));

tf.setSeparator(',');

d1.push_back("13");
d1.push_back("14");
d1.push_back("0");
d1.push_back("135");
d1.push_back("122");
d1.push_back("342");
d1.push_back("4");
d1.push_back("193");

d2.push_back("dato1");
d2.push_back("dato2");
d2.push_back("dato3");
d2.push_back("dato4");
d2.push_back("dato5");

tf.addField("pippo",TaggedFile::tf_string,&d1,3);
tf.addField("prova",TaggedFile::tf_string,&d2,2);
tf.addField("bin",(unsigned char*)b,strlen(b));

cout << "saving" << endl;
tf.save();

cout << "loading" << endl;
tf.load();

cout << "n data="<< tf.getNData("prova") << endl;
cout << "n data="<< tf.getNData("bin") << endl;
cout << "first data="<<tf.getData("pippo") << endl;
cout << "third data="<< tf.getData("pippo",3) << endl;
cout << "integer numeric data="<< tf.getNumber<int>("pippo",5) << endl;

tf.getHexData("bin",(unsigned char*)d,strlen(b));
cout << "bin: " << d  << endl;

cout << "end!" << endl;
#endif

#if 0
simpleCfgFile cfg;
char p[200];
getcwd(p,200);
puts(p);

cfg.loadCfgFile("program2.pg");
cfg.printDatabaseInfo();
cout << cfg.getValueOf("PGM1") << endl;
cout << cfg.getValueOf("PGM2") << endl;
cout << cfg.getValueOf("PGM3") << endl;
cout << cfg.getValueOf("SECTEST.PGM1") << endl;
cout << cfg.getValueOf("SECTEST.PGM2") << endl;
cout << cfg.getValueOf("SECTEST.PGM3") << endl;
return(0);
}
#endif


