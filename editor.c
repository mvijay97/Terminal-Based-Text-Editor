#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "trie.h"

typedef struct buffer
{
	int gapStart;
	int gapEnd;
	int point;
	int buflen;
	int prevPos;
	char text[80*24];
	int mode;
	int count;
	int flag[24];

}buffer;

typedef struct Doc
{	
	buffer* first;
	buffer* current;
	int nbuffers;
}Doc;


int maxx,maxy;
buffer* initBuffer();
void process(Doc*doc,int y,int x, trie* dict);
void addchar(Doc* b,char ch,int x, int y, trie* dict);
void delchar(Doc* b, int x, int y);

void save(Doc* doc,int y,int x);
void shiftBufferLeft(Doc* b, int pos);
void shiftBufferRight(Doc* b, int pos);
int getPosition(Doc* b, int pos);
int movedCursor(int prev, int current,int gap);
void shiftLeftDel(Doc* b, int pos);
void importtext(Doc* b, char* f);
void spellcheck(Doc* b, int x, int y,trie* dict);
void getcommand(Doc* b,int y, int x);
void searchtext(Doc* b);
trie* create_trie();

int main(int argc, char** argv)
{	
	Doc newdoc;
	newdoc.first=initBuffer();
	newdoc.current=newdoc.first;
	newdoc.nbuffers=1;
	Doc* doc=&newdoc;

	trie* dict=create();
	FILE* fp;

	int x,y;
	initscr(); //initialises terminal to curses mode and allocates window memory
	cbreak(); //sets the terminal to a mode like raw, but it processess control sequences
	noecho(); //turns of echoing
	refresh(); //you have to call this function every time you change config or render something for the change to take effect
	keypad(stdscr,TRUE); //enable using arrow keys
	getyx(stdscr,y,x); //get cursor position
	getmaxyx(stdscr, maxy, maxx);
	if(argc==2)
	{
		importtext(doc,argv[1]);
	}
	fp=fopen("dictionary.txt","r");
	int i=0;
	char s[35];
	while(fgets(s,30,fp))
	{
		char t[35];
		for(i=0;i<strlen(s)-1;i++)
			t[i]=s[i];
		t[i]='\0';
		insert(dict,t);
		for(i=0;i<35;i++)
		{
			t[i]='\0';
		}
	}
	fclose(fp);
	while(1)
	{
		process(doc,y,x,dict);
	}
}

void process(Doc* doc,int y,int x,trie* dict)
{
	getyx(stdscr,y,x);
	int c=getch(); //disable line buffering
	switch(c)
	{
		case KEY_DOWN:
			move(y+1,x); //move the cursor
			break;
		case KEY_LEFT:
			move(y,x-1);
			break;
		case KEY_RIGHT:
			move(y,x+1);
			break;
		case KEY_UP:
			move(y-1,x);
			break;
		/*case 9:
			
			break;*/
		case 10: //enter
			//enter(doc,x,y);
			move(y+1,0);
			//hline(' ',80);
			insertln();
			break;
		case 127: //backspace
			if(x==0)
				move(y-1,79);
			else
				move(y,x-1);
			delchar(doc,x,y);
			delch();
			break;
		case 27:
			save(doc,y,x);
			endwin(); //closes the window created
			exit(0);
			break;
		case 9:
			getcommand(doc,y,x);
			break;
		default:
			if(x==maxx-1)
			{
				insch(c);
				move(y+1,0);
			}
			else
				insch(c);
			addchar(doc,c,x,y,dict);
			move(y,x+1);
			break;
	}
	refresh();
}

buffer* initBuffer()
{
	buffer* newBuffer=(buffer*)malloc(sizeof(buffer));
	if(newBuffer!=NULL)
	{
		newBuffer->gapStart=5;
		newBuffer->gapEnd=newBuffer->gapStart+9;
		newBuffer->point=-1;
		newBuffer->buflen=10;
		newBuffer->mode=0;
		newBuffer->prevPos=-1;
		newBuffer->count=0;
		for(int i=0;i<23;i++)
			newBuffer->flag[i]=0;
		for(int i=0;i<80*24;i++)
			newBuffer->text[i]='\0';
		return(newBuffer);
	}
	printf("Something Went Wrong!");
	exit(0);
}

int getPosition(Doc* b, int pos)
{
	if(b->current->mode==1&&pos==b->current->gapStart)
	{
		if(pos>b->current->gapEnd)
		{
			return(b->current->prevPos-1);
		}
		return(pos);
	}
	if(pos>=b->current->gapStart)
	{
		{
			if(pos<=b->current->gapEnd)
			{
				pos=b->current->gapEnd+(pos-b->current->gapStart)+1;
			}
			else
			{
				pos=pos+(b->current->gapEnd-b->current->gapStart)+1;
			}
			return(pos);
		}
	}
	return(pos);
}

void bufferLeftAdd(Doc* b, int pos,char ch)
{
	for(int i=b->current->gapStart-1;i>=pos;i--)
	{
		b->current->text[b->current->gapEnd--]=b->current->text[i];
	}
	b->current->text[pos]=ch;
	b->current->gapStart=pos+1;
	for(int i=b->current->gapStart;i<b->current->gapEnd+1;i++)
	{
		b->current->text[i]='\0';
	}
}

void  bufferRightAdd(Doc* b, int pos, char ch)
{
	for(int i=b->current->gapEnd+1;i<pos;i++)
	{
		b->current->text[b->current->gapStart++]=b->current->text[i];
	}
	b->current->text[b->current->gapStart++]=ch;
	b->current->gapEnd=pos-1;
	for(int i=b->current->gapStart;i<b->current->gapEnd+1;i++)
	{
		b->current->text[i]='\0';
	}
}

void shiftLeftDel(Doc* b, int pos)
{
	for(int i=b->current->gapStart-1;i>=pos;i--)
	{
		b->current->text[b->current->gapEnd--]=b->current->text[i];
	}
	b->current->gapStart=pos-1;
	b->current->buflen++;
	for(int i=b->current->gapStart;i<b->current->gapEnd+1;i++)
	{
		b->current->text[i]='\0';
	}
}

void shiftRightDel(Doc* b, int pos)
{
	for(int i=b->current->gapEnd+1;i<pos-1;i++)
	{
		b->current->text[b->current->gapStart++]=b->current->text[i];
	}
	b->current->gapEnd=pos-1;
	b->current->buflen++;
	for(int i=b->current->gapStart;i<b->current->gapEnd+1;i++)
	{
		b->current->text[i]='\0';
	}
}

void save(Doc* doc,int y,int x)
{
	printw("\n");
	FILE* fp=fopen("new.txt","w");
	//fwrite(doc->current->text,sizeof(char),50,fp);
	for(int j=0;j<24;j++)
	{	
		if(doc->current->flag[j]==1)
		{
			shiftRightDel(doc,doc->current->point+1);
			for(int i=80*j;i<80*(j+1);i++)
			{
				/*if(i>doc->current->point)
					break;*/
				fprintf(fp,"%c",doc->current->text[i]);
			}
			fputc('\n',fp);
		}
	}
	fclose(fp);
}



void delchar(Doc* b, int x, int y)
{
	int pos=y*80+x;
	pos=getPosition(b,pos);
	b->current->prevPos=pos;
	if(pos==b->current->point+1)
	{
		b->current->point--;
	}
	else if(pos==b->current->gapEnd+1)
	{
		b->current->gapStart--;
		b->current->buflen++;
	}
	else if(pos==b->current->gapEnd+2)
	{
		b->current->gapEnd++;
		b->current->buflen++;
	}
	else if(pos<b->current->gapStart-1)
	{
		shiftLeftDel(b,pos);
	}
	else
	{
		shiftRightDel(b,pos);
	}
	/*if(pos%80==1)
		b->current->flag[y]=0;*/
}


void addchar(Doc* b,char ch,int x, int y, trie* dict)
{
	//b->current->count++;
	if(ch==' ' && b->current->count!=0)
	{
		spellcheck(b,x,y,dict);
	}
	if(ch==' ' && b->current->count==0)
	{
		b->current->count=1;
	}
	int pos=80*y+x;
	pos=getPosition(b, pos);
	if(b->current->point==b->current->gapStart-1)
	{
		b->current->point=b->current->gapEnd;
	}
	if(pos==b->current->point+1)
	{
		b->current->text[++b->current->point]=ch;
		b->current->buflen--;
	}
	else if(pos==b->current->gapEnd+1)
	{
		b->current->text[b->current->gapStart++]=ch;
		b->current->buflen--;
	}
	else
	{
		if(pos<b->current->gapStart)
		{
			bufferLeftAdd(b,pos,ch);
			b->current->buflen--;
		}
		else
		{
			bufferRightAdd(b,pos,ch);
			b->current->buflen--;
		}
	}
	if(b->current->gapStart==b->current->gapEnd+1)
	{
		b->current->gapStart=b->current->gapEnd+1;
		for(int i=b->current->point;i>=b->current->gapEnd+1;i--)
		{
			b->current->text[i+10]=b->current->text[i];
		}
		b->current->gapEnd=b->current->gapStart+9;
		b->current->point+=10;
		b->current->buflen=10;
	}
	b->current->flag[y]=1;
}

trie* create_trie(trie* root)
{
	FILE* fp;
	char s[36];
	fp=fopen("dict.txt","r");
	while(fgets(s,35,fp))
	{
		insert(root,s);
	}
	fclose(fp);
	return(root);
}

void spellcheck(Doc* b, int x, int y, trie* dict)
{

	int pos=80*y+x-1;
	pos=getPosition(b,pos);
	char s[36]="\0";
	char d[2];
	int i=pos;

	while(b->current->text[i]!=' ')
	{
		if(i==b->current->gapEnd)
		{
			i=b->current->gapStart-1;
		}
		else if(i==b->current->gapStart)
		{
			i=b->current->gapStart-1;
		}
		if(b->current->text[i]!=' ')
		{
			char d[2]={b->current->text[i],'\0'};
			strcat(s,d);
			i--;
		}
	}

	char t[36];
	int j=0;
	for(int i=strlen(s)-1;i>=0;i--)
	{
		t[j]=s[i];
		j++;
	}
	t[j]='\0';
	move(23,0);
	printw("         ");
	move(23,0);

	if(search(dict,t))
	{
		printw("Correct");
	}
	else
		printw("Incorrect");
	move(y,x);
}

void importtext(Doc* b,char* f)
{
	FILE* fp=fopen(f,"r");
	int i=0;
	char c;
	while((c=fgetc(fp))!=EOF)
	{
		if(i==b->current->gapStart)
			i=b->current->gapEnd+1;
		b->current->text[i]=c;
		printw("%c",c);
		b->current->flag[i/80]=1;
		i++;
	}
	b->current->point=--i;
	move(0,0);
}

void getcommand(Doc* b,int y, int x)
{
	int c=getch();
	int i=0,pos;
	switch(c)
	{
		case KEY_LEFT:
			move(y,0);
			break;
		case KEY_UP:
			move(0,0);
			break;
		case 9:
			searchtext(b);
		default:
			return;
	}
}


void searchtext(Doc* b)
{
	int c;
	trie* tree=create();
	int i=3;
	char s[30];
	int j=0,count=0;
	int len=11;
	int k;
	move(23,len);
	char find[30];
	c=getch();
	while(1)
	{
		if(c=='-')
			break;
		insch(c);
		move(23,len++);
		find[count++]=c;
		c=getch();
	}
	find[count]='\0';

	
	for(int k=0;k<30;k++)
	{
		s[i]='\0';
	}
	while(i!=b->current->point)
	{
		if(i==b->current->gapStart)
			i=b->current->gapEnd+1;
		
		if(b->current->text[i]==' ')
		{
			i++;
			for(k=0;k<30;k++)
				s[k]='\0';
			while(b->current->text[i]!=' ')
			{
				if(i==b->current->gapStart)
					i=b->current->gapEnd+1;
				s[j++]=b->current->text[i++];
				//printw("%c",b->current->text[i++]);
			}
			s[j]='\0';
			j=0;
			//printw("%s",s);
			insert(tree,s);
		}
		
	}	
	if(search(tree,find))
		printw("found");
}
