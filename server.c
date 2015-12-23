//*******************************************************************************
//server.c 	Batuhan Tüter / 21200624
//
//This program is a server that works at the background, waiting for any client
//to send it message. This message will include file names to be manipulated.
//Each client has its own clild process and number of threads inside the child
//process for manipulating the files and sending result back to the client.
//*******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <mqueue.h>

void * manupilateFile(void * arg);
char * keyWord;

//Thread Specific Variables
static __thread int matchCounter;
static __thread int lineNumber;
static __thread char * result;
static __thread int matchLines[500];//Allocate more if the file lines are too big
static __thread FILE *inputFile;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    //Variable declerations
    mqd_t reqQueue, replyQueue;
    ssize_t numRead;
    struct mq_attr attr;
    char * fileNames[20];
    int fileNamesSize=0;
    pthread_t threads[10];
    char * buffer;
    char * replyQueueName;

    char * reqQueueName = malloc(sizeof(argv[1])+1);
    strcpy(reqQueueName,argv[1]);

    //Opening the Request Queue to receive messages from clients
    reqQueue=mq_open(reqQueueName, O_RDWR |O_CREAT,0700,NULL);
    if (reqQueue != (mqd_t) -1)
    {
        mq_getattr(reqQueue, &attr);
        buffer = malloc(attr.mq_msgsize);
        if (buffer == NULL)
            printf("Couldn't create the buffer");

        for(;;)
        {
            //Waiting to receive any available message
            numRead = mq_receive(reqQueue, buffer, attr.mq_msgsize,0);

            if (numRead != -1)
            {
                //Creating child process for each client
                if(fork()==0)
                {
                    char * curLine = buffer;

                    //Allocating keyword
                    if(curLine)
                    {
                        char * nextLine = strchr(curLine, '\n');
                        int curLineLen = nextLine ? (nextLine-curLine) : strlen(curLine);
                        keyWord = (char *) malloc(curLineLen+1);
                        if (keyWord)
                        {

                            memcpy(keyWord, curLine, curLineLen);
                            keyWord[curLineLen] = '\0';  // NUL-terminate!
                        }
                        else printf("malloc() failed!?\n");

                        curLine = nextLine ? (nextLine+1) : NULL;
                    }

                    //Allocating reply queue name
                    if(curLine)
                    {
                        char * nextLine = strchr(curLine, '\n');
                        int curLineLen = nextLine ? (nextLine-curLine) : strlen(curLine);
                        replyQueueName = (char *) malloc(curLineLen+1);
                        if (replyQueueName)
                        {
                            memcpy(replyQueueName, curLine, curLineLen);
                            replyQueueName[curLineLen] = '\0';  // NUL-terminate!
                        }
                        else printf("malloc() failed!?\n");

                        curLine = nextLine ? (nextLine+1) : NULL;
                    }

                    //Allocating file names
                    while(curLine)
                    {
                        char * nextLine = strchr(curLine, '\n');
                        int curLineLen = nextLine ? (nextLine-curLine) : strlen(curLine);
                        char * temp = (char *) malloc(curLineLen+1);
                        if (temp)
                        {
                            memcpy(temp, curLine, curLineLen);
                            temp[curLineLen] = '\0';  // NUL-terminate!
                            fileNames[fileNamesSize]=temp;
                            fileNamesSize++;
                        }
                        else printf("malloc() failed!?\n");

                        curLine = nextLine ? (nextLine+1) : NULL;
                    }

                    //Create Threads for each file
                    int i;
                    for(i=0; i<fileNamesSize;i++)
                    {
                        if(pthread_create(&threads[i],NULL,manupilateFile,fileNames[i])!=0)
                        {
                            printf("Tnread could not created, child process will terminate...");
                            exit(0);//Child Process Terminates
                        }
                    }
                    char * results[fileNamesSize];

                    for(i=0; i<fileNamesSize;i++)
                    {
                        void * result;

                        if(pthread_join(threads[i],&result)!=0)
                        {
                            printf("Tnread could not joined, child process will terminate...");
                            exit(0);//Child Process Terminates
                        }
                        results[i]=(char *)result;
                    }

                    replyQueue=mq_open(replyQueueName,O_RDWR);
                    if (reqQueue == (mqd_t) -1)
                    {
                        printf("Could Not Open Reply-Queue\n");
                        return(0);
                    }

                    for(i=0; i<fileNamesSize;i++)
                    {
                            if(mq_send(replyQueue,results[i], strlen(results[i]), 0) == -1)
                            {
                                printf("Could Not Send Message to Request-Queue");
                                return(0);
                            }
                    }

                //Child Process Finishes
                free(keyWord);
                free(replyQueueName);
                exit(0);
                }
                wait(NULL);

            }
            else if (numRead == -1)
            {
                printf("Could Not Receive Message From Request-Queue\n");
                return(0);
            }
        }
    }
    else
    {
        printf("Could Not Create Request-Queue\n");
        return(0);
    }
    //Memory Deallocation
    int i;
    for(i=0; i<fileNamesSize;i++)
    {
        free(fileNames[i]);
    }

    free(reqQueueName);
    return(0);
}
//Thread Function
void * manupilateFile(void * arg)
{
    //Initialization of thread specific variables and other variables

    char * fileName = (char *) arg;
    result = malloc (sizeof(char *));
    inputFile = fopen(fileName, "r");
    lineNumber =0;
    matchCounter=0;
    size_t nbytes = 0;
    char * myString = (char *) malloc(nbytes);
    int bytes_read;
    pthread_mutex_lock(&mtx);
    //Reading the file line-by-line
    while((bytes_read = getline(&myString, &nbytes, inputFile)) > 0)
    {
        lineNumber++;
        char * line = malloc((bytes_read) * (sizeof(char)));
        if(myString[bytes_read - 1]=='\n')//Reading the file lines
        {
                strncpy(line, myString, ((bytes_read - 1) * (sizeof(char))));//Discard the \n character
                line[bytes_read - 1] = '\0';
        }
        else//Last line to be read in the file
            strncpy(line, myString, ((bytes_read) * (sizeof(char))));

        //Reading lines word-by-word
        const char s[2] = " ";
        char *token = strtok(line, s);
        while( token != NULL )
        {
            if(strcmp(token,keyWord)==0)
            {

                matchLines[matchCounter]=lineNumber;
                matchCounter++;

            }
            token = strtok(NULL, s);
        }
        free(line);
    }
    fclose(inputFile);//After submission I found that this causes a problem time to time
    pthread_mutex_unlock(&mtx);
    sprintf(result,"\n<%s> [%d]: ",fileName, matchCounter);
    int i;
    for(i=0; i<matchCounter; i++)
    {
        char * temp = malloc(sizeof(char *));
        sprintf(temp,"%d ",matchLines[i]);
        strcat(result,temp);
        free(temp);
    }

    //Function finishes
    free(myString);
    pthread_exit(result);
}
