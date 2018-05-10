#include "stdafx.h"
#include "FaceDetect.h"
#include "FaceRotate.h"
#include "FaceProcessing.h"
#include "ExtractFeature_.h"
#include "ComputeDistance.h"

using namespace std;
using namespace dlib;

/*
D:\work\OpenCV\caffe\scripts\build\install\bin;C:\Users\user\.caffe\dependencies\libraries_v140_x64_py35_1.1.0\libraries\lib
*/

void Example1(void)
{
	string strFile = "test.xml";

	FileStorage fs(strFile, FileStorage::WRITE);

	fs << "MyTest" << 666;

	fs.release();

	fs.open(strFile, FileStorage::READ);

	int iter;
	fs["MyTest"] >> iter;
	cout << "iter:" << iter << endl;

	iter = (int)fs["MyTest"];
	cout << "iter:" << iter << endl;

	fs.release();
}

void Example2(void)
{
	string strFile = "test.xml";
	FileStorage fs(strFile, FileStorage::WRITE);

	Mat R = Mat_<UCHAR>::eye(30, 30);	//* 对角线矩阵，对角线以1填充
	Mat T = Mat_<DOUBLE>::zeros(80, 500);

	fs << "R" << R;
	fs << "T" << T;

	fs.release();

	Mat R_OUT, T_OUT;
	fs.open(strFile, FileStorage::READ);
	fs["R"] >> R_OUT;
	fs["T"] >> T_OUT;
	cout << R_OUT << endl;
	cout << T_OUT << endl;

	cout << R_OUT.rows << " " << R_OUT.cols << endl;
	cout << T_OUT.rows << " " << T_OUT.cols << endl;
}

void Example3(void)
{
	string strFile = "test.xml";
	FileStorage fs(strFile, FileStorage::WRITE);

	fs << "seq_node" << "[";
	for (int i = 0; i < 16; i++)
		fs << i;
	fs << "]";

	fs << "map_node" << "{";
	for (int i = 0; i < 10; i++)
		fs << "node" + to_string(i) << i;
	fs << "}";

	fs.release();

	fs.open(strFile, FileStorage::READ);
	std::vector<int> a, b;

	FileNode seq_node = fs["seq_node"];
	for (int i = 0; i < seq_node.size(); i++)
	{
		a.push_back(seq_node[i]);
		cout << a[i] << endl;
	}

	cout << "=============================" << seq_node.size() << endl;

	a.clear();
	FileNodeIterator it = seq_node.begin();
	for (; it != seq_node.end(); it++)
	{
		a.push_back(*it);
		cout << (int)*it << endl;
	}

	FileNode map_node = fs["map_node"];

	cout << "=============================" << map_node.size() << endl;
	for (int i = 0; i < map_node.size(); i++)
	{		
		b.push_back(map_node["node" + to_string(i)]);
		cout << b[i] << endl;
	}

	fs.release();
}

struct MyData {
	int i;
	string str;
	Mat mat;
};

static void WriteStruct(FileStorage &fs, const MyData &mydata)
{
	fs << "MY_MAP_DATA" << "{"
	   << "index"		<< mydata.i
	   << "str"			<< mydata.str
	   << "img"			<< mydata.mat
	   << "}";
}

static void ReadStruct(const FileNode &node, MyData &mydata, const MyData &default_val = MyData())
{
	if (node.empty())
		mydata = default_val;
	else
	{
		node["index"] >> mydata.i;
		node["str"] >> mydata.str;
		node["img"] >> mydata.mat;
	}
}

void Example4(void)
{
	string strFile = "test.xml";
	FileStorage fs(strFile, FileStorage::WRITE);

	MyData mydata;

	mydata.i = 42;
	mydata.str = "Hello world";
	mydata.mat = Mat_<DOUBLE>::zeros(8, 3);

	WriteStruct(fs, mydata);

	fs.release();

	fs.open(strFile, FileStorage::READ); 

	MyData data;
	ReadStruct(fs["MY_MAP_DATA"], data);

	fs.release();

	cout << "data.i: " << data.i << endl;
	cout << "data.str: " << data.str << endl;
	cout << "data.mat: " << data.mat << endl;
}

class CMyData 
{
public:
	CMyData() : A(0), X(0), id() {}
	CMyData(int i) : X(0), id()
	{
		A = i;
	}
	CMyData(int i, double x, string str) : A(i), X(x), id(str){}

public:
	int A;
	double X;
	string id;

public:
	void write(FileStorage &fs) const 
	{
		fs << "{";
		fs << "A" << A;
		fs << "X" << X;
		fs << "id" << id;
		fs << "}";
	}

	void read(const FileNode &node)
	{
		A = (int)(node["A"]);
		X = (double)(node["X"]);
		id = (string)(node["id"]);
	}
};

//* 重点是函数原型，编译器会根据这个重载<<操作符
void write(FileStorage& fs, const std::string&, const CMyData& x)
{
	x.write(fs);
}

void read(const FileNode& node, CMyData& x, const CMyData& default_value = CMyData())
{
	if (node.empty())
		x = default_value;
	else;
		x.read(node);
}

//* 重载cout “<<”操作符，用于标准输出
static ostream& operator<<(ostream& out, const CMyData& x)
{
	out << "{ id = " << x.id << ", ";
	out << "X = " << x.X << ", ";
	out << "A = " << x.A << "}";

	return out;
}

void Example5(void)
{
	string strFile = "test.xml";
	FileStorage fs(strFile, FileStorage::WRITE);

	CMyData mydata(42, 67.42, "Hello, World!");
	fs << "MY_MAP_DATA" << mydata;
	fs.release();

	fs.open(strFile, FileStorage::READ);
	CMyData data;

	fs["MY_MAP_DATA"] >> data;
	cout << data << endl;
	fs.release();
}

int main(int argc, char** argv)
{
	Example5();

	return 1;
}