#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <float.h>

#include <netdb.h> 
#include <netinet/in.h>  
#include <sys/socket.h> 
#include <sys/types.h> 
#include <errno.h>  
#include <unistd.h>     
#include <arpa/inet.h>    
#include <sys/time.h>

#define SA struct sockaddr

int usersock[5];

// Return the user currently logged in to the system through the given socket
char* curruser(int sockfd)
{
	char *curruser = (char *) malloc(sizeof(char) * 256);
	for(int i=0;i<5;i++)
	{
		if(usersock[i]==sockfd)
		{
			snprintf(curruser,255,"%d",i+1);
			return curruser;
		}
	}
}

// Utility functin to concat two given strings
char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

// Utility function to return max of two floats 
float max(float a,float b)
{
	if(a>=b)
		return a;
	else
		return b;
}

// Returns current date and time at the server
char* Timestamp()
{
	char *ret = malloc(40);

	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char date_time[30];
	strftime( date_time, sizeof(date_time), "%x %X", t );
	strcpy(ret,date_time);
	return ret;
}


// Handles login functionality
bool Login(int sockfd,char*u,char*p)
{
	char buff[256];
	int res;

 	char inpuser[256],inppass[256];
	strcpy(inpuser,u);
	strcpy(inppass,p);

	FILE *file = fopen ( "Credentials.txt", "r" ); // Opens credentials file to verify username and password
   	if (file == NULL)
   	{
   		return false;
   	}
    else
    {
      char line [ 256 ];

      char *user,*pass;
      while ( fgets ( line, sizeof(line), file ) != NULL ) 
      {
         user = strtok(line," \n");
         pass = strtok(NULL," \n");

         if(!strcmp(user,inpuser) && !strcmp(pass,inppass)) // If username and password match the stored values
        {
        	usersock[u[0]-'1']=sockfd; // Mapping the user to a socket fd

        	FILE*file1 = fopen("Not.txt","r"); // Opens notifications file to read no. of read trades
			char liner[256];
			char reply[256];
			char tosend[256];

			char fname2[256];
			snprintf(fname2,255,"Trade_%s.txt",inpuser); // Opens trade file to read matched trades

			char*l1,*l2,*l3,*l4,*l5;
			int det;
			int x;
			int add,stor,siz=0;
			int send;
			FILE*f3=fopen(fname2,"r");
			if(f3==NULL)
				add=0;
			else
			{
				fgets(liner,sizeof(liner),f3);
				siz=strlen(liner);
				fseek(f3,0,SEEK_END);
				add=ftell(f3);
				fclose(f3);
			}
			int i1,i2,i3,i4,i5;

			while(fgets(liner,sizeof(liner),file1))
			{
				l1=strtok(liner," \n");
				l2=strtok(NULL," \n");
				l3=strtok(NULL," \n");
				l4=strtok(NULL," \n");
				l5=strtok(NULL," \n");
				i1=atoi(l1);
				i2=atoi(l2);
				i3=atoi(l3);
				i4=atoi(l4);
				i5=atoi(l5);

				det=curruser(sockfd)[0]-'0';
				if(det==1) 
				{
					stor=i1;
				}
				else if(det==2)
				{
					stor=i2;
				}
				else if(det==3)
				{
					stor=i3;
				}
				else if(det==4)
				{
					stor=i4;
				}
				else
				{
					stor=i5;
				}
				break;
			}
			fclose(file1);

			if(siz==0)
				send=0;
			else
				send=(add-stor)/siz;
			
			bzero(reply,256);
			snprintf(reply,255,"%d",send);
      		write(sockfd,reply,sizeof(reply)); // Sends no. of unread notifications to client
        	return true;
        }
         
      }
      fclose ( file );

      char temp[256];
		bzero(temp,256);

      snprintf(temp, 255, "-1");
      write(sockfd, temp, sizeof(temp));
  	  return false;
    }	
}

// Inserts a sell order into the appropriate sell file in sorted order according to price
void SellSortedInsert(int item,float price,char*entry)
{
	char file_name[256]={0},temp_file[256]={0};
	snprintf(file_name,255,"Sell_%02d.txt",item);
	snprintf(temp_file,255,"Sell_%02d_stemp.txt",item);

	FILE*fil = fopen(file_name,"r");
	FILE*fil1 = fopen(temp_file,"a");

	if(fil==NULL)
	{
		fprintf(fil1,"%s\n",entry);
		rename(temp_file,file_name);
		fclose(fil1);
		return;
	}
	else
	{
		char line[256],tline[256];
		bool done=false;
		while (fgets(line, sizeof(line), fil)) 
		{
			strcpy(tline,line);

			char*oid,*p,*q,*tn,*ts;
			oid=strtok(line,",\n");
			p=strtok(NULL,",\n");
			q=strtok(NULL,",\n");
			tn=strtok(NULL,",\n");
			ts=strtok(NULL,",\n");

			float pri = atof(p);
			float qua = atof(q);

			// Finds position to enter new line
			if(pri<=price) 
			{
				fprintf(fil1,"%s",tline);
			}
			else
			{
				if(!done)
				{
					fprintf(fil1,"%s\n",entry);
					done=true;
				}
				fprintf(fil1,"%s",tline);
			}

		}

		if(!done)
		{
			fprintf(fil1,"%s\n",entry);
			done=true;
		}
		fclose(fil1);
		remove(file_name);
		rename(temp_file,file_name);
		return;
	}
}

// Handles buy requests
void Buy(int sockfd,char*order_id,int item,float price, float quant)
{
	int save=quant;

	char order_det[256]={0};
	snprintf(order_det,255,"%s,%02d,%07.2f,%07.2f,%s,%s",order_id,item,price,quant,curruser(sockfd),Timestamp());

	char new_name[256];
	snprintf(new_name,255,"Orders_%s.txt",curruser(sockfd));
	FILE*ordf = fopen(new_name,"a");
	fprintf(ordf,"%s\n",order_det); // Appends entry to appropriate orders file
	fclose(ordf);

	char file_name[256]={0},temp_file[256]={0};
	snprintf(file_name,255,"Sell_%02d.txt",item);
	snprintf(temp_file,255,"Sell_%02d_temp.txt",item);

	int del=0;
	FILE*fil = fopen(file_name,"r+"); // Opens corresponding sell file

	// If item's sell file does not exists, buy request is not matched with any sell request
	if(fil==NULL)
	{
		char fname[256];
		snprintf(fname,255,"Buy_%02d.txt",item);
		FILE*fp=fopen(fname,"a");
		char entry[256];
		snprintf(entry,255,"%s,%07.2f,%07.2f,%s,%s",order_id,price,quant,curruser(sockfd),Timestamp());
		fprintf(fp,"%s\n",entry); // Appends entry to appropriate buy requests file
		fclose(fp);

		char ret1[256]= "No match as of now :( . Order stored.";
		char ret2[256];
		bzero(ret2,256);

		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
	}

	char line[256];
	char nline[256];

	// Read sell requests for the item
	while (fgets(line, sizeof(line), fil)) 
	{
		char*oid,*p,*q,*tn,*ts;
		oid=strtok(line,",\n");

		p=strtok(NULL,",\n");		
		q=strtok(NULL,",\n");
		tn=strtok(NULL,",\n");
		ts=strtok(NULL,",\n");

		float pri = atof(p);
		float qua = atof(q);

		if(pri<=price)
		{
			// If greater quantity is available at lesser price, set remaining quantity to buy = 0
			if(qua>quant)
			{
				snprintf(nline,255,"%s,%s,%07.2f,%s,%s",oid,p,qua-quant,tn,ts);
				FILE*f0=fopen(temp_file,"a");
				fprintf(f0,"%s\n",nline);
				fclose(f0);

				char tr1[256]={0};
				snprintf(tr1,255,"Trade_%s.txt",tn);
				FILE*f1=fopen(tr1,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",oid,item,p,quant,curruser(sockfd),Timestamp());
				fprintf(f1, "%s\n",nline);
				fclose(f1);

				char tr2[256]={0};
				snprintf(tr2,255,"Trade_%s.txt",curruser(sockfd));
				FILE*f2=fopen(tr2,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",order_id,item,p,quant,tn,Timestamp());
				fprintf(f2, "%s\n",nline);
				fclose(f2);

				quant=0;
			}

			// If not enough quantity is available at lesser price, update remaining quantity to buy
			else if(qua<=quant)
			{
				char tr1[256]={0};
				snprintf(tr1,255,"Trade_%s.txt",tn);
				FILE*f1=fopen(tr1,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",oid,item,p,qua,curruser(sockfd),Timestamp());
				fprintf(f1, "%s\n",nline);
				fclose(f1);

				char tr2[256]={0};
				snprintf(tr2,255,"Trade_%s.txt",curruser(sockfd));
				FILE*f2=fopen(tr2,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",order_id,item,p,qua,tn,Timestamp());
				fprintf(f2, "%s\n",nline);
				fclose(f2);

				quant-=qua;
			}
		}

		// If sell request has a greater price than the buy request, it will not be matched
		else
		{
				snprintf(nline,255,"%s,%s,%07.2f,%s,%s",oid,p,qua,tn,ts);
				FILE*f0=fopen(temp_file,"a");
				fprintf(f0,"%s\n",nline);
				fclose(f0);
		}
	}
	remove(file_name);
	rename(temp_file,file_name);

	// If quant=0, buy request has been matched completely
	if(quant==0)
	{
		char ret1[256]= "Order completely matched! Enter 'trd' for more info.";
		char ret2[256];
		bzero(ret2,256);
		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
	}
	else
	{
		char fname[256];
		snprintf(fname,255,"Buy_%02d.txt",item);
		FILE*fp=fopen(fname,"a");
		char entry[256];
		snprintf(entry,255,"%s,%07.2f,%07.2f,%s,%s",order_id,price,quant,curruser(sockfd),Timestamp());
		fprintf(fp,"%s\n",entry);
		fclose(fp);

		// If entire quantity is remaining, no matches have occurred
		if(save==quant)
		{
			char ret1[256]= "No match as of now :( . Order stored.";
			char ret2[256];
		bzero(ret2,256);

			snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
			write(sockfd,ret2,sizeof(ret2));
			return;
		}
		// Otherwise, request has been partially matched
		else
		{
			char ret1[256]= "Order partially matched! Remaining order stored. Enter 'trd' for more info.";
			char ret2[256];
		bzero(ret2,256);

			snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
			write(sockfd,ret2,sizeof(ret2));
			return;
		}
	}
	return;
}

// Handles sell requests
void Sell(int sockfd,char*order_id,int item,float price, float quant)
{
	int save=quant;

	char order_det[256]={0};
	snprintf(order_det,255,"%s,%02d,%07.2f,%07.2f,%s,%s",order_id,item,price,quant,curruser(sockfd),Timestamp());

	char new_name[256];
	snprintf(new_name,255,"Orders_%s.txt",curruser(sockfd));
	FILE*ordf = fopen(new_name,"a");
	fprintf(ordf,"%s\n",order_det); // Inserts entry into appropriate orders file
	fclose(ordf);

	char file_name[256]={0},temp_file[256]={0};
	snprintf(file_name,255,"Buy_%02d.txt",item);
	snprintf(temp_file,255,"Buy_%02d_temp.txt",item);

	int del=0;
	FILE*fil = fopen(file_name,"r+"); // Opens corresponding buy file

	// If item's buy file does not exists, sell request is not matched with any buy request
	if(fil==NULL)
	{
		char entry[256];
		snprintf(entry,255,"%s,%07.2f,%07.2f,%s,%s",order_id,price,quant,curruser(sockfd),Timestamp());
		SellSortedInsert(item,price,entry); // Inserts entry into appropriate sell requests file

		char ret1[256]= "No match as of now :( . Order stored.";
		char ret2[256];
		bzero(ret2,256);
		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
	}

	char line[256];
	char nline[256];

	// Read buy requests for the item
	while (fgets(line, sizeof(line), fil)) 
	{
		char*oid,*p,*q,*tn,*ts;
		oid=strtok(line,",\n");

		p=strtok(NULL,",\n");		
		q=strtok(NULL,",\n");
		tn=strtok(NULL,",\n");
		ts=strtok(NULL,",\n");

		float pri = atof(p);
		float qua = atof(q);

		if(pri>=price)
		{
			// If greater quantity is available at higher price, set remaining quantity to sell = 0
			if(qua>quant)
			{
				snprintf(nline,255,"%s,%s,%07.2f,%s,%s",oid,p,qua-quant,tn,ts);
				FILE*f0=fopen(temp_file,"a");
				fprintf(f0,"%s\n",nline);
				fclose(f0);

				char tr1[256]={0};
				snprintf(tr1,255,"Trade_%s.txt",tn);
				FILE*f1=fopen(tr1,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",oid,item,p,quant,curruser(sockfd),Timestamp());
				fprintf(f1, "%s\n",nline);
				fclose(f1);

				char tr2[256]={0};
				snprintf(tr2,255,"Trade_%s.txt",curruser(sockfd));
				FILE*f2=fopen(tr2,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",order_id,item,p,quant,tn,Timestamp());
				fprintf(f2, "%s\n",nline);
				fclose(f2);

				quant=0;
			}

			// If not enough quantity is available at higher price, update remaining quantity to sell
			else if(qua<=quant)
			{
				char tr1[256]={0};
				snprintf(tr1,255,"Trade_%s.txt",tn);
				FILE*f1=fopen(tr1,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",oid,item,p,qua,curruser(sockfd),Timestamp());
				fprintf(f1, "%s\n",nline);
				fclose(f1);

				char tr2[256]={0};
				snprintf(tr2,255,"Trade_%s.txt",curruser(sockfd));
				FILE*f2=fopen(tr2,"a");
				snprintf(nline,255,"%s,%02d,%s,%07.2f,%s,%s",order_id,item,p,qua,tn,Timestamp());
				fprintf(f2, "%s\n",nline);
				fclose(f2);

				quant-=qua;
			}
		}

		// If buy request has a lower price than the sell request, it will not be matched
		else
		{
				snprintf(nline,255,"%s,%s,%07.2f,%s,%s",oid,p,qua,tn,ts);
				FILE*f0=fopen(temp_file,"a");
				fprintf(f0,"%s\n",nline);
				fclose(f0);

		}
	}
	remove(file_name);
	rename(temp_file,file_name);

	// If quant=0, sell request has been matched completely
	if(quant==0)
	{
		char ret1[256]= "Order completely matched! Enter 'trd' for more info.";
		char ret2[256];
		bzero(ret2,256);
		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
	}
	else
	{
		char entry[256];
		snprintf(entry,255,"%s,%07.2f,%07.2f,%s,%s",order_id,price,quant,curruser(sockfd),Timestamp());
		SellSortedInsert(item,price,entry);
		// If entire quantity is remaining, no matches have occurred
		if(save==quant)
		{
		char ret1[256]= "No match as of now :( . Order stored.";
		char ret2[256];
		bzero(ret2,256);
		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
		}
		// Otherwise, request has been partially matched
		else
		{
		char ret1[256]= "Order partially matched! Remaining order stored. Enter 'trd' for more info.";
		char ret2[256];
		bzero(ret2,256);
		snprintf(ret2,255,"Your order no. is %s.\n%s\n",order_id,ret1);
		write(sockfd,ret2,sizeof(ret2));
		return;
		}
	}
	return;
}

// Sends trade info to the client
void ViewTrade(int sockfd)
{
	char fname1[256],fname2[256];
	snprintf(fname1,255,"Orders_%s.txt",curruser(sockfd));
	snprintf(fname2,255,"Trade_%s.txt",curruser(sockfd));

	char line[256];
	char line1[256];
	char line2[256];
	char tline[256];

	FILE*f1=fopen(fname1,"r");
	FILE*f2=fopen(fname2,"r");

	// If file is not present, user has no orders
	if(f1==NULL)
	{
		bzero(line2,256);
		snprintf(line2,255,"No orders placed yet.");
		write(sockfd,line2,sizeof(line2));
		char temp[256];
		bzero(temp,256);
	    snprintf(temp, 255, "end");
	    write(sockfd, temp, sizeof(temp));

		return;
	}
	int ct=0;
	while(fgets(line,sizeof(line),f1))
	{
		bzero(tline,256);
		strcpy(tline,line);
		char*oid,*it,*p,*q,*tn,*ts;
		oid=strtok(line,",\n");
		it=strtok(line,",\n");
		p=strtok(NULL,",\n");
		q=strtok(NULL,",\n");
		tn=strtok(NULL,",\n");
		ts=strtok(NULL,",\n");

		write(sockfd,tline,sizeof(tline));

		char ord[256];
		bzero(ord,256);
		snprintf(ord,255,"draw");

		// If file is not present, user has no matched trades
		if(f2==NULL)
		{
		bzero(line2,256);
			snprintf(line2,255,"\tNo matched trades for this order.");
			write(sockfd,line2,sizeof(line2));
		}

		// Find matched trades for the order ID in the trades file
		else
		{
			ct=0;
			while(fgets(line1,sizeof(line1),f2))
			{
				strcpy(tline,line1);
				char*oid1;
				oid1=strtok(line1,",\n");

				if(!strcmp(oid,oid1))
				{
					ct++;
					bzero(line2,256);
					snprintf(line2,255,"\t%s",tline);
					write(sockfd,line2,sizeof(line2));
				}

			}
			// If no trades were found
			if(ct==0)
			{
				bzero(line2,256);
				snprintf(line2,255,"\tNo matched trades for this order.");
				write(sockfd,line2,sizeof(line2));				
			}
		}
		f2=fopen(fname2,"r");
		write(sockfd,ord,sizeof(ord));				
	}
	fclose(f1);
	if(f2) fclose(f2);

	char temp[256];
	bzero(temp,256);
    snprintf(temp, 255, "end");
    write(sockfd, temp, sizeof(temp));

    // Updating notifications file to mark all trades as read

	FILE*file1 = fopen("Not.txt","r");
	char liner[256];
	char reply[256];

	char*l1,*l2,*l3,*l4,*l5;
	int det;
	int x;
	int add;
	FILE*f3=fopen(fname2,"r");
	if(f3==NULL)
		add=0;
	else
	{
		fseek(f3,0,SEEK_END);
		add=ftell(f3);
		fclose(f3);
	}
	int i1,i2,i3,i4,i5;

	while(fgets(liner,sizeof(liner),file1))
	{
		l1=strtok(liner," \n");
		l2=strtok(NULL," \n");
		l3=strtok(NULL," \n");
		l4=strtok(NULL," \n");
		l5=strtok(NULL," \n");
		i1=atoi(l1);
		i2=atoi(l2);
		i3=atoi(l3);
		i4=atoi(l4);
		i5=atoi(l5);

		det=curruser(sockfd)[0]-'0';
		if(det==1)
		{
			i1=add;
		}
		else if(det==2)
		{
			i2=add;
		}
		else if(det==3)
		{
			i3=add;
		}
		else if(det==4)
		{
			i4=add;
		}
		else
		{
			i5=add;
		}

		snprintf(reply,254,"%d %d %d %d %d\n",i1,i2,i3,i4,i5);
		break;
	}
	fclose(file1);

	FILE*filep = fopen("Not.txt","w");
	fprintf(filep,reply,sizeof(reply));
	fclose(filep);

	return;
}

// Sends order info (max selling and min buying price) to the client
void ViewOrder(int sockfd)
{
	int i;
	FILE*b,*s;
	char bname[256],sname[256],entry[256],line[256];
	for(i=1;i<=10;i++)
	{
		snprintf(bname,255,"Buy_%02d.txt",i);
		snprintf(sname,255,"Sell_%02d.txt",i);
		b=fopen(bname,"r");
		s=fopen(sname,"r");

		bzero(entry,256);
		snprintf(entry,255,"Item #%02d",i);
		write(sockfd,entry,sizeof(entry));

		// Finding minimum selling price for the ith item
		float pri,quant;
		if(s==NULL)
		{
		bzero(entry,256);
			snprintf(entry,255,"No sell request for the item.");
			write(sockfd,entry,sizeof(entry));
		}
		else
		{
			pri=0;
			quant=0;
			int ct=0;
			while(fgets(line,sizeof(line),s))
			{
				char*oid,*p,*q,*tn,*ts;
				oid=strtok(line,",\n");
				p=strtok(NULL,",\n");
				q=strtok(NULL,",\n");
				tn=strtok(NULL,",\n");
				ts=strtok(NULL,",\n");
				if(ct==0)
				{
					pri=atof(p);
					quant=atof(q);
					ct++;
				}
				else
				{
					if(atof(p)==pri)
						quant+=atof(q);
					else
						break;
				}
			}
		bzero(entry,256);
			snprintf(entry,255,"%07.2f %07.2f",pri,quant);
			write(sockfd,entry,sizeof(entry));

			fclose(s);
		}

		// Finding maximum buying price for the ith item 
		if(b==NULL)
		{
		bzero(entry,256);
			snprintf(entry,255,"No buy request for the item.");
			write(sockfd,entry,sizeof(entry));
		}
		else
		{
			pri=FLT_MIN;
			quant=0;
			while(fgets(line,sizeof(line),b))
			{
				char*oid,*p,*q,*tn,*ts;
				oid=strtok(line,",\n");
				p=strtok(NULL,",\n");
				q=strtok(NULL,",\n");
				tn=strtok(NULL,",\n");
				ts=strtok(NULL,",\n");

				pri=max(pri,atof(p));
			}

			fseek(b,0,SEEK_SET);
			while(fgets(line,sizeof(line),b))
			{
				char*oid,*p,*q,*tn,*ts;
				oid=strtok(line,",\n");
				p=strtok(NULL,",\n");
				q=strtok(NULL,",\n");
				tn=strtok(NULL,",\n");
				ts=strtok(NULL,",\n");
				if(abs(atof(p)-pri)<=0.005)
				{
					quant+=atof(q);
				}
			}
		bzero(entry,256);

			snprintf(entry,255,"%07.2f %07.2f",pri,quant);
			write(sockfd,entry,sizeof(entry));

			fclose(b);
		}
	}
	char temp[256];
		bzero(temp,256);
    snprintf(temp, 255, "end");
    write(sockfd, temp, sizeof(temp));
	return;
}

// Check if the user operating through given socket is logged in
bool LoggedIn(int sockfd)
{
	for(int i=0;i<5;i++)
	{
		if(usersock[i]==sockfd)
		return true;			
	}
	return false;
}

// Send notifications (unread trades) to the client
void Notify(int sockfd)
{
	FILE*file1 = fopen("Not.txt","r");
	char liner[256];
	char reply[256];

	char*l1,*l2,*l3,*l4,*l5;
	int det;
	int store;
	int i1,i2,i3,i4,i5;

	// Get read trades from the notifications file
	while(fgets(liner,sizeof(liner),file1))
	{
		l1=strtok(liner," \n");
		l2=strtok(NULL," \n");
		l3=strtok(NULL," \n");
		l4=strtok(NULL," \n");
		l5=strtok(NULL," \n");
		i1=atoi(l1);
		i2=atoi(l2);
		i3=atoi(l3);
		i4=atoi(l4);
		i5=atoi(l5);

		det=curruser(sockfd)[0]-'0';
		if(det==1)
		{
			store=i1;
		}
		else if(det==2)
		{
			store=i2;
		}
		else if(det==3)
		{
			store=i3;
			
		}
		else if(det==4)
		{
			store=i4;
		}
		else
		{
			store=i5;
		}
		break;
	}
	fclose(file1);

	char fname2[256];
	snprintf(fname2,255,"Trade_%s.txt",curruser(sockfd));

	FILE*fread=fopen(fname2,"r");
	int ct=0;
	if(fread!=NULL) {
		fseek(fread,store,SEEK_SET);

		ct=0;
		bzero(liner,256);
		while(fgets(liner,sizeof(liner),fread))
		{
			ct++;
			write(sockfd,liner,sizeof(liner));
			bzero(liner,256);
		}
		fclose(fread);
	}
	if(ct==0)
	{
		char temp[256];
		bzero(temp,256);
		snprintf(temp, 255, "No New Notifications.\n");
		write(sockfd,temp,sizeof(temp));		
	}

	char temp[256];
	bzero(temp,256);
    snprintf(temp, 255, "end");
    write(sockfd, temp, sizeof(temp));

    // Updating notification files with the updated values

	file1 = fopen("Not.txt","r");

	int add;
	FILE*f3=fopen(fname2,"r");
	if(f3==NULL)
		add=0;
	else
	{
		fseek(f3,0,SEEK_END);
		add=ftell(f3);
		fclose(f3);
	}

	while(fgets(liner,sizeof(liner),file1))
	{
		l1=strtok(liner," \n");
		l2=strtok(NULL," \n");
		l3=strtok(NULL," \n");
		l4=strtok(NULL," \n");
		l5=strtok(NULL," \n");
		i1=atoi(l1);
		i2=atoi(l2);
		i3=atoi(l3);
		i4=atoi(l4);
		i5=atoi(l5);

		det=curruser(sockfd)[0]-'0';
		if(det==1)
		{
			i1=add;
		}
		else if(det==2)
		{
			i2=add;
		}
		else if(det==3)
		{
			i3=add;
		}
		else if(det==4)
		{
			i4=add;
		}
		else
		{
			i5=add;
		}

		snprintf(reply,254,"%d %d %d %d %d\n",i1,i2,i3,i4,i5);
		break;
	}
	fclose(file1);

	FILE*filep = fopen("Not.txt","w");
	fprintf(filep,reply,sizeof(reply));
	fclose(filep);

	return;
}

// Main control function which calls all other functions based on command sent by the client 
void Trade(int sockfd,char*com) 
{ 
	char buff[256];
	int res;	
	bzero(buff,256);
	strcpy(buff,com);

	if(buff[0]=='n' && buff[1]=='o' && buff[2]=='t')
	{
		if(!LoggedIn(sockfd))
		{
			char reply[256];  
			bzero(reply,256);   		
			snprintf(reply,255,"Please login first.\n");
			write(sockfd,reply,sizeof(reply));
			return;
		}

		Notify(sockfd);
	}
	else if(buff[0]=='c' && buff[1]=='r' && buff[2]=='d')
	{
		char*com,*u,*p;
		com = strtok(buff," \n");
		u = strtok(NULL," \n");
		p = strtok(NULL," \n");

		bool ret=Login(sockfd,u,p);
		char reply[256];
		if(!ret)
		{
		bzero(reply,256);
			snprintf(reply,255,"Login unsuccessful! Incorrect credentials.\n");
			write(sockfd,reply,sizeof(reply));
		}
		else
		{
		bzero(reply,256);
			snprintf(reply,255,"Welcome, User %s!\n", curruser(sockfd));
			write(sockfd,reply,sizeof(reply));
		}

	}
	else if(buff[0]=='b' && buff[1]=='u' && buff[2]=='y')
	{
		if(!LoggedIn(sockfd))
		{
				char reply[256];
		bzero(reply,256);	
			snprintf(reply,255,"Please login first.\n");
				write(sockfd,reply,sizeof(reply));
				return;
		}

		char*com,*it,*pr,*qty;
		com = strtok(buff," \n");
		it = strtok(NULL," \n");
		pr = strtok(NULL," \n");
		qty = strtok(NULL," \n");

		int item = atoi(it);
		float price = atof(pr);
		float quant = atof(qty);

		FILE*numFile = fopen("Counter.txt","r+"); // Reads counter file to make the next order ID for the buy or sell request
		char line[256],z[256];
		char*ordCode,*lin,*lin2;
		int lint,lint2;
		while (fgets(line, sizeof(line), numFile)) 
		{
			lin=strtok(line," \n");
			lin2=strtok(NULL," \n");

			ordCode=concat("B",lin);
			lint=atoi(lin);
			lint++;
			lint2=atoi(lin2);
			snprintf(z,255,"%04d %04d",lint,lint2);

			fseek(numFile,0,SEEK_SET);
			fprintf(numFile, "%s\n",z); // Updates the counter file with the incremented value
			break;
		}
		fclose(numFile);
		Buy(sockfd,ordCode,item,price,quant);
	}
	else if(buff[0]=='s' && buff[1]=='e' && buff[2]=='l')
	{
		if(!LoggedIn(sockfd))
		{
			char reply[256];  
		bzero(reply,256);   		
			snprintf(reply,255,"Please login first.\n");
			write(sockfd,reply,sizeof(reply));
			return;
		}
	    char*com,*it,*pr,*qty;
		com = strtok(buff," \n");
		it = strtok(NULL," \n");
		pr = strtok(NULL," \n");
		qty = strtok(NULL," \n");

		int item = atoi(it);
		float price = atof(pr);
		float quant = atof(qty);

		FILE*numFile = fopen("Counter.txt","r+");
		char line[256],z[256];
		char*ordCode,*lin,*lin2;
		int lint,lint2;
		while (fgets(line, sizeof(line), numFile)) 
		{
			lin=strtok(line," \n");
			lin2=strtok(NULL," \n");

			ordCode=concat("S",lin2);
			
			lint=atoi(lin);
			lint2=atoi(lin2);
			lint2++;
			snprintf(z,255,"%04d %04d",lint,lint2);

			fseek(numFile,0,SEEK_SET);
			fprintf(numFile, "%s\n",z);
			break;
		}
		fclose(numFile);
		Sell(sockfd,ordCode,item,price,quant);
	}            
	else if(buff[0]=='o' && buff[1]=='r' && buff[2]=='d')
	{
	   	if(!LoggedIn(sockfd))
		{
			char reply[256];  
		bzero(reply,256);   		
			snprintf(reply,255,"Please login first.\n");
			write(sockfd,reply,sizeof(reply));
			return;
		}
	    ViewOrder(sockfd);
	}            
	else if(buff[0]=='t' && buff[1]=='r' && buff[2]=='d')
	{
	   	if(!LoggedIn(sockfd))
		{
			char reply[256]; 
		bzero(reply,256);    		
			snprintf(reply,255,"Please login first.\n");
			write(sockfd,reply,sizeof(reply));
			return;
		}
	    ViewTrade(sockfd);
	}            
	return;
} 


// Driver function 
int main(int argc, char const *argv[]) 
{ 
	// Check if port number is supplied
	if (argc < 2)
	{
		printf("Please give port number as argument\n");
		return 0;
	}

	for(int i=0;i<5;i++)
		usersock[i]=-1;


	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("Socket Creation Failed!\n"); 
		exit(0); 
	} 
	else
		printf("Socket Successfully Created!\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(atoi(argv[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("Socket Bind Failed!\n"); 
		exit(0); 
	} 
	else
		printf("Socket Successfully Binded!\n"); 

	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen Failed!\n"); 
		exit(0); 
	} 
	else
		printf("Server listening...\n"); 
	len = sizeof(cli); 

	fd_set readfds;
	int clientfd[5];
	for(int i=0;i<5;i++)
		clientfd[i]=0;

	int max_sd,sd,activity;
	char buffer[256];
	int valread;
	while (1)
	{
        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(sockfd, &readfds);   
        max_sd = sockfd;   
             
        //add child sockets to set  
        for (int i = 0 ; i < 5 ; i++)   
        {   
            //socket descriptor  
            sd = clientfd[i];   
                 
            //if valid socket descriptor then add to read list  
            if(sd > 0)   
                FD_SET( sd , &readfds);   
                 
            //highest file descriptor number, need it for the select function  
            if(sd > max_sd)   
                max_sd = sd;   
        } 

        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
       
        if ((activity < 0) && (errno!=EINTR))   
        {   
            printf("Select Error!\n");   
        }

        if (FD_ISSET(sockfd, &readfds))   
        {   
			// Accept the data packet from client and verification and create a new socket
			connfd = accept(sockfd, (SA*)&cli, &len); 
			if (connfd < 0) { 
				printf("Server Acccept Failed!\n"); 
				continue; 
			} 
			else
				printf("Server acccepts the client\n"); 
             
            //inform user of socket number - used in send and receive commands  
            printf("New connection, socket fd is %d, IP: %s , Port: %d\n" , connfd , inet_ntoa(cli.sin_addr) , ntohs (cli.sin_port));      
                 
            //add new socket to array of sockets  
            for (int i = 0; i < 5; i++)   
            {   
                //if position is empty  
                if( clientfd[i] == 0 )   
                {   
                    clientfd[i]=connfd;   
                    break;   
                }   
            }   
        }

         for (int i = 0; i < 5; i++)   
        {   
            sd = clientfd[i];   
            if (FD_ISSET( sd , &readfds))   
            {   
                //Check if it was for closing, and also read the incoming message  
                if ((valread = read( sd , buffer, 256)) == 0)   
                {   
                    // Get details of disconnected host and print them  
                    getpeername(sd , (struct sockaddr*)&cli , \ 
                        (socklen_t*)&len);   
                    printf("Host disconnected, IP: %s , Port: %d \n" ,  
                          inet_ntoa(cli.sin_addr) , ntohs(cli.sin_port));   
   
                    for(int i=0;i<5;i++)
                    {
                    	if(usersock[i]==sd)
                    		usersock[i]=-1;
                    } 
                    //Close the socket and mark as 0 in list for reuse  
                    close( sd );   
                    clientfd[i] = 0;  
                }   
                
                // Call trade function if activity was not for disconnection
                else 
                {    
					Trade(sd,buffer); 
                }   
            }   
        } 
	}
	// After finishing the task close the socket 
	close(sockfd); 
} 
