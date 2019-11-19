#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>
#include <sys/socket.h> 
#include <signal.h> 

#define SA struct sockaddr

// Read utility to read entire data from socket, size given by x
int readx(int socket, void* buffer, unsigned int x)
{
    int bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1 )
        {
            // Error
        }
        bytesRead += result;
    }
    return 5;
}
// Utility to print bold text in terminal 
void printfb(char*s)
{
    char str[256],str1[256];
    snprintf(str,255,"\033[1m%s\033[0m",s);
    printf("%s",str);
    fflush(stdout);
} 

// Reads one line from server and prints it
void OneRead(int sockfd)
{
    char buff[256];
    bzero(buff,256);
    int res=readx(sockfd,buff,sizeof(buff));
    if(res==0)
    {
        return;
    }
    
    printf("%s\n",buff);
}

// Reads number of pending notifications from server and displays system notification for the same
void NotifyRead(int sockfd)
{
    char buff[256];
    bzero(buff,256);
    int res=readx(sockfd,buff,sizeof(buff));
    if(res==0)
    {
        return;
    }
    int ret=atoi(buff);
    
    if(ret>0)
    {
    char toshow[256];
    snprintf(toshow,255,"\"You have %d unread notifications.\"",ret);
	char comm[256];
	snprintf(comm,255,"notify-send %s",toshow); // Calls system command notify-send to display notification 
	system(comm);
    }
    return;
}

// Utility to format trade data read from the server and print it 
char* Spaced(char*s, int x)
{
    char *result = malloc(256); 

    char*s1,*s2,*s3,*s4,*s5,*s6;
    s1=strtok(s,",\n");
    s2=strtok(NULL,",\n");
    s3=strtok(NULL,",\n");
    s4=strtok(NULL,",\n");
    s5=strtok(NULL,",\n");
    s6=strtok(NULL,",\n");

    if(!x)
    {
        snprintf(result,255,"Order: #%s    Item: %s    Cost: Rs. %s    Units: %s    Time: %s",s1,s2,s3,s4,s6);
        return result;
    }
    else
    {
        snprintf(result,255,"\tPrice: Rs. %s    Units: %s    CounterParty: %s    Time: %s",s3,s4,s5,s6);
        return result;      
    }
}

// Utility to format notification data read from the server and print it
char* SpacedNot(char*s)
{
    char *result = malloc(256); 

    char*s1,*s2,*s3,*s4,*s5,*s6;
    s1=strtok(s,",\n");
    s2=strtok(NULL,",\n");
    s3=strtok(NULL,",\n");
    s4=strtok(NULL,",\n");
    s5=strtok(NULL,",\n");
    s6=strtok(NULL,",\n");

    snprintf(result,255,"\t > Order: #%s    Item: %s    Price: Rs. %s    Units: %s    CounterParty: %s    Time: %s",s1,s2,s3,s4,s5,s6);
    return result;
    
}

// Reads trade data from the server
void ViewTrade(int sockfd)
{
    char buff[256];
    char*newbuff;
    while(1)
    {
        bzero(buff,256);
        int res=readx(sockfd,buff,sizeof(buff));
        if(res==0)
        	return;
        if(!strcmp(buff,"end")) // Signals the end of trade info sent by the server
        {
        return;
        }
        if(!strcmp(buff,"draw"))
        {
            for(int i=0;i<100;i++) {
                printf("-");
                fflush(stdout);
            }
            printf("\n");
            continue;
        }
        if(!strcmp(buff,"Please login first.\n"))
        {
        printf("%s\n",buff);
        return;
        }
        if(!strcmp(buff, "No orders placed yet.")) {
            printf("%s\n", buff);
            continue;
        }
        if(!strcmp(buff, "\tNo matched trades for this order.")) {
            printf("%s\n", buff);
            continue;
        }

        newbuff = Spaced(buff,buff[0]=='\t'); // Calls Spaced() utility to format this line and print it
        if(buff[0]=='\t')
            printf("%s\n",newbuff);
        else
        {
            char newestbuff[256];
            snprintf(newestbuff,255,"%s\n",newbuff);
            printfb(newestbuff);
        }
   	}
    return;
}

// Reads notification/unread trade data from the server
void ViewNotifs(int sockfd)
{
    char buff[256];
    while(1)
    {
        bzero(buff,256);
        int res=readx(sockfd,buff,sizeof(buff));


        if(res==0)
        {
            return;
        }
        if(!strcmp(buff,"end")) // Signals the end of trade info sent by the server
        {
            return;
        }
        if(!strcmp(buff,"Please login first.\n"))
        {
            printf("%s\n",buff);
            return;
        }
        if(!strcmp(buff,"No New Notifications.\n"))
        {
            printf("%s\n",buff);
            continue;
        }
        printf("%s\n",SpacedNot(buff)); // Calls SpacedNot() utility to format this line and print it
    }
    return;

}

// Reads order data from the server
void ViewOrder(int sockfd)
{
    char buff[256];
    int ctr=0;
    while(1)
    {
        bzero(buff,256);
        int res=readx(sockfd,buff,sizeof(buff));

        if(res==0)
        {
        return;
        }
        if(!strcmp(buff,"end")) // Signals the end of trade info sent by the server
        {
        return;
        }
        if(!strcmp(buff,"Please login first.\n"))
        {
        printf("%s\n",buff);
        return;
        }

        char temp[256];
        strcpy(temp,buff);
        char *a1,*a2;
        a1=strtok(buff," \n");
        a2=strtok(NULL," \n");

        if(ctr==0)
        {
            printfb(temp);
            printf("\n");
        }
        else if(ctr==1)
        {
            if(strcmp(a1,"No"))
                printf("    > Minimum Selling Price : %s    Quantity Available : %s\n",a1,a2);
            else
            {
                printf("    > %s\n",temp);
            }
        }
        else if (ctr==2)
        {
            if(strcmp(a1,"No"))
            {
                printf("    > Maximum Buying Price  : %s    Quantity Available : %s\n",a1,a2);
            }
            else
            {
                printf("    > %s\n",temp);
            }    
            for(int i=0;i<70;i++) { 
                printf("-");
                fflush(stdout);
            }
            printf("\n");
        }
        
        ctr=(ctr+1)%3;
    }

}

// Prints all commands available for the user
void Help()
{
    printf("List of commands: \n\n");
    printfb("  a. crd <username> <password>\n");
    printf("     - Used to Login to the server.\n\n");

    printfb("  b. buy <item_code> <item_price> <item_quantity>\n");
    printf("     - Make a buy request to the server. The buy request will be matched with the best available sell request(lowest price).\n\n");

    printfb("  c. sel <item_code> <item_price> <item_quantity>\n");
    printf("     - Make a sell request to the server. The sell request will be matched with a suitable buy request.\n\n");

    printfb("  d. trd\n");
    printf("     - View the details of your matched orders, their quantities, prices and counterparty code.\n\n");

    printfb("  e. ord\n");
    printf("     - View the current best sell (least price) and the best buy (max price) for each item and their quantities.\n\n");

    printfb("  e. not\n");
    printf("     - View unread notifications/trades.\n\n");

    printfb("  f. end\n");
    printf("     - Exit the application.\n\n");

    return;
}

// Main control function which calls all other functions based on command sent by the client
void Trade(int sockfd)
{
    char input[256];

    while(1)
    {
        printf(">> ");
        fflush(stdout);

        scanf ("%[^\n]%*c", input);
        write(sockfd,input,sizeof(input));
        
        if(input[0]=='c' && input[1]=='r' && input[2]=='d')
        {
            NotifyRead(sockfd);
            OneRead(sockfd);
        }
        else if(input[0]=='n' && input[1]=='o' && input[2]=='t')
        {
            ViewNotifs(sockfd);
        } 
        else if(input[0]=='b' && input[1]=='u' && input[2]=='y')
        {
            OneRead(sockfd);
        }
        else if(input[0]=='s' && input[1]=='e' && input[2]=='l')
        {
            OneRead(sockfd);
        }            
        else if(input[0]=='o' && input[1]=='r' && input[2]=='d')
        {
            ViewOrder(sockfd);
        }            
        else if(input[0]=='t' && input[1]=='r' && input[2]=='d')
        {
            ViewTrade(sockfd);
            printf("\n");
        }            
        else if(input[0]=='e' && input[1]=='n' && input[2]=='d')
        {
            printf("Thanks for trading.\n");           
            return;
        }
        else if(input[0]=='m' && input[1]=='a' && input[2]=='n')
        {
            Help();
        }
        else
        {
            printf("Please enter valid command. (Type man to see the list of commands)\n");
        }
    }
    return;
}

// Signal Handler for SIGTSTP (Stop CTRL+Z) 
void sighandler(int sig_num) 
{ 
    // Reset handler to catch SIGTSTP next time 
    signal(SIGTSTP, sighandler); 
    printf("Cannot execute Ctrl+Z\n"); 
} 

int main(int argc, char const *argv[]) 
{ 
    if (argc < 3) // Checks for required number of arguments to the executable
    {
        printf("Please provide IP address and port of the client");
        fflush(stdout);
        return 0;
    }

	signal(SIGTSTP, sighandler); 

    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 

    // socket create and varification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) { 
        printf("Socket Creation Failed...\n"); 
        exit(0); 
    } 

    bzero(&servaddr, sizeof(servaddr)); 

    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
    servaddr.sin_port = htons(atoi(argv[2])); 

    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("Connection with the server failed...\n"); 
        exit(0); 
    } 

    // function for trading
    Trade(sockfd); 

    // close the socket 
    close(sockfd); 

    return 0; 
} 
