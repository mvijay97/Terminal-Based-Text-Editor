#include <stdlib.h>
#include <stdbool.h>
#include <string.h>



typedef struct trie
{
	struct trie* a[26];
	bool end;
}trie;




trie* create()
{
	trie* nextlevel=(trie*)malloc(sizeof(trie));
	if(nextlevel!=NULL)
	{
		for(int i=0;i<26;i++)
		{
			nextlevel->a[i]=NULL;
		}
		nextlevel->end=false;
		return(nextlevel);
	}
	exit(0);
}


void insert(trie* root, const char* s)
{
	trie* p=root;
	for(int i=0;i<strlen(s);i++)
	{
		int charcode=(int)s[i]-(int)'a';
		if(!p->a[charcode])
			p->a[charcode]=create();
		
		p=p->a[charcode];
	}
	p->end=true;
}

bool search(trie* root, const char* s)
{
	trie* p=root;
	for(int i=0;i<strlen(s);i++)
	{
		int charcode=(int)s[i]-(int)'a';
		if(p->a[charcode])
			p=p->a[charcode];
		else
			return(false);
	}
	return(p!=NULL && p->end);
}









