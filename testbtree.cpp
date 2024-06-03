//tstbtree.cc
#include "btnode.h"
#include "./btindex/btnode.cpp" // for template method body
#include "btree.h"
#include "./btindex/btree.cpp"   // for template method body
#include <iostream>

using namespace std;

const char * keys="CSDTAMPIBWNGURKEHOLJYQZFXV";

// for template separate compile

//template class SimpleIndex<char>;
//template class BTree<char>;
//template class BTreeNode<char>;
//template class RecordFile<BTreeNode<char> >;


// order 5로 수정
const int BTreeSize = 5;
int main (int argc, char ** argv)
{
	int result, i;
	BTree <char> bt (BTreeSize);
	result = bt.Create ("testbt.dat",ios::in|ios::out);
	if (!result)
	{
		cout<<"Please delete testbt.dat"<<endl;
		return 0;
	}
	cout << "Insert test" << endl;
	for (i = 0; i<26; i++)
	{
		cout<<"Inserting "<<keys[i]<<endl;
		result = bt.Insert(keys[i],i);
		bt.Print(cout);
	}

	// BTree::remove연산 테스트
	const char* keysToRemove = "DTAMPIBWNGU";
	cout << "\nRemove test" << endl;
	for (i = 0; i < 11; i++)
	{
		cout << "Removing " << keysToRemove[i] << endl;
		result = bt.Remove(keysToRemove[i]);
		bt.Print(cout);
	}
	bt.Search('S',1);
	return 1;
}