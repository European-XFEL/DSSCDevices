/*
 * File:   DSSC_PPT_FTP.cc
 * Author: kirchgessner
 *
 * Created on 18. März 2014, 11:12
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <ctype.h>

#include <fcntl.h>
#include <netdb.h>

#include "DSSC_PPT_FTP.h"
#include "utils.h"

#define LENGTH 1024

//#define DEBUG


using namespace SuS;

DSSC_PPT_FTP::DSSC_PPT_FTP(std::string _hostIPAddress, int _ftpPort)
{
  openFtp(_hostIPAddress,_ftpPort);
}

DSSC_PPT_FTP::~DSSC_PPT_FTP()
{
  close(sockfd);
#ifdef DEBUG
  printf("[Client] Connection closed.\n");
#endif
}


int DSSC_PPT_FTP::getOpenSocket(int dataPort)
{
  struct sockaddr_in remote_data;
  /* Fill the socket address struct */
  remote_data.sin_family = AF_INET;
  remote_data.sin_port = htons(dataPort);
  inet_pton(AF_INET,hostIPAddress.c_str(), &remote_data.sin_addr);
  bzero(&(remote_data.sin_zero), 8);

  int dataSocket;
  if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
      fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor! (errno = %d)\n",errno);
      return false;
  }
#ifdef DEBUG
  else
  {
      printf("INFO: Obtained Socket!\n");
  }
#endif
  /* Try to connect the remote */
  if (connect(dataSocket, (struct sockaddr *) &remote_data, sizeof(struct sockaddr)) == -1)
  {
      fprintf(stderr, "ERROR: Failed to connect to the data port of the host! (errno = %d), dataPort: %d\n",errno,dataPort);
      return false;
  }
#ifdef DEBUG
  else
  {
      printf("[Client] Connected to server data port %d...ok!\n", dataPort);
  }
#endif
  return dataSocket;
}

bool DSSC_PPT_FTP::openFtp(std::string _hostIPAddress, int _ftpPort)
{
  hostIPAddress = _hostIPAddress;
  ftpPort = _ftpPort;

  sockfd = getOpenSocket(ftpPort);

  char buf[100];
  recv(sockfd,buf,100,0);
#ifdef DEBUG
  printf("FTPServer Answerd with %s\n",buf);
#endif

  std::string answer(buf);
  if(answer.find("220")!=std::string::npos){
    sendReadCommand(buf,"USER anonymous");
  }

  return true;

}

void DSSC_PPT_FTP::sendReadCommand(char * buf, std::string cmd)
{
  cmd += "\r\n";
  int length = cmd.length();
#ifdef DEBUG
  printf("Server Send cmd %s\n",cmd.c_str());
#endif

  strcpy(buf,cmd.c_str());
  send(sockfd,buf,length,0);
  memset(buf,0,100);
  recv(sockfd,buf,100,0);
#ifdef DEBUG
  printf("Server Answerd with %s\n",buf);
#endif
}

void DSSC_PPT_FTP::sendCommand(char * buf, std::string cmd)
{
  cmd += "\r\n";
  int length = cmd.length();
#ifdef DEBUG
  printf("Server Send cmd %s\n",cmd.c_str());
#endif
  strcpy(buf,cmd.c_str());
  send(sockfd,buf,length,0);
  memset(buf,0,length);
}

bool DSSC_PPT_FTP::sendFile(std::string fileName)
{
  /* Send File to Server */
  int fh = open(fileName.c_str(), O_RDONLY);
  if(fh == -1){
      printf("ERROR: File %s not found.\n", fileName.c_str());
      return false;
  }

  int status,size;
  struct stat obj;
  char buf[100];

  sendReadCommand(buf,"PASV");

  int dataPort = getOpenedPort(buf);
  if(dataPort == -1){
    return false;
  }

  std::string baseName = getBaseFileName(fileName);

  sendCommand(buf, "STOR " + baseName); // send data before wait for answer

  int dataSocket = getOpenSocket(dataPort);

  off_t offset = 0;
  stat(fileName.c_str(), &obj);
  size = obj.st_size;

  printf("Sending File of size %d...\n",size);

  sendfile(dataSocket, fh, &offset, size);

  close(fh);

  close(dataSocket);

  recv(sockfd,buf,100,0);

  status = getReturnCode(buf);
  if(!status){
    printf("ERROR: Sending file files: error %d\n",status);
    return false;
  }
  else
  {
    recv(sockfd,buf,100,0);
#ifdef DEBUG
    printf("INFO: Got Return Status: %s\n",buf);
#endif
    std::string cmd = "SIZE " + utils::getFileName(fileName);
    sendReadCommand(buf,cmd);

    int numBytesToReceive = getFileSize(buf);
    if(numBytesToReceive == size){
      printf("File stored successfully. Status: %d\n",status);
    }

  }
  return true;
}

bool DSSC_PPT_FTP::readFile(std::string fileName)
{
  /* Read File from Server */
  printf("Read File from FTP!\n");

  char buf[100];

  sendReadCommand(buf,"PASV");

  int dataPort = getOpenedPort(buf);
  if(dataPort == -1){
    return false;
  }

  sendReadCommand(buf, "TYPE A"); // send data before wait for answer

  std::string cmd = "SIZE " + fileName;

  sendReadCommand(buf,cmd);

  int numBytesToReceive = getFileSize(buf);

  cmd = "RETR " + fileName;

  sendCommand(buf,cmd);

  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWOTH;

  int fh = open(fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC,mode);
  if(fh == -1){
      printf("ERROR: File could not be created!\n");
      return false;
  }

  int dataSocket = getOpenSocket(dataPort);

  char * dataBuf = new char[numBytesToReceive];
  int bytesLeft = numBytesToReceive;
  while(bytesLeft != 0){

    ssize_t rc = recv(dataSocket, dataBuf, bytesLeft, MSG_WAITALL);
    if (rc == 0){
      break;
    }else if (rc < 0 && errno != EINTR){
      fprintf(stderr, "Exit %d\n", __LINE__);
      break;
    }else if (rc > 0){
#ifdef DEBUG
      printf("INFO: Still %d bytes to read!\n",bytesLeft);
#endif
      bytesLeft -= rc;
    }
  }

  if(bytesLeft != 0 ){
    printf("ERROR: Number of bytes read not correct: %d unread bytes/%d!\n",bytesLeft,numBytesToReceive);
  }

  int bytesWritten = write(fh,dataBuf,numBytesToReceive);
  if(bytesWritten != numBytesToReceive){
    printf("ERROR: Number of bytes written not correct %d/%d!\n",bytesWritten,numBytesToReceive);
  }

  close(fh);

  close(dataSocket);

  delete [] dataBuf;

  recv(sockfd,buf,100,0);
#ifdef DEBUG
  printf("Server Answerd with %s\n",buf);
#endif
  return true;
}

int DSSC_PPT_FTP::getReturnCode(const char * buf)
{
  std::string answer(buf);
  int pos = answer.find_first_of(' ');
  int retVal = atoi(answer.substr(0,pos).c_str());
#ifdef DEBUG
  printf("INFO: Got Return Status: %s\n",buf);
#endif
  if(answer.find("150 Ok")==std::string::npos){
    printf("ERROR: File can not be received\n");
  }
  return retVal;
}

int DSSC_PPT_FTP::getOpenedPort(const char * buf)
{
  std::string answer(buf);

  if(answer.find("227 PASV ok")==std::string::npos){
    printf("ERROR - getOpenedPort: Server answered error something went wrong: %s\n",buf);
    return -1;
  }

  int start = answer.find_first_of('(')+1;
  int length = answer.find_last_of(')')-start;
  std::string ansdata = answer.substr(start,length);
#ifdef DEBUG
  printf("INFO: Found data: %s\n",ansdata.c_str());
#endif

  int pos = ansdata.find_first_of(',');
  std::string address = ansdata.substr(0,pos) + ".";
  ansdata = ansdata.substr(pos+1,std::string::npos);
  pos = ansdata.find_first_of(',');
  address += ansdata.substr(0,pos) + ".";
  ansdata = ansdata.substr(pos+1,std::string::npos);
  pos = ansdata.find_first_of(',');
  address += ansdata.substr(0,pos) + ".";
  ansdata = ansdata.substr(pos+1,std::string::npos);
  pos = ansdata.find_first_of(',');
  address += ansdata.substr(0,pos);
  ansdata = ansdata.substr(pos+1,std::string::npos);
#ifdef DEBUG
  printf("INFO: Found Address: %s\n",address.c_str());
#endif

  int port = 0;
  pos = ansdata.find_first_of(',');
  std::string numStr = ansdata.substr(0,pos);
  port = atoi(numStr.c_str())<<8;
  ansdata = ansdata.substr(pos+1,std::string::npos);
  port += atoi(ansdata.c_str());
#ifdef DEBUG
  printf("INFO: Found Port: %d\n",port);
#endif
  return port;
}

int DSSC_PPT_FTP::getFileSize(const char * buf)
{
  std::string answer(buf);

  if(answer.find("213")==std::string::npos){
    printf("ERROR - getFileSize: Server answered error something went wrong: %s\n",buf);
    return -1;
  }

  int start = answer.find_first_of(' ')+1;
  int length = answer.length() - start;
  std::string ansdata = answer.substr(start,length);
#ifdef DEBUG
  printf("INFO: Found data: %s\n",ansdata.c_str());
#endif
  int fileSize = atoi(ansdata.c_str());

#ifdef DEBUG
  printf("INFO: NumBytes to receive: %d\n",fileSize);
#endif
  return fileSize;
}

std::string DSSC_PPT_FTP::getBaseFileName(std::string filePathName)
{
  unsigned found = filePathName.find_last_of("/\\");
  return filePathName.substr(found+1);
}

void DSSC_PPT_FTP::error(const char *msg)
{
    perror(msg);
}
