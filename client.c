//*********************************************************************************
//client.c 	Batuhan Tüter / 21200624
//
//This program is a client that sends a message to a server. This message includes
//the file names that the client wants server to manipulate. It waits for
//the server to send results back and prints the acquired results back to screen
//*********************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <mqueue.h>

int main(int argc, char *argv[])
{
	//Variable Declerations
    mqd_t reqQueue,replyQueue;
    ssize_t numRead;
    struct mq_attr attr;
    char * keyWord = argv[2];
    int numberofFiles = atoi(argv[3]);
    char * message = malloc(sizeof(char*)*1000);
    char * pid = malloc(sizeof(char*));
    sprintf(pid,"%d",getpid());

    char * reqQueueName = malloc(sizeof(argv[1])+1);
    strcpy(reqQueueName,argv[1]);

    char * replyQueueName = malloc(sizeof(pid)+1);
    strcpy(replyQueueName,"/");
    strcat(replyQueueName,pid);
    //printf("Reply Queue Name: %s",replyQueueName);

    strcpy(message,keyWord);
    strcat(message,"\n");
    strcat(message,replyQueueName);
    strcat(message,"\n");

    //Opening the Request Queue to send message to server
    reqQueue=mq_open(reqQueueName,O_RDWR);
    if (reqQueue == (mqd_t) -1)
    {
        printf("Could Not Open Request-Queue\n");
        return(0);
    }
    //Opening the Reply Queue to get results from server
    replyQueue=mq_open(replyQueueName, O_RDWR |O_CREAT,0700,NULL);
    if (replyQueue == (mqd_t) -1)
    {
        printf("Could Not Create Reply-  Queue\n");
        return(0);
    }

    //Shaping the message
    int i;
    for(i=0; i<numberofFiles;i++)
    {
        strcat(message,argv[i+4]);
        if((i+1)!=numberofFiles)strcat(message,"\n");
    }
    //Sending the message to Server
    if(mq_send(reqQueue, message, strlen(message), 0) == -1)
    {
        printf("Could Not Send Message to Request-Queue");
        return(0);
    }

    //Receiving the results from server and printing them
    printf("keyword: %s",keyWord);
    mq_getattr(replyQueue, &attr);
    char * results[numberofFiles];

    for(i=0; i<numberofFiles; i++)
    {
        results[i] = malloc(attr.mq_msgsize);
        numRead = mq_receive(replyQueue, results[i], attr.mq_msgsize,0);
        printf("%s",results[i]);
        if (numRead == -1)
        {
            printf("Could Not Receive Message From Reply-Queue\n");
            return(0);
        }
    }

    printf("\n\n");

    //Deletion of the reply queue
    if (mq_unlink(replyQueueName) == -1)
    {
        printf("Could Not Delete Reply-Queue\n");
        return(0);
    }

    //Memory Deallocation
    for(i=0; i<numberofFiles; i++)
        free(results[i]);

    free(reqQueueName);
    free(replyQueueName);
    free(message);
    free(pid);

    return(0);
}
